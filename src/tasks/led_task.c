/**
 * @file led_task.c
 * @brief LED task implementation
 * 
 * Original: sub_420129D8 (led_task)
 * 
 * Controls 3 LEDs (GPIO 12, 14, 15) to indicate system status:
 * - WiFi connection status
 * - System state
 * - Error conditions
 */

#include "led_task.h"
#include "../tasks/wifi_task.h"
#include "../utils/factory_test.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>

static const char *TAG = "led_task";

#define LED_GPIO_1    12
#define LED_GPIO_2    14
#define LED_GPIO_3    15

// LED state machine
typedef enum {
    LED_STATE_0 = 0,
    LED_STATE_1,
    LED_STATE_2,
    LED_STATE_3,
    LED_STATE_4
} led_state_t;

static led_state_t s_led_state = LED_STATE_0;
static int s_led_error_count = 0;

/**
 * @brief Set LED GPIO state
 * 
 * Original: sub_42012996, sub_420129C2, sub_420129AC
 */
static void led_set_gpio(int gpio, bool state)
{
    gpio_set_level(gpio, state ? 0 : 1);  // Inverted logic (0 = ON, 1 = OFF)
}

/**
 * @brief LED task
 * 
 * Original: sub_420129D8
 * Controls LEDs based on system state and WiFi connection
 */
static void led_task(void *pvParameters)
{
    bool led1 = false;
    bool led2 = false;
    bool led3 = false;
    bool wifi_connected = false;
    bool factory_mode = factory_test_is_enabled();

    ESP_LOGI(TAG, "LED task started");

    while (1) {
        // Check WiFi connection status
        wifi_connected = wifi_task_is_connected();

        // Check factory mode (from parameter)
        factory_mode = factory_test_is_enabled();

        if (factory_mode) {
            // Factory mode: specific LED patterns
            if (s_led_state == LED_STATE_0) {
                s_led_state = LED_STATE_1;
                // Initialize factory mode indicator
            }

            // Factory mode state machine
            switch (s_led_state) {
                case LED_STATE_1:
                    led1 = !led1;
                    led2 = false;
                    led3 = false;
                    s_led_state = LED_STATE_2;
                    break;

                case LED_STATE_2:
                    led1 = !led1;
                    led2 = !led2;
                    led3 = false;
                    s_led_state = LED_STATE_3;
                    break;

                case LED_STATE_3:
                    led1 = !led1;
                    led2 = !led2;
                    led3 = !led3;
                    s_led_state = LED_STATE_4;
                    break;

                case LED_STATE_4:
                    led1 = !led1;
                    led2 = !led2;
                    led3 = !led3;
                    s_led_state = LED_STATE_1;
                    break;

                default:
                    s_led_state = LED_STATE_0;
                    break;
            }

            // Error handling
            if (s_led_error_count > 4) {
                // System reboot on repeated errors
                ESP_LOGE(TAG, "Too many LED errors, rebooting...");
                esp_restart();
            }
        } else {
            // Normal mode: WiFi status indication
            s_led_error_count = 0;
            s_led_state = LED_STATE_0;

            if (wifi_connected) {
                // WiFi connected: LED1 ON
                led1 = true;
                led2 = false;
                led3 = false;
            } else {
                // WiFi disconnected: LED1 blinking
                led1 = !led1;
                led2 = false;
                led3 = false;
            }
        }

        // Update LEDs
        led_set_gpio(LED_GPIO_1, led1);
        led_set_gpio(LED_GPIO_2, led2);
        led_set_gpio(LED_GPIO_3, led3);

        // Delay based on mode
        int delay_ms = factory_mode ? 20 : 50;
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

/**
 * @brief Initialize LED task
 */
esp_err_t led_task_init(void)
{
    // Configure GPIO pins as outputs
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LED_GPIO_1) | (1ULL << LED_GPIO_2) | (1ULL << LED_GPIO_3),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Initialize LEDs to OFF
    gpio_set_level(LED_GPIO_1, 1);
    gpio_set_level(LED_GPIO_2, 1);
    gpio_set_level(LED_GPIO_3, 1);

    // Create LED task (priority 5)
    BaseType_t ret = xTaskCreate(led_task, "led_task", 2048, NULL, 5, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LED task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "LED task initialized (GPIOs: %d, %d, %d)", LED_GPIO_1, LED_GPIO_2, LED_GPIO_3);
    return ESP_OK;
}

