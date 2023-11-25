#ifndef H_HELPER_BLE_
#define H_HELPER_BLE_

#include <host/ble_uuid.h>

#include "esp_central.h"

#define BLE_TAG "nimble_helper"

/*** Radar constants ***/

// Service UUID
const ble_uuid128_t radar_service_uuid =
    BLE_UUID128_INIT(0x44, 0xA3, 0x7A, 0x83, 0xE0, 0x9B, 0x6A, 0xBE, 0xAB, 0x42, 0xEE, 0x31, 0xE9, 0x2D, 0xE2, 0xB5);

// TX UUID
const ble_uuid128_t radar_tx_uuid = BLE_UUID128_INIT(0x44, 0xA3, 0x7A, 0x83, 0xE0, 0x9B, 0x6A, 0xBE, 0xAB, 0x42, 0xEE, 0x31, 0xEA, 0x2D, 0xE2, 0xB5);

// RX UUID
const ble_uuid128_t radar_rx_uuid = BLE_UUID128_INIT(0x44, 0xA3, 0x7A, 0x83, 0xE0, 0x9B, 0x6A, 0xBE, 0xAB, 0x42, 0xEE, 0x31, 0xEB, 0x2D, 0xE2, 0xB5);

/*** LVGL interface constants ***/

// Service UUID
const ble_uuid128_t interface_service_uuid =
    BLE_UUID128_INIT(0xAF, 0x2E, 0x72, 0x69, 0xCE, 0x7B, 0x47, 0x7F, 0x85, 0x12, 0xCC, 0x72, 0xA0, 0x3C, 0xC0, 0xBB);

// TX UUID
const ble_uuid128_t interface_tx_uuid =
    BLE_UUID128_INIT(0xAF, 0x2E, 0x72, 0x6A, 0xCE, 0x7B, 0x47, 0x7F, 0x85, 0x12, 0xCC, 0x72, 0xA0, 0x3C, 0xC0, 0xBB);

// RX UUID
const ble_uuid128_t interface_rx_uuid =
    BLE_UUID128_INIT(0xAF, 0x2E, 0x72, 0x6B, 0xCE, 0x7B, 0x47, 0x7F, 0x85, 0x12, 0xCC, 0x72, 0xA0, 0x3C, 0xC0, 0xBB);

// CCCD descriptor UUID
const ble_uuid16_t cccd_uuid = BLE_UUID16_INIT(0x2902);

void ble_hosttask(void *param);
void blecent_scan(void);
int blecent_on_write(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg);
int blecent_should_connect(const struct ble_gap_disc_desc *disc);
void blecent_connect_if_interesting(void *disc);
int blecent_gap_event(struct ble_gap_event *event, void *arg);
void blecent_on_reset(int reason);
void blecent_on_sync(void);
void blecent_on_disc_complete(const struct peer *peer, int status, void *arg);
void blecent_rx_subscribe(const struct peer *peer);
int blecent_write(uint16_t conn_handle, const bleQueueItem dataToWrite);
int blecent_on_subscribe(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg);
void ble_init();
void ble_helpertask(void *pvParameter);

#endif