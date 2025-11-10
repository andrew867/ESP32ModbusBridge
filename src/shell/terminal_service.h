/**
 * @file terminal_service.h
 * @brief UART terminal service
 * 
 * Original: sub_4200D91E (terminal_service_init), sub_42012BB0 (command_handler)
 * 
 * Command format:
 * - LPTS1: Set device SN (param 9)
 * - LPTS3: Set router/server (param 5, 6)
 * - LPTS4: Set query period (param 8)
 * - LPTS5: Clear connection flag (param 10)
 * - LPTS7: Set device SN (param 7, 8)
 * - LPTQ1: Query device SN (param 9)
 * - LPTQ2: Query router/server (param 5, 6)
 * - LPTQ3: Query firmware version
 * - LPTQ4: Query query period (param 8)
 * - LPTQ6: Query connection results
 * - LPTQ7: Query server (param 7, 8)
 * - SHELL: Enable shell mode
 */

#ifndef TERMINAL_SERVICE_H
#define TERMINAL_SERVICE_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize terminal service
 * 
 * Sets up UART terminal command parser and handlers.
 * 
 * @return ESP_OK on success
 */
esp_err_t terminal_service_init(void);

#ifdef __cplusplus
}
#endif

#endif // TERMINAL_SERVICE_H

