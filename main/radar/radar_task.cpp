#include "radar_task.h"

#include <freertos/timers.h>

#include "debug.hpp"
#include "escort-jni.hpp"
#include "esp_central.h"

void radar_status_request(TimerHandle_t pxTimer) {
    if (xRadarOutboxQueue != NULL) {
        bleQueueItem queueItem;
        queueItem.data = (uint8_t *)malloc(3);
        uint8_t statusRequest[3] = {0xF5, 0x01, 0x94};
        memcpy(queueItem.data, statusRequest, sizeof(statusRequest));
        queueItem.data_length = sizeof(statusRequest);
        xQueueSendToBack(xRadarOutboxQueue, (void *)&queueItem, portMAX_DELAY);
    }
}

QueueHandle_t getRadarInbox() { return xRadarInboxQueue; }
QueueHandle_t getRadarOutbox() { return xRadarOutboxQueue; }

void radar_task(void *pvParameter) {
    ESP_LOGI(RADAR_TAG, "task start");

    TimerHandle_t xRadarStatusTimer = xTimerCreate("RadarStatus", pdMS_TO_TICKS(5000), pdTRUE, (void *)0, radar_status_request);
    if (xTimerStart(xRadarStatusTimer, pdMS_TO_TICKS(1000)) == pdTRUE) {
        ESP_LOGI(RADAR_TAG, "status request timer started");
    } else {
        ESP_LOGE(RADAR_TAG, "could not start status request timer!");
    }

    xRadarInboxQueue = xQueueCreate(10, sizeof(bleQueueItem));
    xRadarOutboxQueue = xQueueCreate(10, sizeof(bleQueueItem));

    while (true) {
        BLEDeviceTaskNotification notification = BLEDeviceTaskNotification::NONE;

        if (xTaskNotifyWait(0, ULONG_MAX, (uint32_t *)&notification, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (notification == BLEDeviceTaskNotification::DISCONNECTED) {
                // Kill task, we're no longer connected to radar!
                vQueueDelete(xRadarInboxQueue);
                vQueueDelete(xRadarOutboxQueue);
                xRadarInboxQueue = NULL;
                xRadarOutboxQueue = NULL;
                return;
            }
        }
        bleQueueItem iPacket;
        if (xQueueReceive(xRadarInboxQueue, &iPacket, pdMS_TO_TICKS(50)) == pdTRUE) {
            // Packets follow format
            //  F5 _length_ _code_ _data_
            if (iPacket.data[0] != 0xF5) {
                ESP_LOGE(RADAR_TAG, "dropping malformed packet, bad magic byte!");
                continue;
            }

            if (iPacket.data_length - 2 != iPacket.data[1]) {
                ESP_LOGW(RADAR_TAG, "dropping malformed packet, bad length!");
                continue;
            }

            // Valid packet, unpack it into individual components.
            uint8_t commandByte = iPacket.data[2];
            uint8_t dataLength = iPacket.data[1];
            uint8_t *data = (uint8_t *)malloc(dataLength);
            if (data == NULL) {
                ESP_LOGE(RADAR_TAG, "could not malloc for packet databuf!");
                continue;
            }
            memcpy(data, iPacket.data + 3, dataLength);
            free(iPacket.data);

            const char *dataStr = uint8_to_hex_string(data, dataLength).c_str();
            ESP_LOGI(RADAR_TAG, "(Packet) command byte: %02hhx, datalen: %02hhx, data: %s", commandByte, dataLength, dataStr);
            // ESP_LOGD(RADAR_TAG, "datalength: %02hhx", dataLength);

            if (commandByte == 0xA1) {
                // Radar locked, unlock it with the static keys
                uint32_t *out = (uint32_t *)malloc(2);

                esc_pack(data, out);

                uint32_t *enc = (uint32_t *)malloc(2);
                xtea_encrypt(35, out, SMARTCORD_KEY, enc);

                uint8_t *unpack = (uint8_t *)malloc(10);
                esc_unpack(enc, unpack);

                uint8_t authResponsePacket[13] = {0xF5,      0x0b,      0xA4,      unpack[0], unpack[1], unpack[2], unpack[3],
                                                  unpack[4], unpack[5], unpack[6], unpack[7], unpack[8], unpack[9]};
                bleQueueItem aPacket;
                aPacket.data = (uint8_t *)malloc(13);
                aPacket.data_length = 13;
                memcpy(aPacket.data, authResponsePacket, 13);
                ESP_LOGI(RADAR_TAG, "Unlocking detector");
                xQueueSendToBack(xRadarOutboxQueue, (void *)&aPacket, portMAX_DELAY);
            } else if (commandByte == 0xA3) {
                if (data[0] == 0x01) {
                    ESP_LOGW(RADAR_TAG, "Radar locked!");
                } else if (data[0] == 0x02) {
                    ESP_LOGE(RADAR_TAG, "Radar locked out!");
                } else {
                    ESP_LOGI(RADAR_TAG, "Radar unlocked!");
                }
            }
        }
    }
}