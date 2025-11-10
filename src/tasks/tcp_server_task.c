/**
 * @file tcp_server_task.c
 * @brief TCP server task implementation
 * 
 * Original: sub_42012878 (TCPServerApp_Init), sub_4201427A (tcp_server_task)
 * 
 * This task handles:
 * - TCP server socket creation and binding
 * - TLS server setup
 * - Multiple client connection management
 * - Data reception and transmission
 */

#include "tcp_server_task.h"
#include "../protocol/data_process.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_tls_mbedtls.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

static const char *TAG = "tcp_server";

#define TCP_SERVER_PORT           8080
#define TCP_SERVER_MAX_CLIENTS     4
#define TCP_SERVER_RECV_BUF_SIZE   2048
#define TCP_SERVER_BACKLOG         5

// Client connection state
typedef enum {
    TCP_CLIENT_STATE_FREE,
    TCP_CLIENT_STATE_CONNECTING,
    TCP_CLIENT_STATE_CONNECTED,
    TCP_CLIENT_STATE_TLS_HANDSHAKE,
    TCP_CLIENT_STATE_READY
} tcp_client_state_t;

// Client structure
typedef struct {
    int sock;
    esp_tls_t *tls;
    esp_tls_cfg_t tls_cfg;
    tcp_client_state_t state;
    TaskHandle_t task_handle;
    data_process_handle_t data_handle;
    uint8_t *recv_buffer;
    SemaphoreHandle_t mutex;
    char name[32];
} tcp_client_t;

// TCP server structure
typedef struct {
    int listen_sock;
    uint16_t port;
    tcp_client_t clients[TCP_SERVER_MAX_CLIENTS];
    SemaphoreHandle_t mutex;
    TaskHandle_t server_task_handle;
    bool use_tls;
} tcp_server_t;

static tcp_server_t s_tcp_server = {0};

/**
 * @brief Client receive task
 * 
 * Original: sub_42014B6C (client_receive_task)
 */
static void tcp_client_receive_task(void *pvParameters)
{
    tcp_client_t *client = (tcp_client_t *)pvParameters;
    int bytes_received;
    
    ESP_LOGI(TAG, "[%s] Client receive task started", client->name);

    while (client->state == TCP_CLIENT_STATE_READY) {
        // Receive data
        if (s_tcp_server.use_tls && client->tls) {
            bytes_received = esp_tls_conn_read(client->tls, 
                                               client->recv_buffer, 
                                               TCP_SERVER_RECV_BUF_SIZE);
        } else {
            bytes_received = recv(client->sock, 
                                  client->recv_buffer, 
                                  TCP_SERVER_RECV_BUF_SIZE, 0);
        }

        if (bytes_received > 0) {
            // Process received data
            if (client->data_handle) {
                data_process_receive(client->data_handle, 
                                    client->recv_buffer, 
                                    bytes_received);
            }
        } else if (bytes_received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }
            ESP_LOGE(TAG, "[%s] Receive error: %d", client->name, errno);
            break;
        } else {
            // Connection closed
            ESP_LOGI(TAG, "[%s] Connection closed by client", client->name);
            break;
        }
    }

    // Cleanup
    if (xSemaphoreTake(s_tcp_server.mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        if (client->tls) {
            esp_tls_conn_delete(client->tls);
            client->tls = NULL;
        }
        if (client->sock >= 0) {
            close(client->sock);
            client->sock = -1;
        }
        if (client->data_handle) {
            data_process_destroy(client->data_handle);
            client->data_handle = NULL;
        }
        if (client->recv_buffer) {
            free(client->recv_buffer);
            client->recv_buffer = NULL;
        }
        client->state = TCP_CLIENT_STATE_FREE;
        xSemaphoreGive(s_tcp_server.mutex);
    }

    ESP_LOGI(TAG, "[%s] Client receive task ended", client->name);
    vTaskDelete(NULL);
}

/**
 * @brief Client send callback
 */
