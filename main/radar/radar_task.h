#ifndef H_RADAR_TASK
#define H_RADAR_TASK

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/timers.h>

inline QueueHandle_t xRadarInboxQueue;
inline QueueHandle_t xRadarOutboxQueue;
inline TaskHandle_t xRadarTask = NULL;
inline bool unlocked = false;

// types
/*
134 - police (shield)
1 - speed trap
2 - speed camera
3 - red light camera
4 -
7 - rl/speed camera
133 - laser
*/
typedef enum RadarAlertType {
    SPEED_TRAP = 1,
    SPEED_CAMERA = 2,
    RED_LIGHT_CAMERA = 3,
    RED_LIGHT_SPEED_CAMERA = 7,
    LASER = 133,
    POLICE = 134
} RadarAlertType;

QueueHandle_t getRadarInbox();
QueueHandle_t getRadarOutbox();

void clear_alert();
void send_alert(RadarAlertType alertType, uint32_t distanceFt, uint32_t heading);
void show_message(char *msg, size_t len);
void radar_task(void *pvParameter);

#endif