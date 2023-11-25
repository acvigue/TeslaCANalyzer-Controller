#include "mqtt_task.h"

#include <esp_crt_bundle.h>
#include <esp_event.h>
#include <esp_log.h>
#include <mqtt_client.h>

#define TAG "mqtt"

esp_mqtt_client_handle_t mqttClient = nullptr;

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    static char lastTopic[200];
    static char *dataBuf = nullptr;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED: {
            ESP_LOGI(TAG, "connected to broker..");
            break;
        }
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "mqtt disconnected");
            break;
        case MQTT_EVENT_DATA:
            /*
            if (event->topic_len != 0) {
                memset(lastTopic, 0, 200);
                strncpy(lastTopic, event->topic, event->topic_len);
            }

            // fill buffer
            if (event->current_data_offset == 0) {
                ESP_LOGD(MQTT_TAG, "creating message buffer for topic %s (size %d)", lastTopic, event->total_data_len);
                dataBuf = (char *)heap_caps_malloc(event->total_data_len, MALLOC_CAP_SPIRAM);
            }

            memcpy((void *)(dataBuf + event->current_data_offset), event->data, event->data_len);
            if (event->data_len + event->current_data_offset >= event->total_data_len) {
                char *messagePtr = dataBuf;

                mqttMessage newMessageItem;
                newMessageItem.messageLen = event->total_data_len;
                newMessageItem.pMessage = messagePtr;
                strcpy(newMessageItem.topic, lastTopic);
                if (xQueueSend(xMqttMessageQueue, &(newMessageItem), pdMS_TO_TICKS(1000)) != pdTRUE) {
                    ESP_LOGE(MQTT_TAG, "couldn't post message for topic %s (len %d) to queue!", lastTopic, event->total_data_len);
                }
            }
            */
            break;
        default:
            break;
    }
}

void mqtt_task(void *pvParameter) {
    ESP_LOGI(TAG, "task started");
    while (true) {
        MQTTTaskNotification notification;
        if (xTaskNotifyWait(0, ULONG_MAX, (uint32_t *)&notification, pdMS_TO_TICKS(50)) == pdPASS) {
            switch (notification) {
                case MQTT_START: {
                    const esp_mqtt_client_config_t mqtt_cfg = {
                        .broker = {.address = "mqtts://mqtt.vigue.me:443", .verification = {.crt_bundle_attach = esp_crt_bundle_attach}},
                        .credentials = {.username = "smartmatrix", .authentication = {.password = "Kaj3neeUvoHIu28g98BpcT5RohUrAXrr"}},
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
    }
}

void mqtt_init() { xTaskCreatePinnedToCore(mqtt_task, "MQTTTask", 4096, NULL, 5, &xMQTTTask, 1); }