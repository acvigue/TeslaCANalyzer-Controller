#include "mqtt_task.h"

#include <esp_crt_bundle.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <mqtt_client.h>

#include "radar_task.h"
#include "secrets.h"

#define TAG "mqtt"

esp_mqtt_client_handle_t mqttClient = nullptr;

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    static char lastTopic[200];
    static uint8_t *dataBuf = nullptr;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED: {
            show_message("MQTT ok", 7);
            ESP_LOGI(TAG, "connected to broker..");

            char device_id[13];
            uint8_t eth_mac[6];
            esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
            snprintf(device_id, 13, "%02X%02X%02X%02X%02X%02X", eth_mac[0], eth_mac[1], eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5]);
            char tmpTopic[200];

            sprintf(tmpTopic, "teslacan/%s/live_alerts", device_id);
            esp_mqtt_client_subscribe(event->client, tmpTopic, 1);
            ESP_LOGI(TAG, "subscribed to %s", tmpTopic);
            break;
        }
        case MQTT_EVENT_DISCONNECTED:
            show_message("MQTT disc!", 10);
            ESP_LOGI(TAG, "mqtt disconnected");
            break;
        case MQTT_EVENT_DATA:
            if (event->topic_len != 0) {
                memset(lastTopic, 0, 200);
                strncpy(lastTopic, event->topic, event->topic_len);
            }

            // fill buffer
            if (event->current_data_offset == 0) {
                ESP_LOGI(TAG, "creating message buffer for topic %s (size %d)", lastTopic, event->total_data_len);
                dataBuf = (uint8_t *)malloc(event->total_data_len);
            }

            memcpy((void *)(dataBuf + event->current_data_offset), event->data, event->data_len);
            if (event->data_len + event->current_data_offset >= event->total_data_len) {
                uint8_t *messagePtr = dataBuf;

                MQTTMessage messageItem;
                messageItem.data_len = event->total_data_len;
                messageItem.data = messagePtr;
                strcpy(messageItem.topic, lastTopic);
                if (xQueueSend(xMQTTInboxQueue, &(messageItem), pdMS_TO_TICKS(1000)) != pdTRUE) {
                    ESP_LOGE(TAG, "couldn't post message for topic %s (len %d) to queue!", lastTopic, event->total_data_len);
                }
            }
            break;
        default:
            break;
    }
}

QueueHandle_t getMQTTInbox() { return xMQTTInboxQueue; }
QueueHandle_t getMQTTOutbox() { return xMQTTOutboxQueue; }

void mqtt_task(void *pvParameter) {
    ESP_LOGI(TAG, "task started");

    xMQTTInboxQueue = xQueueCreate(10, sizeof(MQTTMessage));
    xMQTTOutboxQueue = xQueueCreate(10, sizeof(MQTTMessage));

    while (true) {
        MQTTTaskNotification notification;
        if (xTaskNotifyWait(0, ULONG_MAX, (uint32_t *)&notification, pdMS_TO_TICKS(10)) == pdPASS) {
            switch (notification) {
                case MQTT_START: {
                    const esp_mqtt_client_config_t mqtt_cfg = {
                        .broker = {.address = MQTT_HOST, .verification = {.crt_bundle_attach = esp_crt_bundle_attach}},
                        .credentials = {.username = MQTT_USERNAME, .authentication = {.password = MQTT_PASSWORD}},
                        .network =
                            {
                                .reconnect_timeout_ms = 2500,
                                .timeout_ms = 10000,
                            },
                        .buffer = {
                            .size = 4096,
                        }};

                    mqttClient = esp_mqtt_client_init(&mqtt_cfg);

                    esp_mqtt_client_register_event(mqttClient, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
                    esp_mqtt_client_start(mqttClient);
                    break;
                }
                case MQTT_STOP:
                    esp_mqtt_client_stop(mqttClient);
                    break;
            }
        }
        MQTTMessage incomingMessage;
        if (xQueueReceive(xMQTTInboxQueue, &incomingMessage, pdMS_TO_TICKS(10)) == pdTRUE) {
            ESP_LOGI(TAG, "task handling new message!");
        }
    }
}

void mqtt_init() { xTaskCreatePinnedToCore(mqtt_task, "MQTTTask", 4096, NULL, 5, &xMQTTTask, 1); }