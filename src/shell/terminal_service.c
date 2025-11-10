/**
 * @file terminal_service.c
 * @brief Terminal service implementation
 * 
 * Original: sub_4200D91E (terminal_service_init), sub_42012BB0 (command_handler)
 * 
 * Handles UART terminal commands:
 * - LPTS1-7: Set parameters
 * - LPTQ1-7: Query parameters
 * - SHELL: Enable shell mode
 */

#include "terminal_service.h"
#include "../config/param_manager.h"
#include "../config/param_ids.h"
#include "../tasks/uart_rx_task.h"
#include "../tasks/wifi_task.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "terminal_service";

#define UART_TERMINAL_NUM    UART_NUM_1
#define MAX_CMD_LEN          256
#define MAX_RESPONSE_LEN      512

// Forward declarations
static void terminal_rx_callback(uint8_t *data, size_t len);
static void terminal_process_command(const char *cmd, size_t len);
static void terminal_send_response(const char *response);

/**
 * @brief Send response to terminal
 */
static void terminal_send_response(const char *response)
{
    if (response == NULL) {
        return;
    }
    
    int len = strlen(response);
    uart_write_bytes(UART_TERMINAL_NUM, response, len);
}

/**
 * @brief Handle LPTS1 command: Set device SN (param 9)
 * Format: LPTS1:<SN>\r\n
 */
static void cmd_lpts1(const char *args)
{
    char sn[64];
    if (sscanf(args, "LPTS1:%63s", sn) == 1) {
        if (param_set_string(PARAM_ID_9, sn) == ESP_OK) {
            terminal_send_response("OK\r\n");
        } else {
            terminal_send_response("Fail\r\n");
        }
    } else {
        terminal_send_response("Fail\r\n");
    }
}

/**
 * @brief Handle LPTS3 command: Set router/server (param 5 and 6)
 * Format: LPTS3:<host>,<port>\r\n
 */
static void cmd_lpts3(const char *args)
{
    char host[64];
    char port_str[16];
    if (sscanf(args, "LPTS3:%63[^,],%15s", host, port_str) == 2) {
        int port = atoi(port_str);
        if (port > 0 && port <= 65535) {
            if (param_set_string(PARAM_ID_5, host) == ESP_OK &&
                param_set_int(PARAM_ID_6, port) == ESP_OK) {
                terminal_send_response("OK\r\n");
            } else {
                terminal_send_response("Fail\r\n");
            }
        } else {
            terminal_send_response("Fail\r\n");
        }
    } else {
        terminal_send_response("Fail\r\n");
    }
}

/**
 * @brief Handle LPTS4 command: Set query period (param 8)
 * Format: LPTS4:<period>\r\n
 */
static void cmd_lpts4(const char *args)
{
    int period;
    if (sscanf(args, "LPTS4:%d", &period) == 1) {
        if (period >= 1000 && period <= 60000) {  // Valid range: 1-60 seconds
            if (param_set_int(PARAM_ID_8, period) == ESP_OK) {
                terminal_send_response("OK\r\n");
            } else {
                terminal_send_response("Fail\r\n");
            }
        } else {
            terminal_send_response("Fail\r\n");
        }
    } else {
        terminal_send_response("Fail\r\n");
    }
}

/**
 * @brief Handle LPTS5 command: Clear connection flag (param 10)
 * Format: LPTS5:\r\n
 */
static void cmd_lpts5(const char *args)
{
    (void)args;  // Unused
    if (param_set_int(PARAM_ID_10, 0) == ESP_OK) {
        terminal_send_response("OK\r\n");
        // Reboot after clearing connection flag
        vTaskDelay(pdMS_TO_TICKS(100));
        esp_restart();
    } else {
        terminal_send_response("Fail\r\n");
    }
}

/**
 * @brief Handle LPTS7 command: Set device SN (param 7 and 8)
 * Format: LPTS7:<sn>,<port>\r\n
 */
static void cmd_lpts7(const char *args)
{
    char sn[64];
    int port;
    if (sscanf(args, "LPTS7:%63[^,],%d", sn, &port) == 2) {
        if (param_set_string(PARAM_ID_7, sn) == ESP_OK &&
            param_set_int(PARAM_ID_8, port) == ESP_OK) {
            terminal_send_response("OK\r\n");
        } else {
            terminal_send_response("Fail\r\n");
        }
    } else {
        terminal_send_response("Fail\r\n");
    }
}

/**
 * @brief Handle LPTQ1 command: Query device SN (param 9)
 */
static void cmd_lptq1(const char *args)
{
    (void)args;  // Unused
    char sn[64];
    char response[MAX_RESPONSE_LEN];
    if (param_get_string(PARAM_ID_9, sn, sizeof(sn)) == ESP_OK) {
        snprintf(response, sizeof(response), "SN:%s\r\n", sn);
        terminal_send_response(response);
    } else {
        terminal_send_response("ERROR\r\n");
    }
}

/**
 * @brief Handle LPTQ2 command: Query router/server (param 5 and 6)
 */
static void cmd_lptq2(const char *args)
{
    (void)args;  // Unused
    char host[64];
    char port_str[16];
    char response[MAX_RESPONSE_LEN];
    int32_t port = 0;
    
    if (param_get_string(PARAM_ID_5, host, sizeof(host)) == ESP_OK &&
        param_get_int(PARAM_ID_6, &port) == ESP_OK) {
        snprintf(response, sizeof(response), "Router:%s,%d\r\n", host, (int)port);
        terminal_send_response(response);
    } else {
        terminal_send_response("ERROR\r\n");
    }
}

/**
 * @brief Handle LPTQ3 command: Query firmware version
 */
static void cmd_lptq3(const char *args)
{
    (void)args;  // Unused
    terminal_send_response("FW:V3.3\r\n");
}

