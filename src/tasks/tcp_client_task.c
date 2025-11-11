/**
 * @file tcp_client_task.c
 * @brief TCP client task implementation
 * 
 * Original: sub_4200D710 (TCPClientApp_Init), sub_42014D4A (tcp_client_task)
 * 
 * This task handles:
 * - TCP connection to "dongle_ssl.solarcloudsystem.com:4348"
 * - TLS/SSL with PSK authentication
 * - Heartbeat mechanism (function code 193)
 * - Data reception and transmission
 * - Automatic reconnection
 */

#include "tcp_client_task.h"
#include "../config/param_manager.h"
#include "../config/param_ids.h"
#include "../protocol/data_process.h"
#include "../protocol/function_codes.h"
#include "../protocol/crc_utils.h"
#include "../tasks/wifi_task.h"
#include "../tasks/rs485_task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_https_ota.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "mbedtls/ssl.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/md5.h"
#include "mbedtls/error.h"
#include "mbedtls/platform.h"

static const char *TAG = "tcp_client";

// TLS wrapper using mbedtls directly (esp_tls.h not available in v5.5)
typedef struct {
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_net_context server_fd;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    int sockfd;
    bool initialized;
} esp_tls_t;

typedef struct {
    int timeout_ms;
    const char *psk_hint_key;
    size_t psk_hint_key_len;
    const uint8_t *psk_key;
    size_t psk_key_len;
    const char **alpn_protos;
    bool skip_common_name;
    bool use_global_ca_store;
    int sockfd;
    const unsigned char *servercert_buf;
    size_t servercert_buf_len;
    const unsigned char *cacert_buf;
    size_t cacert_buf_len;
} esp_tls_cfg_t;

