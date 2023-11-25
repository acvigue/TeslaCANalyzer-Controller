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
            }
        }
        bleQueueItem inboxItem;
        if (xQueueReceive(xRadarInboxQueue, &inboxItem, pdMS_TO_TICKS(50)) == pdTRUE) {
            // Packets follow format
            //  F5 _length_ _code_ _data_
            if (!validateInboxItem(inboxItem)) {
                ESP_LOGW(TAG, "dropping invalid packet");
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
            ESP_LOGI(TAG, "(Packet) command byte: %02hhx, datalen: %02hhx, data: %s", thisPacket.commandByte, thisPacket.len, dataStr);

            processPacket(thisPacket, xRadarOutboxQueue);
        }
    }
}