
#include <esp_event.h>
#include <freertos/FreeRTOS.h>
#include <nvs_flash.h>
#include <stdio.h>

#include "ble_task.h"
#include "mqtt_task.h"
#include "provisioning_task.h"
#include "wifi_task.h"

extern "C" void app_main(void) {
    /* Initialize NVS — it is used to store PHY calibration data */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    xBLESemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(xBLESemaphore);

    blecent_init();
    provisioning_init();
    wifi_init();
    mqtt_init();

    while (true) {
        ESP_LOGI("main", "free heap: %" PRIu32 "b", esp_get_free_heap_size());
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}