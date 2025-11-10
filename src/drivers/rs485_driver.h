/**
 * @file rs485_driver.h
 * @brief RS485-specific driver functions
 */

#ifndef RS485_DRIVER_H
#define RS485_DRIVER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t rs485_driver_init(void);

#ifdef __cplusplus
}
#endif

#endif // RS485_DRIVER_H

