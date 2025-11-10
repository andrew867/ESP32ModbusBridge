/**
 * @file command_handlers.c
 * @brief Command handlers implementation
 */

#include "command_handlers.h"
#include "esp_log.h"

static const char *TAG = "command_handlers";

esp_err_t command_handlers_init(void)
{
    ESP_LOGI(TAG, "Command handlers initialized");
    return ESP_OK;
}

