/**
 * @file led_task.h
 * @brief LED control task
 * 
 * Original: sub_420129D8 (led_task, priority 6)
 */

#ifndef LED_TASK_H
#define LED_TASK_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t led_task_init(void);

#ifdef __cplusplus
}
#endif

#endif // LED_TASK_H

