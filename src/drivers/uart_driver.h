/**
 * @file uart_driver.h
 * @brief UART driver abstraction
 */

#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t uart_driver_init(void);

#ifdef __cplusplus
}
#endif

#endif // UART_DRIVER_H

