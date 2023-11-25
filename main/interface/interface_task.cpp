#include "interface_task.h"

#include "esp_central.h"

QueueHandle_t getInterfaceInbox() { return xInterfaceInboxQueue; }
QueueHandle_t getInterfaceOutbox() { return xInterfaceOutboxQueue; }

void interface_task(void *pvParameter) {
    ESP_LOGI(INTERFACE_TAG, "task start");

    xInterfaceInboxQueue = xQueueCreate(10, sizeof(bleQueueItem));
    xInterfaceOutboxQueue = xQueueCreate(10, sizeof(bleQueueItem));

    while (true) {
        BLEDeviceTaskNotification notification = BLEDeviceTaskNotification::NONE;

        if (xTaskNotifyWait(0, ULONG_MAX, (uint32_t *)&notification, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (notification == BLEDeviceTaskNotification::DISCONNECTED) {
                // Kill task, we're no longer connected to radar!
                vQueueDelete(xInterfaceInboxQueue);
                vQueueDelete(xInterfaceOutboxQueue);
                xInterfaceInboxQueue = NULL;
                xInterfaceOutboxQueue = NULL;
                return;
            }
        }
    }
}