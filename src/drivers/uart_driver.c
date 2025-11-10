/**
 * @file uart_driver.c
 * @brief UART driver implementation
 */

#include "uart_driver.h"
#include "esp_log.h"

static const char *TAG = "uart_driver";

esp_err_t uart_driver_init(void)
{
    ESP_LOGI(TAG, "UART driver initialized");
    return ESP_OK;
}

