#include "provisioning_task.h"

#include <esp_event.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>

#include "ble_task.h"

#define TAG "provisioning"

void prov_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_CRED_FAIL: {
                ESP_LOGE(TAG, "provisioning error");

                if (xProvisioningTask != NULL) {
                    xTaskNotify(xProvisioningTask, ProvisioningTaskNotification_t::RESET_SM_ON_FAILURE, eSetValueWithOverwrite);
                }

                break;
            }
            case WIFI_PROV_CRED_SUCCESS: {
                ESP_LOGI(TAG, "provisioning successful");
                break;
            }
            case WIFI_PROV_END: {
                ESP_LOGI(TAG, "provisioning end");
                wifi_prov_mgr_deinit();
                break;
            }
            case WIFI_PROV_START: {
                ESP_LOGI(TAG, "provisioning started");
                break;
            }
            case WIFI_PROV_CRED_RECV: {
                ESP_LOGI(TAG, "credentials received, attempting connection");
                break;
            }
            case WIFI_PROV_DEINIT: {
                xSemaphoreGive(xBLESemaphore);
                if (xBLEHelperTask != NULL) {
                    xTaskNotify(xBLEHelperTask, BLEHelperTaskNotification::NIMBLE_START, eSetValueWithOverwrite);
                }
            }
        }
    }
}

void provisioning_task(void *pvParameter) {
    ESP_LOGI(TAG, "task started");
    while (true) {
        ProvisioningTaskNotification_t notification;
        if (xTaskNotifyWait(0, ULONG_MAX, (uint32_t *)&notification, portMAX_DELAY) == pdTRUE) {
            wifi_prov_mgr_config_t config;

            switch (notification) {
                case STOP_PROVISIONING:
                    ESP_LOGI(TAG, "stopping provisioner");
                    wifi_prov_mgr_stop_provisioning();

                    break;
                case START_PROVISIONING:
                    ESP_LOGI(TAG, "starting provisioner");

                    config = {.scheme = wifi_prov_scheme_ble, .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BT};

                    wifi_prov_mgr_init(config);

                    if (wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_0, NULL, "TeslaCANalyzer", NULL) != ESP_OK) {
                        if (xBLEHelperTask != NULL) {
                            ESP_LOGW(TAG, "init failed, taking BLE from nimble");
                            xTaskNotify(xBLEHelperTask, BLEHelperTaskNotification::NIMBLE_STOP, eSetValueWithOverwrite);
                            if (xSemaphoreTake(xBLESemaphore, portMAX_DELAY) == pdPASS) {
                                ESP_LOGI(TAG, "attempting to retry");
                                if (wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_0, NULL, "TeslaCANalyzer", NULL) != ESP_OK) {
                                    esp_restart();
                                }
                            }
                        }
                    }
                    break;
                case RESET_PROVISIONING:
                    ESP_LOGW(TAG, "reset");
                    ESP_ERROR_CHECK(wifi_prov_mgr_reset_provisioning());
                    esp_restart();
                    break;
                case RESET_SM_ON_FAILURE:
                    ESP_LOGI(TAG, "reset sm state on failure");
                    wifi_prov_mgr_reset_sm_state_on_failure();
                    break;
                default:
                    break;
            }
        }
    }
}

void provisioning_init() {
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &prov_event_handler, NULL));
    xTaskCreatePinnedToCore(provisioning_task, "ProvisioningTask", 4096, NULL, 5, &xProvisioningTask, 1);
}
