/**
 * @file command_parser.c
 * @brief Command parser implementation
 */

#include "command_parser.h"
#include "esp_log.h"

static const char *TAG = "command_parser";

esp_err_t command_parser_init(void)
{
    ESP_LOGI(TAG, "Command parser initialized");
    return ESP_OK;
}

