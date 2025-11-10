/**
 * @file poll_timer.h
 * @brief Poll timer utilities
 * 
 * Original: sub_42013BC0 (poll_timer_setup)
 */

#ifndef POLL_TIMER_H
#define POLL_TIMER_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize poll timer
 * 
 * @return ESP_OK on success
 */
esp_err_t poll_timer_init(void);

/**
 * @brief Start poll timer with callback
 * 
 * @param callback Callback function to call on each poll
 * @param period_ms Poll period in milliseconds
 * @return ESP_OK on success
 */
esp_err_t poll_timer_start(void (*callback)(void), uint32_t period_ms);

/**
 * @brief Stop poll timer
 * 
 * @return ESP_OK on success
 */
esp_err_t poll_timer_stop(void);

/**
 * @brief Set poll period
 * 
 * @param period_ms Poll period in milliseconds
 * @return ESP_OK on success
 */
esp_err_t poll_timer_set_period(uint32_t period_ms);

#ifdef __cplusplus
}
#endif

#endif // POLL_TIMER_H