static void tcp_client_send_callback(const uint8_t *data, size_t len)
{
    // Find the client that should send this data
    // For now, send to first connected client
    for (int i = 0; i < TCP_SERVER_MAX_CLIENTS; i++) {
        if (s_tcp_server.clients[i].state == TCP_CLIENT_STATE_READY) {
            tcp_client_t *client = &s_tcp_server.clients[i];
            int sent = 0;
            
            if (s_tcp_server.use_tls && client->tls) {
                sent = esp_tls_conn_write(client->tls, data, len);
            } else {
                sent = send(client->sock, data, len, 0);
            }
            
            if (sent < 0) {
                ESP_LOGE(TAG, "[%s] Failed to send data: %d", client->name, sent);
            }
            break;  // Send to first available client
        }
    }
}

/**
 * @brief Client receive callback
 * 
 * Forwards received data to RS485 if needed
 */
static void tcp_client_receive_callback(const uint8_t *data, size_t len)
{
    ESP_LOGD(TAG, "Received %zu bytes from client", len);
    
    // Process received data through data processing module
    // The data processing module will handle protocol parsing
    // and can trigger RS485 forwarding if needed
    // This is handled by the data_process_receive call in the receive task
}

/**
 * @brief TCP server accept task
 * 
 * Original: sub_4201427A (tcp_server_task)
 */
