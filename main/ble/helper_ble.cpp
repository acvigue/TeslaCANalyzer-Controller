#include "helper_ble.h"

#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "blecent.h"
#include "console/console.h"
#include "debug.hpp"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "interface_task.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "radar_task.h"
#include "services/gap/ble_svc_gap.h"

// some wack shit
extern "C" {
void ble_store_config_init(void);
}

uint16_t radar_conn_handle = 0;
uint16_t interface_conn_handle = 0;

void ble_hosttask(void *param) {
    ESP_LOGI(BLE_TAG, "nimble task started");

    nimble_port_run();  // Won't return until ble shits iteslf

    nimble_port_freertos_deinit();
}

void blecent_scan(void) {
    uint8_t own_addr_type;
    int rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(BLE_TAG, "error determining address type; rc=%d", rc);
        return;
    }

    struct ble_gap_disc_params disc_params;
    disc_params.filter_duplicates = 1;
    disc_params.passive = 1;
    disc_params.itvl = 0;
    disc_params.window = 0;
    disc_params.filter_policy = 0;
    disc_params.limited = 0;

    rc = ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &disc_params, blecent_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(BLE_TAG, "Error initiating GAP discovery procedure; rc=%d", rc);
    }
}

int blecent_should_connect(const struct ble_gap_disc_desc *disc) {
    /* The device has to be advertising connectability. */
    if (disc->event_type != BLE_HCI_ADV_RPT_EVTYPE_ADV_IND && disc->event_type != BLE_HCI_ADV_RPT_EVTYPE_DIR_IND) {
        return 0;
    }

    struct ble_hs_adv_fields fields;
    int rc = ble_hs_adv_parse_fields(&fields, disc->data, disc->length_data);
    if (rc != 0) {
        return 0;
    }

    for (int i = 0; i < fields.num_uuids128; i++) {
        if (ble_uuid_cmp(&fields.uuids128[i].u, &radar_service_uuid.u) == 0 || ble_uuid_cmp(&fields.uuids128[i].u, &interface_service_uuid.u) == 0) {
            return 1;
        }
    }

    return 0;
}

