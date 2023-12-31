#include "wifi_task.h"

#include <esp_err.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

#include "mqtt_task.h"
#include "provisioning_task.h"
#include "radar_task.h"

char hostname[22];

#define TAG "wifi"

void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    static int wifiConnectionAttempts = 0;
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START: {
                ESP_LOGI(TAG, "STA started");

                /* check if device has been provisioned */
                wifi_config_t wifi_cfg;
                esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg);
                if (strlen((const char *)wifi_cfg.sta.ssid)) {
                    esp_wifi_connect();
                } else {
                    xTaskNotify(xProvisioningTask, ProvisioningTaskNotification_t::START_PROVISIONING, eSetValueWithOverwrite);
                }
                break;
            }
            case WIFI_EVENT_STA_DISCONNECTED: {
                wifiConnectionAttempts++;
                show_message("WiFi disc!", 10);
                ESP_LOGI(TAG, "STA disconnected");

                if (xMQTTTask != NULL) {
                    xTaskNotify(xMQTTTask, MQTTTaskNotification::MQTT_STOP, eSetValueWithOverwrite);
                }

                if (wifiConnectionAttempts > 5) {
                    xTaskNotify(xProvisioningTask, ProvisioningTaskNotification_t::START_PROVISIONING, eSetValueWithOverwrite);
                }
                ESP_LOGI(TAG, "STA reconnecting..");
                esp_wifi_connect();
                break;
            }
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            wifiConnectionAttempts = 0;
            ESP_LOGI(TAG, "STA connected!");
            show_message("WiFi OK", 7);

            if (xMQTTTask != NULL) {
                xTaskNotify(xMQTTTask, MQTTTaskNotification::MQTT_START, eSetValueWithOverwrite);
            }
        }
    }
}

void wifi_init() {
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(wifi_mode_t::WIFI_MODE_STA));

    char device_id[13];
    uint8_t eth_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(device_id, 13, "%02X%02X%02X%02X%02X%02X", eth_mac[0], eth_mac[1], eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5]);
    snprintf(hostname, 22, "TeslaCAN-%s", device_id);
    esp_netif_set_hostname(netif, hostname);
    ESP_ERROR_CHECK(esp_wifi_start());
}