static void tcp_server_task(void *pvParameters)
{
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_sock;
    int opt = 1;
    
    ESP_LOGI(TAG, "TCP server task started on port %d", s_tcp_server.port);

    // Create socket
    s_tcp_server.listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (s_tcp_server.listen_sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket: %d", errno);
        return;
    }

    // Set socket options
    setsockopt(s_tcp_server.listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(s_tcp_server.port);

    if (bind(s_tcp_server.listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to bind socket: %d", errno);
        close(s_tcp_server.listen_sock);
        return;
    }

    // Listen
    if (listen(s_tcp_server.listen_sock, TCP_SERVER_BACKLOG) < 0) {
        ESP_LOGE(TAG, "Failed to listen: %d", errno);
        close(s_tcp_server.listen_sock);
        return;
    }

    ESP_LOGI(TAG, "TCP server listening on port %d", s_tcp_server.port);

    while (1) {
        // Accept connection
        client_sock = accept(s_tcp_server.listen_sock, 
                            (struct sockaddr *)&client_addr, 
                            &client_addr_len);
        
        if (client_sock < 0) {
            ESP_LOGE(TAG, "Failed to accept connection: %d", errno);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        ESP_LOGI(TAG, "New client connected from %s:%d", 
                 inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Find free client slot
        tcp_client_t *client = NULL;
        if (xSemaphoreTake(s_tcp_server.mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            for (int i = 0; i < TCP_SERVER_MAX_CLIENTS; i++) {
                if (s_tcp_server.clients[i].state == TCP_CLIENT_STATE_FREE) {
                    client = &s_tcp_server.clients[i];
                    break;
                }
            }
            
            if (client == NULL) {
                ESP_LOGW(TAG, "No available client slots, closing connection");
                close(client_sock);
                xSemaphoreGive(s_tcp_server.mutex);
                continue;
            }

            // Initialize client
            client->sock = client_sock;
            client->state = TCP_CLIENT_STATE_CONNECTING;
            snprintf(client->name, sizeof(client->name), "client.%d", client - s_tcp_server.clients);
            
            // Allocate receive buffer
            client->recv_buffer = malloc(TCP_SERVER_RECV_BUF_SIZE);
            if (client->recv_buffer == NULL) {
                ESP_LOGE(TAG, "[%s] Failed to allocate receive buffer", client->name);
                close(client_sock);
                client->state = TCP_CLIENT_STATE_FREE;
                xSemaphoreGive(s_tcp_server.mutex);
                continue;
            }

            // Setup TLS if enabled
            if (s_tcp_server.use_tls) {
                client->state = TCP_CLIENT_STATE_TLS_HANDSHAKE;
                
                // Configure TLS
                memset(&client->tls_cfg, 0, sizeof(client->tls_cfg));
                client->tls_cfg.timeout_ms = 5000;
                client->tls_cfg.servercert_buf = NULL;  // Would need actual certificate
                client->tls_cfg.servercert_buf_len = 0;
                client->tls_cfg.cacert_buf = NULL;
                client->tls_cfg.cacert_buf_len = 0;
                client->tls_cfg.alpn_protos = NULL;
                client->tls_cfg.use_global_ca_store = false;
                client->tls_cfg.sockfd = client_sock;  // Use existing socket

                // Create TLS connection (server mode)
                client->tls = esp_tls_conn_new_sync(NULL, 0, 0, &client->tls_cfg);
                if (client->tls == NULL) {
                    ESP_LOGE(TAG, "[%s] TLS handshake failed", client->name);
                    free(client->recv_buffer);
                    close(client_sock);
                    client->state = TCP_CLIENT_STATE_FREE;
                    xSemaphoreGive(s_tcp_server.mutex);
                    continue;
                }
            }

            // Create data processing handle
            client->data_handle = data_process_create(
                tcp_client_send_callback,
                tcp_client_receive_callback
            );

            client->state = TCP_CLIENT_STATE_READY;
            xSemaphoreGive(s_tcp_server.mutex);

            // Create client receive task
            BaseType_t ret = xTaskCreate(tcp_client_receive_task, 
                                        client->name, 
                                        4096, 
                                        client, 
                                        4, 
                                        &client->task_handle);
            if (ret != pdPASS) {
                ESP_LOGE(TAG, "[%s] Failed to create receive task", client->name);
                if (xSemaphoreTake(s_tcp_server.mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                    if (client->tls) {
                        esp_tls_conn_delete(client->tls);
                    }
                    if (client->data_handle) {
                        data_process_destroy(client->data_handle);
                    }
                    free(client->recv_buffer);
                    close(client_sock);
                    client->state = TCP_CLIENT_STATE_FREE;
                    xSemaphoreGive(s_tcp_server.mutex);
                }
            }
        }
    }
}

/**
 * @brief Initialize TCP server task
 * 
 * Original: sub_42012878 (TCPServerApp_Init)
 */
esp_err_t tcp_server_task_init(void)
{
    // Create mutex
    s_tcp_server.mutex = xSemaphoreCreateMutex();
    if (s_tcp_server.mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    // Initialize server
    s_tcp_server.listen_sock = -1;
    s_tcp_server.port = TCP_SERVER_PORT;
    s_tcp_server.use_tls = false;  // TLS can be enabled if certificates are available

    // Initialize clients
    for (int i = 0; i < TCP_SERVER_MAX_CLIENTS; i++) {
        s_tcp_server.clients[i].sock = -1;
        s_tcp_server.clients[i].tls = NULL;
        s_tcp_server.clients[i].state = TCP_CLIENT_STATE_FREE;
        s_tcp_server.clients[i].recv_buffer = NULL;
        s_tcp_server.clients[i].data_handle = NULL;
        s_tcp_server.clients[i].task_handle = NULL;
    }

    // Create TCP server task (priority 5)
    BaseType_t ret = xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, 
                                  &s_tcp_server.server_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create TCP server task");
        vSemaphoreDelete(s_tcp_server.mutex);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "TCP server task initialized on port %d", s_tcp_server.port);
    return ESP_OK;
}

/**
 * @brief Check if TCP server is running
 */
bool tcp_server_task_is_running(void)
{
    return (s_tcp_server.listen_sock >= 0);
}

/**
 * @brief Get number of connected clients
 */
int tcp_server_task_get_client_count(void)
{
    int count = 0;
    if (xSemaphoreTake(s_tcp_server.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (int i = 0; i < TCP_SERVER_MAX_CLIENTS; i++) {
            if (s_tcp_server.clients[i].state == TCP_CLIENT_STATE_READY) {
                count++;
            }
        }
        xSemaphoreGive(s_tcp_server.mutex);
    }
    return count;
}

