/**
 * @file button_task.h
 * @brief Button handling task
 * 
 * Original: sub_42012FDE (ButtonTask, priority 6)
 */

#ifndef BUTTON_TASK_H
#define BUTTON_TASK_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t button_task_init(void);

#ifdef __cplusplus
}
#endif

#endif // BUTTON_TASK_H