// TLS wrapper functions using mbedtls
static esp_tls_t *esp_tls_conn_new_sync(const char *hostname, size_t hostname_len, int port, const esp_tls_cfg_t *cfg)
{
    if (!cfg) return NULL;
    
    esp_tls_t *tls = calloc(1, sizeof(esp_tls_t));
    if (!tls) return NULL;
    
    mbedtls_ssl_init(&tls->ssl);
    mbedtls_ssl_config_init(&tls->conf);
    mbedtls_net_init(&tls->server_fd);
    mbedtls_entropy_init(&tls->entropy);
    mbedtls_ctr_drbg_init(&tls->ctr_drbg);
    
    tls->sockfd = cfg->sockfd;
    tls->server_fd.fd = cfg->sockfd;
    
    const char *pers = "tls_client";
    int ret = mbedtls_ctr_drbg_seed(&tls->ctr_drbg, mbedtls_entropy_func, &tls->entropy, (const unsigned char *)pers, strlen(pers));
    if (ret != 0) {
        ESP_LOGE(TAG, "mbedtls_ctr_drbg_seed failed: %d", ret);
        goto error;
    }
    
    ret = mbedtls_ssl_config_defaults(&tls->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        ESP_LOGE(TAG, "mbedtls_ssl_config_defaults failed: %d", ret);
        goto error;
    }
    
    // Configure PSK
    if (cfg->psk_key && cfg->psk_key_len > 0) {
        ret = mbedtls_ssl_conf_psk(&tls->conf, cfg->psk_key, cfg->psk_key_len, 
                                   (const unsigned char *)cfg->psk_hint_key, cfg->psk_hint_key_len);
        if (ret != 0) {
            ESP_LOGE(TAG, "mbedtls_ssl_conf_psk failed: %d", ret);
            goto error;
        }
    }
    
    mbedtls_ssl_conf_authmode(&tls->conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_rng(&tls->conf, mbedtls_ctr_drbg_random, &tls->ctr_drbg);
    
    ret = mbedtls_ssl_setup(&tls->ssl, &tls->conf);
    if (ret != 0) {
        ESP_LOGE(TAG, "mbedtls_ssl_setup failed: %d", ret);
        goto error;
    }
    
    if (hostname) {
        ret = mbedtls_ssl_set_hostname(&tls->ssl, hostname);
        if (ret != 0) {
            ESP_LOGE(TAG, "mbedtls_ssl_set_hostname failed: %d", ret);
            goto error;
        }
    }
    
    mbedtls_ssl_set_bio(&tls->ssl, &tls->server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);
    
    // Perform handshake
    while ((ret = mbedtls_ssl_handshake(&tls->ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            ESP_LOGE(TAG, "mbedtls_ssl_handshake failed: %d", ret);
            goto error;
        }
    }
    
    tls->initialized = true;
    return tls;
    
error:
    mbedtls_ssl_free(&tls->ssl);
    mbedtls_ssl_config_free(&tls->conf);
    mbedtls_net_free(&tls->server_fd);
    mbedtls_entropy_free(&tls->entropy);
    mbedtls_ctr_drbg_free(&tls->ctr_drbg);
    free(tls);
    return NULL;
}

static int esp_tls_conn_read(esp_tls_t *tls, void *data, size_t datalen)
{
    if (!tls || !tls->initialized) return -1;
    return mbedtls_ssl_read(&tls->ssl, (unsigned char *)data, datalen);
}

static int esp_tls_conn_write(esp_tls_t *tls, const void *data, size_t datalen)
{
    if (!tls || !tls->initialized) return -1;
    return mbedtls_ssl_write(&tls->ssl, (const unsigned char *)data, datalen);
}

static void esp_tls_conn_delete(esp_tls_t *tls)
{
    if (!tls) return;
    if (tls->initialized) {
        mbedtls_ssl_close_notify(&tls->ssl);
    }
    mbedtls_ssl_free(&tls->ssl);
    mbedtls_ssl_config_free(&tls->conf);
    mbedtls_net_free(&tls->server_fd);
    mbedtls_entropy_free(&tls->entropy);
    mbedtls_ctr_drbg_free(&tls->ctr_drbg);
    free(tls);
}

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdlib.h>

#define TCP_CLIENT_HOST            "dongle_ssl.solarcloudsystem.com"
#define TCP_CLIENT_PORT            4348
#define TCP_CLIENT_PSK_IDENTITY     "psk_identity_dongle"
#define TCP_CLIENT_PSK_KEY_PREFIX   "LuxD1ngl2X"
#define TCP_CLIENT_RECV_BUF_SIZE   2048
#define TCP_CLIENT_CONNECT_TIMEOUT  10000  // 10 seconds
#define TCP_CLIENT_RECONNECT_DELAY  5000   // 5 seconds
#define TCP_CLIENT_HEARTBEAT_INTERVAL 10000  // 10 seconds

// TCP client state
typedef enum {
    TCP_CLIENT_STATE_DISCONNECTED,
    TCP_CLIENT_STATE_CONNECTING,
    TCP_CLIENT_STATE_CONNECTED,
    TCP_CLIENT_STATE_TLS_HANDSHAKE,
    TCP_CLIENT_STATE_READY
} tcp_client_state_t;

// TCP client structure
typedef struct {
    int sock;
    esp_tls_t *tls;
    esp_tls_cfg_t tls_cfg;
    tcp_client_state_t state;
    char host[128];
    uint16_t port;
    uint8_t psk[16];
    uint8_t *recv_buffer;
    SemaphoreHandle_t mutex;
    TaskHandle_t task_handle;
    data_process_handle_t data_handle;
    void (*receive_callback)(const uint8_t *data, size_t len);
    void (*send_callback)(const uint8_t *data, size_t len);
    uint32_t last_heartbeat;
    bool use_tls;
} tcp_client_t;

static tcp_client_t s_tcp_client = {0};

/**
 * @brief Generate PSK key from device SN
 * 
 * Original: PSK generation logic in sub_42014D4A
 * PSK = MD5("LuxD1ngl2X" + device_sn)
 */
static esp_err_t tcp_client_generate_psk(const char *device_sn, uint8_t *psk_out)
{
    mbedtls_md5_context ctx;
    uint8_t hash[16];
    char input[128];
    
    // Build input string: "LuxD1ngl2X" + device_sn
    snprintf(input, sizeof(input), "%s%s", TCP_CLIENT_PSK_KEY_PREFIX, device_sn);
    
    mbedtls_md5_init(&ctx);
    mbedtls_md5_starts(&ctx);
    mbedtls_md5_update(&ctx, (const unsigned char *)input, strlen(input));
    mbedtls_md5_finish(&ctx, hash);
    mbedtls_md5_free(&ctx);
    
    memcpy(psk_out, hash, 16);
    
    ESP_LOGD(TAG, "Generated PSK for device SN: %s", device_sn);
    return ESP_OK;
}

/**
 * @brief TCP client receive callback
 * 
 * Original: sub_420118A4 (tcp_client_receive)
 * 
 * Parses protocol frames and forwards data transmission frames to RS485
 */
static void tcp_client_receive_callback(const uint8_t *data, size_t len)
{
    ESP_LOGD(TAG, "TCP client received %zu bytes", len);
    
    // Parse protocol frame if it's a data transmission frame (function code 194)
    if (len >= 20 && data[0] == 0xA1 && data[1] == 0x1A) {
        uint8_t func_code = data[7];
        
        if (func_code == PROTOCOL_FC_DATA_TRANSMISSION) {
            // Parse data transmission frame
            uint8_t *modbus_data = NULL;
            uint16_t modbus_data_len = 0;
            
            if (parse_data_transmission_frame(data, len, &modbus_data, &modbus_data_len) == 0) {
                // Forward to RS485 as Modbus frame
                if (modbus_data != NULL && modbus_data_len > 0) {
                    ESP_LOGD(TAG, "Forwarding TCP data to RS485: %u bytes", modbus_data_len);
                    
                    // Build Modbus frame: [addr][func][data][crc(2)]
                    // Note: The protocol frame doesn't preserve the original Modbus address/function code,
                    // so we use defaults. The actual address/func should be extracted from the protocol
                    // frame header if available, or configured via parameters.
                    uint8_t modbus_frame[512];
                    if (modbus_data_len + 4 <= sizeof(modbus_frame)) {
                        modbus_frame[0] = 0x01;  // Modbus address (default - should be configurable)
                        modbus_frame[1] = 0x03;  // Function code (default - should match original frame)
                        memcpy(&modbus_frame[2], modbus_data, modbus_data_len);
                        
                        // Calculate CRC
                        uint16_t crc = modbus_crc16(modbus_frame, modbus_data_len + 2);
                        modbus_frame[modbus_data_len + 2] = crc & 0xFF;
                        modbus_frame[modbus_data_len + 3] = (crc >> 8) & 0xFF;
                        
                        // Send to RS485
                        esp_err_t send_ret = rs485_task_send_frame(modbus_frame, modbus_data_len + 4);
                        if (send_ret != ESP_OK) {
                            ESP_LOGW(TAG, "Failed to send frame to RS485: %d", send_ret);
                        }
                    }
                }
            }
        }
    }
    
    // Call user callback if registered
    if (s_tcp_client.receive_callback) {
        s_tcp_client.receive_callback(data, len);
    }
}

/**
 * @brief TCP client send callback
 * 
 * Original: sub_42011AA8 (tcp_client_send)
 */
static void tcp_client_send_callback(const uint8_t *data, size_t len)
{
    if (s_tcp_client.sock < 0 || s_tcp_client.state != TCP_CLIENT_STATE_READY) {
        ESP_LOGW(TAG, "Cannot send: not connected");
        return;
    }

    int sent = 0;
    if (s_tcp_client.use_tls && s_tcp_client.tls) {
        sent = esp_tls_conn_write(s_tcp_client.tls, data, len);
    } else {
        sent = send(s_tcp_client.sock, data, len, 0);
    }

    if (sent < 0) {
        ESP_LOGE(TAG, "Failed to send data: %d", sent);
    } else if (sent != len) {
        ESP_LOGW(TAG, "Partial send: %d/%zu bytes", sent, len);
    }
}

/**
 * @brief Send heartbeat
 * 
 * Original: sub_42010FDC (send_heartbeat)
 */
static esp_err_t tcp_client_send_heartbeat(void)
{
    // Heartbeat is function code 193 (0xC1)
    // Build minimal heartbeat frame
    uint8_t heartbeat_frame[] = {0xC1, 0x00, 0x00, 0x00};  // Function code + minimal data
    
    if (s_tcp_client.send_callback) {
        s_tcp_client.send_callback(heartbeat_frame, sizeof(heartbeat_frame));
    }
    
    s_tcp_client.last_heartbeat = xTaskGetTickCount();
    return ESP_OK;
}

/**
 * @brief TCP client task
 * 
 * Original: sub_42014D4A (tcp_client_task)
 */
static void tcp_client_task(void *pvParameters)
{
    struct sockaddr_in server_addr;
    struct addrinfo hints, *res = NULL;
    char port_str[8];
    int ret;
    int bytes_received;
    
    ESP_LOGI(TAG, "TCP client task started");

    while (1) {
        // Wait for WiFi connection
        while (!wifi_task_is_connected()) {
            ESP_LOGI(TAG, "Waiting for WiFi connection...");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        // Get server hostname and port from parameters
        char host[128];
        uint16_t port = TCP_CLIENT_PORT;
        
        esp_err_t err = param_get_string(PARAM_ID_5, host, sizeof(host));
        if (err == ESP_ERR_NOT_FOUND) {
            strncpy(host, TCP_CLIENT_HOST, sizeof(host) - 1);
        }
        
        int32_t port_int = 0;
        err = param_get_int(PARAM_ID_6, &port_int);
        if (err == ESP_OK && port_int > 0) {
            port = (uint16_t)port_int;
        }

        strncpy(s_tcp_client.host, host, sizeof(s_tcp_client.host) - 1);
        s_tcp_client.port = port;

        ESP_LOGI(TAG, "Connecting to %s:%d", s_tcp_client.host, s_tcp_client.port);

        // Resolve hostname
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        
        snprintf(port_str, sizeof(port_str), "%d", port);
        ret = getaddrinfo(host, port_str, &hints, &res);
        if (ret != 0 || res == NULL) {
            ESP_LOGE(TAG, "getaddrinfo failed: %s", gai_strerror(ret));
            vTaskDelay(pdMS_TO_TICKS(TCP_CLIENT_RECONNECT_DELAY));
            continue;
        }

        // Create socket
        s_tcp_client.sock = socket(AF_INET, SOCK_STREAM, 0);
        if (s_tcp_client.sock < 0) {
            ESP_LOGE(TAG, "Failed to create socket: %d", errno);
            freeaddrinfo(res);
            vTaskDelay(pdMS_TO_TICKS(TCP_CLIENT_RECONNECT_DELAY));
            continue;
        }

        // Connect
        s_tcp_client.state = TCP_CLIENT_STATE_CONNECTING;
        memcpy(&server_addr, res->ai_addr, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        
        ret = connect(s_tcp_client.sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
        freeaddrinfo(res);
        
        if (ret < 0) {
            ESP_LOGE(TAG, "Failed to connect: %d", errno);
            close(s_tcp_client.sock);
            s_tcp_client.sock = -1;
            vTaskDelay(pdMS_TO_TICKS(TCP_CLIENT_RECONNECT_DELAY));
            continue;
        }

        ESP_LOGI(TAG, "Connected to server");

        // Setup TLS
        s_tcp_client.state = TCP_CLIENT_STATE_TLS_HANDSHAKE;
        
        // Get device SN for PSK
        char device_sn[64];
        err = param_get_string(PARAM_ID_7, device_sn, sizeof(device_sn));
        if (err == ESP_ERR_NOT_FOUND) {
            strncpy(device_sn, "default", sizeof(device_sn) - 1);
        }
        
        // Generate PSK
        tcp_client_generate_psk(device_sn, s_tcp_client.psk);

        // Configure TLS
        memset(&s_tcp_client.tls_cfg, 0, sizeof(s_tcp_client.tls_cfg));
        s_tcp_client.tls_cfg.timeout_ms = TCP_CLIENT_CONNECT_TIMEOUT;
        s_tcp_client.tls_cfg.psk_hint_key = TCP_CLIENT_PSK_IDENTITY;
        s_tcp_client.tls_cfg.psk_hint_key_len = strlen(TCP_CLIENT_PSK_IDENTITY);
        s_tcp_client.tls_cfg.psk_key = s_tcp_client.psk;
        s_tcp_client.tls_cfg.psk_key_len = 16;
        s_tcp_client.tls_cfg.alpn_protos = NULL;
        s_tcp_client.tls_cfg.skip_common_name = true;
        s_tcp_client.tls_cfg.use_global_ca_store = false;

        // Create TLS connection (using existing socket)
        s_tcp_client.tls_cfg.sockfd = s_tcp_client.sock;
        s_tcp_client.tls = esp_tls_conn_new_sync(host, strlen(host), port, &s_tcp_client.tls_cfg);
        if (s_tcp_client.tls == NULL) {
            ESP_LOGE(TAG, "TLS handshake failed");
            close(s_tcp_client.sock);
            s_tcp_client.sock = -1;
            s_tcp_client.state = TCP_CLIENT_STATE_DISCONNECTED;
            vTaskDelay(pdMS_TO_TICKS(TCP_CLIENT_RECONNECT_DELAY));
            continue;
        }

        s_tcp_client.use_tls = true;
        s_tcp_client.state = TCP_CLIENT_STATE_READY;
        s_tcp_client.last_heartbeat = xTaskGetTickCount();
        
        ESP_LOGI(TAG, "TLS connection established");

        // Main receive loop
        while (s_tcp_client.state == TCP_CLIENT_STATE_READY) {
            // Check for heartbeat timeout
            if ((xTaskGetTickCount() - s_tcp_client.last_heartbeat) > 
                pdMS_TO_TICKS(TCP_CLIENT_HEARTBEAT_INTERVAL)) {
                tcp_client_send_heartbeat();
            }

            // Receive data
            if (s_tcp_client.use_tls && s_tcp_client.tls) {
                bytes_received = esp_tls_conn_read(s_tcp_client.tls, 
                                                   s_tcp_client.recv_buffer, 
                                                   TCP_CLIENT_RECV_BUF_SIZE);
            } else {
                bytes_received = recv(s_tcp_client.sock, 
                                      s_tcp_client.recv_buffer, 
                                      TCP_CLIENT_RECV_BUF_SIZE, 0);
            }

            if (bytes_received > 0) {
                // Process received data
                if (s_tcp_client.data_handle) {
                    data_process_receive(s_tcp_client.data_handle, 
                                        s_tcp_client.recv_buffer, 
                                        bytes_received);
                }
                tcp_client_receive_callback(s_tcp_client.recv_buffer, bytes_received);
            } else if (bytes_received < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                    continue;
                }
                ESP_LOGE(TAG, "Receive error: %d", errno);
                break;
            } else {
                // Connection closed
                ESP_LOGI(TAG, "Connection closed by server");
                break;
            }
        }

        // Cleanup
        if (s_tcp_client.tls) {
            esp_tls_conn_delete(s_tcp_client.tls);
            s_tcp_client.tls = NULL;
        }
        if (s_tcp_client.sock >= 0) {
            close(s_tcp_client.sock);
            s_tcp_client.sock = -1;
        }
        s_tcp_client.state = TCP_CLIENT_STATE_DISCONNECTED;
        s_tcp_client.use_tls = false;

        ESP_LOGI(TAG, "Disconnected, reconnecting in %d ms...", TCP_CLIENT_RECONNECT_DELAY);
        vTaskDelay(pdMS_TO_TICKS(TCP_CLIENT_RECONNECT_DELAY));
    }
}

/**
 * @brief Initialize TCP client task
 * 
 * Original: sub_4200D710 (TCPClientApp_Init)
 */
esp_err_t tcp_client_task_init(void)
{
    // Allocate receive buffer
    s_tcp_client.recv_buffer = malloc(TCP_CLIENT_RECV_BUF_SIZE);
    if (s_tcp_client.recv_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate receive buffer");
        return ESP_ERR_NO_MEM;
    }

    // Create mutex
    s_tcp_client.mutex = xSemaphoreCreateMutex();
    if (s_tcp_client.mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        free(s_tcp_client.recv_buffer);
        return ESP_ERR_NO_MEM;
    }

    // Initialize state
    s_tcp_client.sock = -1;
    s_tcp_client.tls = NULL;
    s_tcp_client.state = TCP_CLIENT_STATE_DISCONNECTED;
    s_tcp_client.use_tls = false;
    s_tcp_client.receive_callback = NULL;
    s_tcp_client.send_callback = tcp_client_send_callback;
    s_tcp_client.last_heartbeat = 0;

    // Create data processing handle
    s_tcp_client.data_handle = data_process_create(
        tcp_client_send_callback,
        tcp_client_receive_callback
    );
    if (s_tcp_client.data_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create data process handle");
        vSemaphoreDelete(s_tcp_client.mutex);
        free(s_tcp_client.recv_buffer);
        return ESP_FAIL;
    }

    // Create TCP client task (priority 5)
    BaseType_t ret = xTaskCreate(tcp_client_task, "tcp_client", 8192, NULL, 5, 
                                  &s_tcp_client.task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create TCP client task");
        data_process_destroy(s_tcp_client.data_handle);
        vSemaphoreDelete(s_tcp_client.mutex);
        free(s_tcp_client.recv_buffer);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "TCP client task initialized");
    return ESP_OK;
}

/**
 * @brief Check if TCP client is connected
 */
bool tcp_client_task_is_connected(void)
{
    return (s_tcp_client.state == TCP_CLIENT_STATE_READY);
}

/**
 * @brief Send data through TCP client
 */
esp_err_t tcp_client_task_send(const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!tcp_client_task_is_connected()) {
        return ESP_ERR_INVALID_STATE;
    }

    tcp_client_send_callback(data, len);
    return ESP_OK;
}