void blecent_connect_if_interesting(void *disc) {
    uint8_t own_addr_type;
    int rc;
    ble_addr_t *addr;

    /* Don't do anything if we don't care about this advertiser. */
    if (!blecent_should_connect((struct ble_gap_disc_desc *)disc)) {
        return;
    }

    /* Scanning must be stopped before a connection can be initiated. */
    rc = ble_gap_disc_cancel();
    if (rc != 0) {
        ESP_LOGD(BLE_TAG, "Failed to cancel scan; rc=%d", rc);
        return;
    }

    /* Figure out address to use for connect (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(BLE_TAG, "error determining address type; rc=%d", rc);
        return;
    }

    addr = &((struct ble_gap_disc_desc *)disc)->addr;

    rc = ble_gap_connect(own_addr_type, addr, 30000, NULL, blecent_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(BLE_TAG,
                 "Error: Failed to connect to device; addr_type=%d "
                 "addr=%s; rc=%d",
                 addr->type, addr_str(addr->val), rc);
        return;
    }
}

/**
 * Subscribes to notifications for the device's corresponding RX
 * characteristic.
 *
 * Subscription happens by sending [0x01, 0x00] to the
 * characteristic's CCCD descriptor (0x2902)
 */

void blecent_rx_subscribe(const struct peer *peer) {
    const struct peer_chr *chr = NULL;
    const struct peer_dsc *dsc = NULL;
    bool isRadar = false;
    int rc;

    /* Determine device type & attach CCCD descriptor to dsc */
    chr = peer_chr_find_uuid(peer, &radar_service_uuid.u, &radar_rx_uuid.u);
    if (chr != NULL) {
        ESP_LOGI(BLE_TAG, "peer R/W callback determined device type: radar");
        isRadar = true;
        dsc = peer_dsc_find_uuid(peer, &radar_service_uuid.u, &radar_rx_uuid.u, &cccd_uuid.u);
    }

    chr = peer_chr_find_uuid(peer, &interface_service_uuid.u, &interface_rx_uuid.u);
    if (chr != NULL) {
        ESP_LOGI(BLE_TAG, "peer R/W callback determined device type: interface");
        dsc = peer_dsc_find_uuid(peer, &interface_service_uuid.u, &interface_rx_uuid.u, &cccd_uuid.u);
    }

    if (dsc == NULL) {
        ESP_LOGE(BLE_TAG, "peer lacks a CCCD for the RX service!");
        ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return;
    }

    uint8_t value[2] = {1, 1};
    rc = ble_gattc_write_flat(peer->conn_handle, dsc->dsc.handle, value, sizeof value, blecent_on_subscribe, NULL);
    if (rc != 0) {
        ESP_LOGE(BLE_TAG, "failed to subscribe to characteristic: rc=%d", rc);
        ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return;
    }

    if (isRadar) {
        xTaskCreatePinnedToCore(radar_task, "RadarTask", 4096, (void *)&peer, 1, &xRadarTask, 1);
        radar_conn_handle = peer->conn_handle;
    } else {
        xTaskCreatePinnedToCore(interface_task, "InterfaceTask", 4096, (void *)&peer, 1, &xInterfaceTask, 1);
        interface_conn_handle = peer->conn_handle;
    }

    if (xRadarTask == NULL || xInterfaceTask == NULL) {
        ESP_LOGI(BLE_TAG, "not all devices linked, restarting discovery.");
        blecent_scan();
    }
}

/**
 * Application callback.  Called when the attempt to subscribe to notifications
 * for the device's RX characteristic has completed.
 */
int blecent_on_subscribe(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg) {
    struct peer *peer;

    ESP_LOGI(BLE_TAG, "Subscribe complete: status=%d, conn_handle=%d, attr_handle=%d", error->status, conn_handle, attr->handle);

    peer = peer_find(conn_handle);
    if (peer == NULL) {
        ESP_LOGE(BLE_TAG, "Error finding peer, aborting...");
        ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }

    return 0;
}

int blecent_write(bool toRadar, const bleQueueItem dataToWrite) {
    uint16_t conn_handle = toRadar ? radar_conn_handle : interface_conn_handle;
    const struct peer *peer = peer_find(conn_handle);
    if (peer == NULL) {
        ESP_LOGE(BLE_TAG, "write error: peer null, aborting connection.");
        ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return -1;
    }

    const struct peer_chr *chr = NULL;

    chr = peer_chr_find_uuid(peer, &radar_service_uuid.u, &radar_tx_uuid.u);
    if (chr == NULL) {
        chr = peer_chr_find_uuid(peer, &interface_service_uuid.u, &interface_tx_uuid.u);
        if (chr == NULL) {
            ESP_LOGE(BLE_TAG, "write determined device type: unknown");
            return ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        } else {
            ESP_LOGD(BLE_TAG, "write determined device type: interface");
        }
    } else {
        ESP_LOGD(BLE_TAG, "write determined device type: radar");
    }

    int rc = ble_gattc_write_flat(conn_handle, chr->chr.val_handle, dataToWrite.data, dataToWrite.data_length, blecent_on_write, NULL);
    if (rc != 0) {
        ESP_LOGE(BLE_TAG, "failed to write characteristic: rc %d", rc);
        return ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }

    return 0;
}

int blecent_on_write(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg) {
    ESP_LOGD(BLE_TAG, "Write complete; status=%d conn_handle=%d attr_handle=%d", error->status, conn_handle, attr->handle);
    return 0;
}

/**
 * Called when service discovery of the specified peer has completed.
 */
void blecent_on_disc_complete(const struct peer *peer, int status, void *arg) {
    if (status != 0) {
        /* Service discovery failed.  Terminate the connection. */
        ESP_LOGE(BLE_TAG, "service discovery failed: status=%d, conn_handle=%d", status, peer->conn_handle);
        ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return;
    }

    ESP_LOGI(BLE_TAG, "service discovery complete: status=%d, conn_handle=%d", status, peer->conn_handle);
    blecent_rx_subscribe(peer);
}

int blecent_gap_event(struct ble_gap_event *event, void *arg) {
    struct ble_gap_conn_desc desc;
    struct ble_hs_adv_fields fields;
    int rc;

    switch (event->type) {
        case BLE_GAP_EVENT_DISC:
            rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
            if (rc != 0) {
                return 0;
            }

            blecent_connect_if_interesting(&event->disc);
            return 0;
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(BLE_TAG, "Connection established ");

                rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
                assert(rc == 0);

                rc = peer_add(event->connect.conn_handle);
                if (rc != 0) {
                    ESP_LOGE(BLE_TAG, "Failed to add peer; rc=%d", rc);
                    return 0;
                }

                rc = ble_gap_security_initiate(event->connect.conn_handle);
                if (rc != 0) {
                    ESP_LOGI(BLE_TAG, "Security could not be initiated, rc = %d", rc);
                    rc = peer_disc_all(event->connect.conn_handle, blecent_on_disc_complete, NULL);
                    if (rc != 0) {
                        ESP_LOGE(BLE_TAG, "Failed to discover services; rc=%d", rc);
                        return 0;
                    }
                } else {
                    ESP_LOGI(BLE_TAG, "Connection secured");
                }
            } else {
                ESP_LOGE(BLE_TAG, "Error: Connection failed; status=%d", event->connect.status);
                blecent_scan();
            }

            return 0;
        case BLE_GAP_EVENT_ENC_CHANGE:
            ESP_LOGI(BLE_TAG, "encryption change event; status=%d ", event->enc_change.status);
            rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
            assert(rc == 0);
            print_conn_desc(&desc);

            rc = peer_disc_all(event->connect.conn_handle, blecent_on_disc_complete, NULL);
            if (rc != 0) {
                ESP_LOGE(BLE_TAG, "Failed to discover services; rc=%d", rc);
                return 0;
            }
            return 0;
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(BLE_TAG, "disconnect; reason=%d ", event->disconnect.reason);
            print_conn_desc(&event->disconnect.conn);
            ESP_LOGI(BLE_TAG, "");

            peer_delete(event->disconnect.conn.conn_handle);

            blecent_scan();
            return 0;
        case BLE_GAP_EVENT_DISC_COMPLETE:
            ESP_LOGI(BLE_TAG, "discovery complete; reason=%d", event->disc_complete.reason);
            return 0;
        case BLE_GAP_EVENT_NOTIFY_RX:
            bleQueueItem iPacket;
            iPacket.data = (uint8_t *)malloc(OS_MBUF_PKTLEN(event->notify_rx.om));
            if (iPacket.data == NULL) {
                ESP_LOGE(BLE_TAG, "could not malloc buffer for inc. packet");
                return 0;
            }

            iPacket.data_length = OS_MBUF_PKTLEN(event->notify_rx.om);
            rc = os_mbuf_copydata(event->notify_rx.om, 0, OS_MBUF_PKTLEN(event->notify_rx.om), iPacket.data);
            if (rc != 0) {
                ESP_LOGE(BLE_TAG, "could not decode mbuf");
                return 0;
            }

            if (event->notify_rx.conn_handle == radar_conn_handle) {
                xQueueSendToBack(xRadarInboxQueue, (void *)&iPacket, portMAX_DELAY);
            } else if (event->notify_rx.conn_handle == interface_conn_handle) {
                xQueueSendToBack(xInterfaceInboxQueue, (void *)&iPacket, portMAX_DELAY);
            } else {
                ESP_LOGE(BLE_TAG, "bad peer!");
                free(iPacket.data);
            }
            return 0;

        case BLE_GAP_EVENT_MTU:
            ESP_LOGI(BLE_TAG, "mtu update event; conn_handle=%d cid=%d mtu=%d", event->mtu.conn_handle, event->mtu.channel_id, event->mtu.value);
            return 0;

        case BLE_GAP_EVENT_REPEAT_PAIRING:
            rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
            assert(rc == 0);
            ble_store_util_delete_peer(&desc.peer_id_addr);
            return BLE_GAP_REPEAT_PAIRING_RETRY;
        default:
            ESP_LOGI(BLE_TAG, "event: %" PRIu8, event->type);
            return 0;
    }
    return 0;
}

