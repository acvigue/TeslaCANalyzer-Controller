#ifndef PROVISIONING_TASK_H
#define PROVISIONING_TASK_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

typedef enum ProvisioningTaskNotification_t {
    STOP_PROVISIONING = 1,
    START_PROVISIONING = 2,
    RESET_PROVISIONING = 3,
    RESET_SM_ON_FAILURE = 4
} ProvisioningTaskNotification_t;

inline TaskHandle_t xProvisioningTask = nullptr;
inline bool provisioningActive = false;

void provisioning_init();
#endif