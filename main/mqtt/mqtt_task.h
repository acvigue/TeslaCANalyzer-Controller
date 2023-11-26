#ifndef MQTT_TASK_H
#define MQTT_TASK_H

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <mqtt_client.h>

inline TaskHandle_t xMQTTTask = nullptr;
inline QueueHandle_t xMQTTOutboxQueue = nullptr;
inline QueueHandle_t xMQTTInboxQueue = nullptr;

typedef struct MQTTMessage {
    size_t data_len;
    uint8_t *data;
    char topic[50];
} MQTTMessage;

typedef enum MQTTTaskNotification { MQTT_START, MQTT_STOP } MQTTTaskNotification;

QueueHandle_t getMQTTInbox();
QueueHandle_t getMQTTOutbox();

void mqtt_init();

#endif