void blecent_on_reset(int reason) { ESP_LOGE(BLE_TAG, "Resetting state; reason=%d", reason); }

void blecent_on_sync(void) {
    int rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);
    blecent_scan();
}

void ble_init() {
    int ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(BLE_TAG, "Failed to init nimble %d ", ret);
        return;
    }

    ble_hs_cfg.reset_cb = blecent_on_reset;
    ble_hs_cfg.sync_cb = blecent_on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_sc = 0;

    int rc = peer_init(3, 64, 64, 64);
    assert(rc == 0);

    rc = ble_svc_gap_device_name_set("tCANcontroller");
    assert(rc == 0);

    ble_store_config_init();

    nimble_port_freertos_init(ble_hosttask);
    xTaskCreatePinnedToCore(ble_helpertask, "BLEHelperTask", 4096, NULL, 2, NULL, 1);
}

void ble_helpertask(void *pvParameter) {
    ESP_LOGI(BLE_TAG, "helper task started");
    while (true) {
        if (xRadarOutboxQueue != NULL || xInterfaceOutboxQueue != NULL) {
            if (xRadarOutboxQueue != NULL) {
                bleQueueItem queueItem;
                if (xQueueReceive(xRadarOutboxQueue, (void *)&queueItem, pdMS_TO_TICKS(50)) == pdTRUE) {
                    ESP_LOGD(BLE_TAG, "helper sending %d bytes of data to radar", queueItem.data_length);
                    blecent_write(true, queueItem);
                }
            }
            if (xInterfaceOutboxQueue != NULL) {
                bleQueueItem queueItem;
                if (xQueueReceive(xInterfaceOutboxQueue, &queueItem, pdMS_TO_TICKS(50)) == pdTRUE) {
                    ESP_LOGD(BLE_TAG, "helper sending %d bytes of data to interface", queueItem.data_length);
                    blecent_write(false, queueItem);
                }
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}