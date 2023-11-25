#ifndef H_RADAR_TASK
#define H_RADAR_TASK

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/timers.h>

#define RADAR_TAG "radar"

inline QueueHandle_t xRadarInboxQueue;
inline QueueHandle_t xRadarOutboxQueue;
inline TaskHandle_t xRadarTask = NULL;

void radar_task(void *pvParameter);

#endif