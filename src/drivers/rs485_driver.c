/**
 * @file rs485_driver.c
 * @brief RS485 driver implementation
 */

#include "rs485_driver.h"
#include "esp_log.h"

static const char *TAG = "rs485_driver";

esp_err_t rs485_driver_init(void)
{
    ESP_LOGI(TAG, "RS485 driver initialized");
    return ESP_OK;
}

