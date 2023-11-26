#include "radar_task.h"

#include <freertos/timers.h>

#include "debug.hpp"
#include "esp_central.h"
#include "packet_processing.h"

#define TAG "radar"

void radar_status_request(TimerHandle_t pxTimer) {
    if (xRadarOutboxQueue != NULL) {
        bleQueueItem queueItem;
        queueItem.data = (uint8_t *)malloc(3);
        uint8_t statusRequest[3] = {0xF5, 0x01, 0x94};
        memcpy(queueItem.data, statusRequest, sizeof(statusRequest));
        queueItem.data_length = sizeof(statusRequest);
        xQueueSendToBack(xRadarOutboxQueue, (void *)&queueItem, portMAX_DELAY);
        ESP_LOGD(TAG, "sending status request");
    }
}

// TODO: update speed limit from mqtt
void radar_update_speed_limit(TimerHandle_t pxTimer) {
    if (xRadarOutboxQueue != NULL) {
        // speed limit
        bleQueueItem queueItem;
        queueItem.data = (uint8_t *)malloc(5);
        uint8_t statusRequest[5] = {REQ_START, 0x03, REQ_SPEED_LIMIT_UPDATE, 86, 0};
        memcpy(queueItem.data, statusRequest, sizeof(statusRequest));
        queueItem.data_length = sizeof(statusRequest);
        xQueueSendToBack(xRadarOutboxQueue, (void *)&queueItem, portMAX_DELAY);
        ESP_LOGD(TAG, "sending status request");
    }
}

QueueHandle_t getRadarInbox() { return xRadarInboxQueue; }
QueueHandle_t getRadarOutbox() { return xRadarOutboxQueue; }

void radar_task(void *pvParameter) {
    ESP_LOGI(TAG, "task start");

    TimerHandle_t xRadarStatusTimer = xTimerCreate("RadarStatus", pdMS_TO_TICKS(5000), pdTRUE, (void *)0, radar_status_request);
    if (xTimerStart(xRadarStatusTimer, pdMS_TO_TICKS(1000)) == pdTRUE) {
        ESP_LOGI(TAG, "status request timer started");
    } else {
        ESP_LOGE(TAG, "could not start status request timer!");
    }

    TimerHandle_t xRadarSpeedLimitTimer = xTimerCreate("RadarSpeedLimit", pdMS_TO_TICKS(5000), pdTRUE, (void *)0, radar_update_speed_limit);
    if (xTimerStart(xRadarSpeedLimitTimer, pdMS_TO_TICKS(1000)) == pdTRUE) {
        ESP_LOGI(TAG, "speed limit timer started");
    } else {
        ESP_LOGE(TAG, "could not start speed limit timer!");
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
                vTaskDelete(NULL);
                xTimerStop(xRadarStatusTimer, portMAX_DELAY);
                xTimerStop(xRadarSpeedLimitTimer, portMAX_DELAY);
            }
        }
        bleQueueItem inboxItem;
        if (xQueueReceive(xRadarInboxQueue, &inboxItem, pdMS_TO_TICKS(50)) == pdTRUE) {
            // Packets follow format
            //  F5 _length_ _code_ _data_
            if (!validateInboxItem(inboxItem)) {
                ESP_LOGW(TAG, "dropping invalid packet");
                free(inboxItem.data);
                continue;
            }

            // Valid packet, unpack it into individual components.
            RadarPacket thisPacket = {.commandByte = inboxItem.data[2], .len = inboxItem.data[1], .buf = (uint8_t *)malloc(inboxItem.data[1])};
            if (thisPacket.buf == NULL) {
                ESP_LOGE(TAG, "could not malloc for packet databuf!");
                continue;
            }
            memcpy(thisPacket.buf, inboxItem.data + 3, thisPacket.len);
            free(inboxItem.data);

            const char *dataStr = uint8_to_hex_string(thisPacket.buf, thisPacket.len).c_str();
            ESP_LOGD(TAG, "(Packet) command byte: %02hhx, datalen: %02hhx, data: %s", thisPacket.commandByte, thisPacket.len, dataStr);

            processPacket(thisPacket, xRadarOutboxQueue);
        }
    }
}

