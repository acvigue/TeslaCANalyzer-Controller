
#include <freertos/FreeRTOS.h>
#include <helper_ble.h>
#include <nvs_flash.h>
#include <stdio.h>

extern "C" void app_main(void) {
    /* Initialize NVS — it is used to store PHY calibration data */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ble_init();

    while (true) {
        ESP_LOGI("main", "free heap: %" PRIu32 "b", esp_get_free_heap_size());
        vTaskDelay(5000);
    }
}