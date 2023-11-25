#ifndef H_INTERFACE_TASK
#define H_INTERFACE_TASK

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

inline QueueHandle_t xInterfaceInboxQueue;
inline QueueHandle_t xInterfaceOutboxQueue;
inline TaskHandle_t xInterfaceTask = NULL;

void interface_task(void *pvParameter);

#endif