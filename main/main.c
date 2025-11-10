/**
 * @file main.c
 * @brief Main application entry point
 * 
 * Original: sub_4200D710 (app_main)
 * 
 * Initialization sequence:
 * 1. NVS flash initialization/erase check
 * 2. System initialization
 * 3. Parameter initialization
 * 4. Factory test check
 * 5. WiFi initialization
 * 6. Terminal service initialization
 * 7. LED/Button tasks initialization
 * 8. Parameter validation (check param ID 10)
 * 9. TCP server initialization
 * 10. Data processing module initialization
 * 11. OTA initialization
 * 12. TCP client initialization
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "../src/system/sdk_init.h"
#include "../src/config/param_manager.h"
#include "../src/config/param_ids.h"
#include "../src/tasks/wifi_task.h"
#include "../src/shell/terminal_service.h"
#include "../src/tasks/led_task.h"
#include "../src/tasks/button_task.h"
#include "../src/tasks/tcp_server_task.h"
#include "../src/protocol/data_process.h"
#include <stdlib.h>
#include "../src/ota/ota_manager.h"
#include "../src/tasks/tcp_client_task.h"
#include "../src/tasks/rs485_task.h"
#include "../src/tasks/uart_rx_task.h"
#include "../src/tasks/ble_task.h"
#include "../src/utils/heartbeat.h"
#include "../src/utils/poll_timer.h"
#include "../src/utils/factory_test.h"
#include "../src/protocol/modbus_protocol.h"

static const char *TAG = "main";

// Global data handle for RS485-TCP routing
static data_process_handle_t s_rs485_tcp_data_handle = NULL;

// Forward declarations
static void rs485_frame_to_tcp_callback(uint8_t *frame, size_t len);
static void rs485_tcp_send_wrapper(const uint8_t *data, size_t len);

void app_main(void)
{
    ESP_LOGI(TAG, "Starting LuxWiFiDongle application...");

    // 1. Initialize SDK components
    ESP_ERROR_CHECK(sdk_init());

    // 2. Initialize parameter manager
    ESP_ERROR_CHECK(param_manager_init());

    // 3. Check factory test flag (param ID 10)
    int32_t factory_test = 0;
    esp_err_t ret = param_get_int(PARAM_ID_10, &factory_test);
    if (ret == ESP_ERR_NOT_FOUND) {
        factory_test = 0; // Default to normal mode
    }
    
    if (factory_test == 1) {
        ESP_LOGI(TAG, "Factory test mode enabled");
        // Factory test mode is handled by LED task and other modules
        // The factory_test_init() call below will check and log the status
    }

    // 4. Initialize WiFi
    ESP_ERROR_CHECK(wifi_task_init());

    // 5. Initialize terminal service
    ESP_ERROR_CHECK(terminal_service_init());

    // 6. Initialize LED and Button tasks
    ESP_ERROR_CHECK(led_task_init());
    ESP_ERROR_CHECK(button_task_init());

    // 7. Initialize UART RX task
    ESP_ERROR_CHECK(uart_rx_task_init());

    // 8. Initialize BLE task
    ESP_ERROR_CHECK(ble_task_init());

    // 9. Initialize TCP server
    ESP_ERROR_CHECK(tcp_server_task_init());

    // 10. Initialize factory test utilities
    ESP_ERROR_CHECK(factory_test_init());

    // 11. Initialize heartbeat and poll timer
    ESP_ERROR_CHECK(heartbeat_init());
    ESP_ERROR_CHECK(poll_timer_init());

    // 12. Initialize data processing module for RS485-TCP routing
    // Create a data handle that forwards to TCP client
    // We'll create a wrapper callback since tcp_client_task_send has different signature
    s_rs485_tcp_data_handle = data_process_create(
        rs485_tcp_send_wrapper,  // Send callback wrapper
        NULL  // Receive callback not needed for this routing
    );
    if (s_rs485_tcp_data_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create RS485-TCP data process handle");
    }
    
    // Global data handle for heartbeat
    data_process_handle_t data_handle = s_rs485_tcp_data_handle;

    // 13. Initialize OTA manager
    ESP_ERROR_CHECK(ota_manager_init());

    // 14. Initialize TCP client (last, as it depends on WiFi)
    ESP_ERROR_CHECK(tcp_client_task_init());

    // 15. Initialize RS485 task
    ESP_ERROR_CHECK(rs485_task_init());

    // 16. Set up data routing between RS485 and TCP client
    // RS485 frames will be forwarded to TCP client via data processing
    rs485_task_set_callback(rs485_frame_to_tcp_callback);

    // 17. Start heartbeat for TCP client connection
    if (data_handle != NULL) {
        heartbeat_start(data_handle);
    }

    ESP_LOGI(TAG, "Application initialization complete");
}

/**
 * @brief Callback to forward RS485 frames to TCP client
 * 
 * Converts Modbus frames to protocol frames and sends via TCP client
 * 
 * Original: Data routing from RS485 to TCP (sub_42011F22)
 */
static void rs485_frame_to_tcp_callback(uint8_t *frame, size_t len)
{
    if (frame == NULL || len == 0) {
        return;
    }

    ESP_LOGD(TAG, "RS485 frame received: %zu bytes, forwarding to TCP", len);
    
    // Check if TCP client is connected
    if (!tcp_client_task_is_connected()) {
        ESP_LOGD(TAG, "TCP client not connected, skipping RS485 frame");
        return;
    }

    // Extract function code from Modbus frame
    if (len < 2) {
        return;
    }

    uint8_t modbus_addr = frame[0];
    uint8_t modbus_func = frame[1];

    // Convert Modbus frame to protocol data transmission frame (function code 194)
    if (len > 4) {  // At least addr, func, and 2 CRC bytes
        // Extract data (skip addr and func, remove CRC)
        size_t data_len = len - 4;  // Remove addr, func, and 2 CRC bytes
        if (data_len > 0 && data_len < 500) {
            uint8_t *modbus_data = frame + 2;  // Skip addr and func
            
            // Send as data transmission frame via TCP client's data handle
            if (s_rs485_tcp_data_handle != NULL) {
                esp_err_t ret = data_process_send(s_rs485_tcp_data_handle, 
                                                  PROTOCOL_FC_DATA_TRANSMISSION,
                                                  modbus_data, 
                                                  data_len);
                if (ret != ESP_OK) {
                    ESP_LOGW(TAG, "Failed to forward RS485 data to TCP: %d", ret);
                } else {
                    ESP_LOGD(TAG, "Forwarded RS485 data to TCP: %zu bytes", data_len);
                }
            } else {
                // Fallback: send raw data via TCP client
                esp_err_t send_ret = tcp_client_task_send(modbus_data, data_len);
                if (send_ret != ESP_OK) {
                    ESP_LOGW(TAG, "Failed to send raw data to TCP client: %d", send_ret);
                }
            }
        }
    }
}

/**
 * @brief Wrapper to send data to TCP client from data processing module
 */
static void rs485_tcp_send_wrapper(const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return;
    }
    
    // Forward to TCP client
    esp_err_t ret = tcp_client_task_send(data, len);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send data to TCP client: %d", ret);
    }
}

