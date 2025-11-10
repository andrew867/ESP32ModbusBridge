/**
 * @file param_manager.c
 * @brief Parameter management implementation
 */

#include "param_manager.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

static const char *TAG = "param_manager";
static const char *NVS_NAMESPACE = "device_param";
static nvs_handle_t s_nvs_handle = 0;
static bool s_initialized = false;

// Parameter metadata: type, key name, default values
typedef struct {
    param_type_t type;
    const char *key;
    const char *default_string;
    int32_t default_int;
    int32_t min_int;
    int32_t max_int;
    size_t max_string_len;
} param_metadata_t;

// Parameter metadata table
static const param_metadata_t s_param_metadata[PARAM_ID_MAX] = {
    [PARAM_ID_0] = {PARAM_TYPE_INT, "param_0", NULL, 0, 0, 0, 0},
    [PARAM_ID_1] = {PARAM_TYPE_INT, "param_1", NULL, 0, 0, 0, 0},
    [PARAM_ID_2] = {PARAM_TYPE_STRING, "wifi_ssid", "LuxPower", 0, 0, 0, 64},
    [PARAM_ID_3] = {PARAM_TYPE_STRING, "wifi_password", "", 0, 0, 0, 64},
    [PARAM_ID_4] = {PARAM_TYPE_INT, "param_4", NULL, 0, 0, 0, 0},
    [PARAM_ID_5] = {PARAM_TYPE_STRING, "server_host", "dongle_ssl.solarcloudsystem.com", 0, 0, 0, 128},
    [PARAM_ID_6] = {PARAM_TYPE_STRING, "server_port", "4348", 0, 0, 0, 16},
    [PARAM_ID_7] = {PARAM_TYPE_STRING, "device_sn", "", 0, 0, 0, 64},
    [PARAM_ID_8] = {PARAM_TYPE_INT, "query_period", NULL, 5000, 1000, 60000, 0}, // 1-60 seconds
    [PARAM_ID_9] = {PARAM_TYPE_STRING, "device_id", "LuxWiFiDongle", 0, 0, 0, 64},
    [PARAM_ID_10] = {PARAM_TYPE_INT, "factory_test", NULL, 0, 0, 1, 0}, // 0 or 1
    [PARAM_ID_11] = {PARAM_TYPE_INT, "param_11", NULL, 0, 0, 0, 0},
    [PARAM_ID_12] = {PARAM_TYPE_INT, "param_12", NULL, 0, 0, 0, 0},
    [PARAM_ID_13] = {PARAM_TYPE_INT, "param_13", NULL, 0, 0, 0, 0},
    [PARAM_ID_14] = {PARAM_TYPE_INT, "ip_config", NULL, 0, 0, 1, 0}, // 0=DHCP, 1=Static
    [PARAM_ID_15] = {PARAM_TYPE_INT, "param_15", NULL, 0, 0, 0, 0},
};

/**
 * @brief Initialize parameter manager
 */
esp_err_t param_manager_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    // Open NVS namespace
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &s_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(ret));
        return ret;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Parameter manager initialized");
    return ESP_OK;
}

/**
 * @brief Set a string parameter
 */
