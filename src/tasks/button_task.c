/**
 * @file button_task.c
 * @brief Button task implementation
 * 
 * Original: sub_42012FDE (button_task, priority 6)
 * 
 * Handles button presses on GPIO 13:
 * - Short press (>100ms): Factory test mode
 * - Medium press (>500ms): Factory reset with defaults
 * - Long press (>1000ms): System reboot
 */

#include "button_task.h"
#include "../config/param_manager.h"
#include "../config/param_ids.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "button_task";

#define BUTTON_GPIO           13
#define BUTTON_PRESS_TIME_MS  100   // 100ms for factory test
#define BUTTON_RESET_TIME_MS  500   // 500ms for factory reset
#define BUTTON_REBOOT_TIME_MS 1000  // 1000ms for reboot
#define BUTTON_POLL_INTERVAL_MS 5   // Poll every 5ms

// Factory default values
#define FACTORY_WIFI_SSID     "luxpower"
#define FACTORY_WIFI_PASSWORD "12345678"
#define FACTORY_SERVER_HOST   "dongle_ssl.solarcloudsystem.com"
#define FACTORY_SERVER_PORT   4348
#define FACTORY_TEST_FLAG     0

/**
 * @brief Perform factory reset
 * 
 * Original: sub_42012FDE factory reset logic
 * Sets all parameters to factory defaults
 */
static void button_factory_reset(void)
{
    ESP_LOGI(TAG, "Performing factory reset...");

    // Set factory default parameters
    param_set_string(PARAM_ID_2, FACTORY_WIFI_SSID);
    param_set_string(PARAM_ID_3, FACTORY_WIFI_PASSWORD);
    param_set_string(PARAM_ID_5, FACTORY_SERVER_HOST);
    
    int32_t port = FACTORY_SERVER_PORT;
    param_set_int(PARAM_ID_6, port);
    
    int32_t factory_test = FACTORY_TEST_FLAG;
    param_set_int(PARAM_ID_10, factory_test);

    ESP_LOGI(TAG, "Factory reset complete");
}

/**
 * @brief Perform factory test
 * 
 * Original: sub_4200F518 (factory_test)
 */
static void button_factory_test(void)
{
    ESP_LOGI(TAG, "Entering factory test mode...");
    
    // Set factory test flag
    int32_t factory_test = 1;
    param_set_int(PARAM_ID_10, factory_test);
    
    ESP_LOGI(TAG, "Factory test mode enabled");
}

/**
 * @brief Button task
 * 
 * Original: sub_42012FDE
 * Monitors button state and handles press duration
 */
static void button_task(void *pvParameters)
{
    uint32_t press_start_time = 0;
    bool button_pressed = false;
    bool last_button_state = false;

    ESP_LOGI(TAG, "Button task started (GPIO %d)", BUTTON_GPIO);

    while (1) {
        // Read button state (active low)
        bool current_button_state = (gpio_get_level(BUTTON_GPIO) == 0);

        if (current_button_state && !last_button_state) {
            // Button just pressed
            press_start_time = xTaskGetTickCount();
            button_pressed = true;
            ESP_LOGD(TAG, "Button pressed");
        } else if (!current_button_state && last_button_state) {
            // Button just released
            if (button_pressed) {
                uint32_t press_duration = xTaskGetTickCount() - press_start_time;
                uint32_t press_duration_ms = press_duration * portTICK_PERIOD_MS;

                ESP_LOGI(TAG, "Button released after %lu ms", press_duration_ms);

                if (press_duration_ms >= BUTTON_REBOOT_TIME_MS) {
                    // Long press: Reboot
                    ESP_LOGI(TAG, "Long press detected: Rebooting system...");
                    vTaskDelay(pdMS_TO_TICKS(100));
                    esp_restart();
                } else if (press_duration_ms >= BUTTON_RESET_TIME_MS) {
                    // Medium press: Factory reset
                    ESP_LOGI(TAG, "Medium press detected: Factory reset");
                    button_factory_reset();
                    vTaskDelay(pdMS_TO_TICKS(100));
                    esp_restart();
                } else if (press_duration_ms >= BUTTON_PRESS_TIME_MS) {
                    // Short press: Factory test
                    ESP_LOGI(TAG, "Short press detected: Factory test mode");
                    button_factory_test();
                }
            }
            button_pressed = false;
        }

        last_button_state = current_button_state;
        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_INTERVAL_MS));
    }
}

/**
 * @brief Initialize button task
 * 
 * Original: sub_420132C2 (button_init)
 */
esp_err_t button_task_init(void)
{
    // Configure GPIO as input with pull-up
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .pull_down_en = 0,
        .pull_up_en = 1,  // Enable pull-up (button is active low)
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Create button task (priority 6)
    BaseType_t ret = xTaskCreate(button_task, "button_task", 2048, NULL, 6, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Button task initialized (GPIO %d)", BUTTON_GPIO);
    return ESP_OK;
}