void send_alert(RadarAlertType alertType, uint32_t distanceFt, uint32_t heading) {
    if (xRadarOutboxQueue != NULL) {
        ESP_LOGI(TAG, "sending alert type=%" PRIu8 " distance=%" PRIu32, alertType, distanceFt);
        uint8_t *displayLocPacket = (uint8_t *)malloc(5);
        displayLocPacket = getDisplayLocationPacket((uint8_t)alertType, distanceFt, heading);
        bleQueueItem queueItem;
        queueItem.data = (uint8_t *)malloc(8);
        uint8_t alertPayload[8] = {
            REQ_START,          0x06, REQ_DISPLAY_LOCATION, displayLocPacket[0], displayLocPacket[1], displayLocPacket[2], displayLocPacket[3],
            displayLocPacket[4]};
        free(displayLocPacket);
        memcpy(queueItem.data, alertPayload, sizeof(alertPayload));
        queueItem.data_length = sizeof(alertPayload);
        xQueueSendToBack(xRadarOutboxQueue, (void *)&queueItem, portMAX_DELAY);
    }
}

void clear_alert() {
    if (xRadarOutboxQueue != NULL) {
        ESP_LOGI(TAG, "clearing alert");
        bleQueueItem queueItem;
        queueItem.data = (uint8_t *)malloc(3);
        uint8_t clearAlertPayload[3] = {REQ_START, 0x01, REQ_DISPLAY_CLEAR_LOCATION};
        memcpy(queueItem.data, clearAlertPayload, sizeof(clearAlertPayload));
        queueItem.data_length = sizeof(clearAlertPayload);
        xQueueSendToBack(xRadarOutboxQueue, (void *)&queueItem, portMAX_DELAY);
    }
}

void clear_message(TimerHandle_t tvParameter) {
    if (xRadarOutboxQueue != NULL) {
        ESP_LOGI(TAG, "clearing message");
        bleQueueItem queueItem;
        queueItem.data = (uint8_t *)malloc(5);
        uint8_t clearMessagePayload[5] = {REQ_START, 0x03, REQ_DISPLAY_MESSAGE, 0x00, 0x00};
        memcpy(queueItem.data, clearMessagePayload, sizeof(clearMessagePayload));
        queueItem.data_length = sizeof(clearMessagePayload);
        xQueueSendToBack(xRadarOutboxQueue, (void *)&queueItem, portMAX_DELAY);
    }
}

void play_tone(uint8_t toneType) {
    if (xRadarOutboxQueue != NULL) {
        bleQueueItem queueItem;
        queueItem.data = (uint8_t *)malloc(4);
        uint8_t playTonePayload[4] = {REQ_START, 2, REQ_PLAY_TONE, toneType};
        memcpy(queueItem.data, playTonePayload, sizeof(playTonePayload));
        queueItem.data_length = sizeof(playTonePayload);
        xQueueSendToBack(xRadarOutboxQueue, (void *)&queueItem, portMAX_DELAY);
    }
}

void show_message(char *msg, size_t len) {
    if (xRadarOutboxQueue != NULL) {
        ESP_LOGI(TAG, "sending message len=%d", len);

        play_tone(1);

        bleQueueItem queueItem;
        queueItem.data = (uint8_t *)malloc(4 + len);
        uint8_t totalLen = 2 + len;
        uint8_t showMessagePayload[4] = {REQ_START, totalLen, REQ_DISPLAY_MESSAGE, 9};
        memcpy(queueItem.data, showMessagePayload, sizeof(showMessagePayload));
        memcpy(queueItem.data + 4, msg, len);
        queueItem.data_length = 4 + len;
        xQueueSendToBack(xRadarOutboxQueue, (void *)&queueItem, portMAX_DELAY);

        TimerHandle_t xClearMsgTimer = xTimerCreate("ClearMessageTimer", pdMS_TO_TICKS(5000), pdFALSE, (void *)0, clear_message);
        xTimerStart(xClearMsgTimer, pdMS_TO_TICKS(50));
    }
}