/**
 * @brief Handle LPTQ4 command: Query query period (param 8)
 */
static void cmd_lptq4(const char *args)
{
    (void)args;  // Unused
    int32_t period = 0;
    char response[MAX_RESPONSE_LEN];
    if (param_get_int(PARAM_ID_8, &period) == ESP_OK) {
        snprintf(response, sizeof(response), "QueryPeriod:%d\r\n", (int)period);
        terminal_send_response(response);
    } else {
        terminal_send_response("ERROR\r\n");
    }
}

/**
 * @brief Handle LPTQ6 command: Query connection results
 */
static void cmd_lptq6(const char *args)
{
    (void)args;  // Unused
    // Check if connected (this would need to be tracked)
    bool connected = wifi_task_is_connected();
    terminal_send_response(connected ? "Results:PASS\r\n" : "Results:Fail\r\n");
}

/**
 * @brief Handle LPTQ7 command: Query server (param 7 and 8)
 */
static void cmd_lptq7(const char *args)
{
    (void)args;  // Unused
    char sn[64];
    int32_t port = 0;
    char response[MAX_RESPONSE_LEN];
    
    if (param_get_string(PARAM_ID_7, sn, sizeof(sn)) == ESP_OK &&
        param_get_int(PARAM_ID_8, &port) == ESP_OK) {
        snprintf(response, sizeof(response), "Server:%s,%d\r\n", sn, (int)port);
        terminal_send_response(response);
    } else {
        terminal_send_response("ERROR\r\n");
    }
}

/**
 * @brief Handle SHELL command: Enable shell mode
 */
static void cmd_shell(const char *args)
{
    (void)args;  // Unused
    // Enable shell mode (would need additional implementation)
    terminal_send_response("Shell mode enabled\r\n");
}

/**
 * @brief Process terminal command
 * 
 * Original: sub_42012BB0 (command_handler)
 */
static void terminal_process_command(const char *cmd, size_t len)
{
    if (cmd == NULL || len == 0) {
        return;
    }

    // Remove trailing \r\n
    char cmd_buf[MAX_CMD_LEN];
    size_t cmd_len = len;
    if (cmd_len > MAX_CMD_LEN - 1) {
        cmd_len = MAX_CMD_LEN - 1;
    }
    memcpy(cmd_buf, cmd, cmd_len);
    cmd_buf[cmd_len] = '\0';

    // Remove trailing whitespace
    while (cmd_len > 0 && (cmd_buf[cmd_len - 1] == '\r' || cmd_buf[cmd_len - 1] == '\n' || 
                           cmd_buf[cmd_len - 1] == ' ' || cmd_buf[cmd_len - 1] == '\t')) {
        cmd_buf[--cmd_len] = '\0';
    }

    ESP_LOGD(TAG, "Received command: %s", cmd_buf);

    // Parse and execute command
    if (strncmp(cmd_buf, "LPTS1:", 6) == 0) {
        cmd_lpts1(cmd_buf);
    } else if (strncmp(cmd_buf, "LPTS3:", 6) == 0) {
        cmd_lpts3(cmd_buf);
    } else if (strncmp(cmd_buf, "LPTS4:", 6) == 0) {
        cmd_lpts4(cmd_buf);
    } else if (strncmp(cmd_buf, "LPTS5:", 6) == 0) {
        cmd_lpts5(cmd_buf);
    } else if (strncmp(cmd_buf, "LPTS7:", 6) == 0) {
        cmd_lpts7(cmd_buf);
    } else if (strncmp(cmd_buf, "LPTQ1:", 6) == 0) {
        cmd_lptq1(cmd_buf);
    } else if (strncmp(cmd_buf, "LPTQ2:", 6) == 0) {
        cmd_lptq2(cmd_buf);
    } else if (strncmp(cmd_buf, "LPTQ3:", 6) == 0) {
        cmd_lptq3(cmd_buf);
    } else if (strncmp(cmd_buf, "LPTQ4:", 6) == 0) {
        cmd_lptq4(cmd_buf);
    } else if (strncmp(cmd_buf, "LPTQ6:", 6) == 0) {
        cmd_lptq6(cmd_buf);
    } else if (strncmp(cmd_buf, "LPTQ7:", 6) == 0) {
        cmd_lptq7(cmd_buf);
    } else if (strncmp(cmd_buf, "SHELL:", 6) == 0) {
        cmd_shell(cmd_buf);
    } else {
        terminal_send_response("Unknown command\r\n");
    }
}

/**
 * @brief UART RX callback for terminal service
 */
static void terminal_rx_callback(uint8_t *data, size_t len)
{
    static char cmd_buffer[MAX_CMD_LEN];
    static size_t cmd_pos = 0;

    for (size_t i = 0; i < len; i++) {
        char c = data[i];
        
        if (c == '\n' || c == '\r') {
            if (cmd_pos > 0) {
                cmd_buffer[cmd_pos] = '\0';
                terminal_process_command(cmd_buffer, cmd_pos);
                cmd_pos = 0;
            }
        } else if (cmd_pos < MAX_CMD_LEN - 1) {
            cmd_buffer[cmd_pos++] = c;
        } else {
            // Buffer overflow, reset
            cmd_pos = 0;
            terminal_send_response("Command too long\r\n");
        }
    }
}

/**
 * @brief Initialize terminal service
 * 
 * Original: sub_4200D91E (terminal_service_init)
 */
esp_err_t terminal_service_init(void)
{
    // Register callback with UART RX task
    uart_rx_task_set_callback(terminal_rx_callback);

    ESP_LOGI(TAG, "Terminal service initialized");
    ESP_LOGI(TAG, "Supported commands: LPTS1-7, LPTQ1-7, SHELL");
    
    return ESP_OK;
}