esp_err_t param_set_string(param_id_t id, const char *value)
{
    if (id >= PARAM_ID_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_initialized) {
        ESP_LOGE(TAG, "Parameter manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    const param_metadata_t *meta = &s_param_metadata[id];
    if (meta->type != PARAM_TYPE_STRING) {
        ESP_LOGE(TAG, "Parameter %d is not a string type", id);
        return ESP_ERR_INVALID_ARG;
    }

    if (value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Validate string length
    size_t len = strlen(value);
    if (len > meta->max_string_len) {
        ESP_LOGE(TAG, "String too long for parameter %d (max %zu)", id, meta->max_string_len);
        return ESP_ERR_INVALID_ARG;
    }

    // Store in NVS
    esp_err_t ret = nvs_set_str(s_nvs_handle, meta->key, value);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set parameter %d: %s", id, esp_err_to_name(ret));
        return ret;
    }

    // Commit changes
    ret = nvs_commit(s_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit parameter %d: %s", id, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Parameter %d (%s) set to: %s", id, meta->key, value);
    return ESP_OK;
}

/**
 * @brief Set an integer parameter
 */
esp_err_t param_set_int(param_id_t id, int32_t value)
{
    if (id >= PARAM_ID_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_initialized) {
        ESP_LOGE(TAG, "Parameter manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    const param_metadata_t *meta = &s_param_metadata[id];
    if (meta->type != PARAM_TYPE_INT) {
        ESP_LOGE(TAG, "Parameter %d is not an integer type", id);
        return ESP_ERR_INVALID_ARG;
    }

    // Validate range
    if (meta->min_int != meta->max_int) { // Only validate if range is set
        if (value < meta->min_int || value > meta->max_int) {
            ESP_LOGE(TAG, "Value %ld out of range for parameter %d (%ld-%ld)", 
                     value, id, meta->min_int, meta->max_int);
            return ESP_ERR_INVALID_ARG;
        }
    }

    // Store in NVS
    esp_err_t ret = nvs_set_i32(s_nvs_handle, meta->key, value);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set parameter %d: %s", id, esp_err_to_name(ret));
        return ret;
    }

    // Commit changes
    ret = nvs_commit(s_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit parameter %d: %s", id, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Parameter %d (%s) set to: %ld", id, meta->key, value);
    return ESP_OK;
}

/**
 * @brief Get a string parameter
 */
esp_err_t param_get_string(param_id_t id, char *value, size_t max_len)
{
    if (id >= PARAM_ID_MAX || value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_initialized) {
        ESP_LOGE(TAG, "Parameter manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    const param_metadata_t *meta = &s_param_metadata[id];
    if (meta->type != PARAM_TYPE_STRING) {
        ESP_LOGE(TAG, "Parameter %d is not a string type", id);
        return ESP_ERR_INVALID_ARG;
    }

    // Try to read from NVS
    size_t required_size = max_len;
    esp_err_t ret = nvs_get_str(s_nvs_handle, meta->key, value, &required_size);
    
    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "Parameter %d (%s) = %s", id, meta->key, value);
        return ESP_OK;
    } else if (ret == ESP_ERR_NVS_NOT_FOUND) {
        // Return default value
        if (meta->default_string != NULL) {
            strncpy(value, meta->default_string, max_len - 1);
            value[max_len - 1] = '\0';
            ESP_LOGI(TAG, "Parameter %d (%s) not found, using default: %s", id, meta->key, value);
            return ESP_ERR_NOT_FOUND; // Indicate default was used
        } else {
            value[0] = '\0';
            return ESP_ERR_NOT_FOUND;
        }
    } else {
        ESP_LOGE(TAG, "Failed to get parameter %d: %s", id, esp_err_to_name(ret));
        return ret;
    }
}

/**
 * @brief Get an integer parameter
 */
esp_err_t param_get_int(param_id_t id, int32_t *value)
{
    if (id >= PARAM_ID_MAX || value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_initialized) {
        ESP_LOGE(TAG, "Parameter manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    const param_metadata_t *meta = &s_param_metadata[id];
    if (meta->type != PARAM_TYPE_INT) {
        ESP_LOGE(TAG, "Parameter %d is not an integer type", id);
        return ESP_ERR_INVALID_ARG;
    }

    // Try to read from NVS
    esp_err_t ret = nvs_get_i32(s_nvs_handle, meta->key, value);
    
    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "Parameter %d (%s) = %ld", id, meta->key, *value);
        return ESP_OK;
    } else if (ret == ESP_ERR_NVS_NOT_FOUND) {
        // Return default value
        *value = meta->default_int;
        ESP_LOGI(TAG, "Parameter %d (%s) not found, using default: %ld", id, meta->key, *value);
        return ESP_ERR_NOT_FOUND; // Indicate default was used
    } else {
        ESP_LOGE(TAG, "Failed to get parameter %d: %s", id, esp_err_to_name(ret));
        return ret;
    }
}

/**
 * @brief Get parameter type
 */
esp_err_t param_get_type(param_id_t id, param_type_t *type)
{
    if (id >= PARAM_ID_MAX || type == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *type = s_param_metadata[id].type;
    return ESP_OK;
}

/**
 * @brief Commit parameter changes
 */
esp_err_t param_commit(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    return nvs_commit(s_nvs_handle);
}

/**
 * @brief Reset parameter to default value
 */
esp_err_t param_reset(param_id_t id)
{
    if (id >= PARAM_ID_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    const param_metadata_t *meta = &s_param_metadata[id];
    esp_err_t ret;

    if (meta->type == PARAM_TYPE_STRING) {
        if (meta->default_string != NULL) {
            ret = param_set_string(id, meta->default_string);
        } else {
            // Erase the parameter
            ret = nvs_erase_key(s_nvs_handle, meta->key);
            if (ret == ESP_OK) {
                ret = nvs_commit(s_nvs_handle);
            }
        }
    } else {
        ret = param_set_int(id, meta->default_int);
    }

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Parameter %d reset to default", id);
    }

    return ret;
}

/**
 * @brief Reset all parameters to defaults
 */
esp_err_t param_reset_all(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Resetting all parameters to defaults");

    for (param_id_t id = 0; id < PARAM_ID_MAX; id++) {
        param_reset(id);
    }

    return ESP_OK;
}

