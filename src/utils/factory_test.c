/**
 * @file factory_test.c
 * @brief Factory test implementation
 * 
 * Original: sub_4200F518 (factory_test)
 * 
 * Provides factory test mode functionality.
 */

#include "factory_test.h"
#include "../config/param_manager.h"
#include "../config/param_ids.h"
#include "esp_log.h"
#include <stdbool.h>

static const char *TAG = "factory_test";

/**
 * @brief Enable factory test mode
 * 
 * Original: sub_4200F518 (factory_test)
 */
esp_err_t factory_test_enable(void)
{
    ESP_LOGI(TAG, "Enabling factory test mode");
    
    int32_t factory_test = 1;
    esp_err_t ret = param_set_int(PARAM_ID_10, factory_test);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set factory test flag: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Factory test mode enabled");
    return ESP_OK;
}

/**
 * @brief Disable factory test mode
 */
esp_err_t factory_test_disable(void)
{
    ESP_LOGI(TAG, "Disabling factory test mode");
    
    int32_t factory_test = 0;
    esp_err_t ret = param_set_int(PARAM_ID_10, factory_test);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to clear factory test flag: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Factory test mode disabled");
    return ESP_OK;
}

/**
 * @brief Check if factory test mode is enabled
 */
bool factory_test_is_enabled(void)
{
    int32_t factory_test = 0;
    esp_err_t ret = param_get_int(PARAM_ID_10, &factory_test);
    if (ret != ESP_OK) {
        return false;
    }
    return (factory_test != 0);
}

/**
 * @brief Initialize factory test
 */
esp_err_t factory_test_init(void)
{
    ESP_LOGI(TAG, "Factory test initialized");
    
    // Check if factory test mode is enabled
    if (factory_test_is_enabled()) {
        ESP_LOGI(TAG, "Factory test mode is currently enabled");
    }
    
    return ESP_OK;
}

