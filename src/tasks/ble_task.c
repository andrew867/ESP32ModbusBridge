/**
 * @file ble_task.c
 * @brief BLE task implementation
 * 
 * Original: sub_4201000E (ble_app_task, priority 10), sub_4200F298 (ble_data_receive)
 * 
 * This task handles:
 * - BLE GATT server setup
 * - Characteristic read/write
 * - BLE advertising
 * - Data processing through BLE
 */

#include "ble_task.h"
#include "../protocol/data_process.h"
#include "../protocol/function_codes.h"
#include "../config/param_manager.h"
#include "../config/param_ids.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static const char *TAG = "ble_task";

#define BLE_DEVICE_NAME            "LuxWiFiDongle"
#define BLE_SERVICE_UUID           0x1800  // Generic Access Profile
#define BLE_CHAR_RX_UUID            0x2A00  // Device Name
#define BLE_CHAR_TX_UUID            0x2A01  // Appearance

// BLE characteristic handles
static uint16_t s_char_rx_handle = 0;
static uint16_t s_char_tx_handle = 0;
static data_process_handle_t s_data_handle = NULL;
static uint16_t s_conn_handle = BLE_HS_CONN_HANDLE_NONE;  // Current connection handle

/**
 * @brief BLE send callback
 */
static void ble_send_callback(const uint8_t *data, size_t len)
{
    // Send data via BLE notify/indicate
    if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE && s_char_tx_handle != 0) {
        struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
        if (om != NULL) {
            // Use ble_gatts_notify to send notification
            int rc = ble_gatts_notify(s_conn_handle, s_char_tx_handle, om);
            if (rc != 0) {
                ESP_LOGE(TAG, "Failed to send BLE notify: %d", rc);
                os_mbuf_free_chain(om);
            } else {
                ESP_LOGD(TAG, "BLE notify sent: %zu bytes", len);
            }
        } else {
            ESP_LOGE(TAG, "Failed to create mbuf for BLE notify");
        }
    } else {
        ESP_LOGD(TAG, "BLE send: %zu bytes (no connection)", len);
    }
}

/**
 * @brief BLE receive callback
 */
static void ble_receive_callback(const uint8_t *data, size_t len)
{
    // Process received data
    ESP_LOGD(TAG, "BLE receive: %zu bytes", len);
    
    if (s_data_handle) {
        data_process_receive(s_data_handle, data, len);
    }
}

/**
 * @brief BLE GATT write callback
 * 
 * Original: sub_4200F298 (ble_data_receive)
 */
static int ble_gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                   struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    int rc;

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        // Read characteristic
        if (attr_handle == s_char_tx_handle) {
            // Return device status or data
            const char *status = "OK";
            rc = os_mbuf_append(ctxt->om, status, strlen(status));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
    } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        // Write characteristic
        if (attr_handle == s_char_rx_handle) {
            // Process received data
            uint8_t *data = malloc(ctxt->om->om_len);
            if (data == NULL) {
                return BLE_ATT_ERR_INSUFFICIENT_RES;
            }
            
            ble_hs_mbuf_to_flat(ctxt->om, data, ctxt->om->om_len, NULL);
            
            // Process through data processing module
            if (s_data_handle) {
                data_process_receive(s_data_handle, data, ctxt->om->om_len);
            }
            
            free(data);
            return 0;
        }
    }

    return BLE_ATT_ERR_UNLIKELY;
}

/**
 * @brief BLE GATT service definition
 */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(BLE_SERVICE_UUID),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(BLE_CHAR_RX_UUID),
                .access_cb = ble_gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            {
                .uuid = BLE_UUID16_DECLARE(BLE_CHAR_TX_UUID),
                .access_cb = ble_gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            },
            {
                0,  // No more characteristics
            }
        },
    },
    {
        0,  // No more services
    },
};

/**
 * @brief BLE sync callback
 */
static void ble_on_sync(void)
{
    int rc;

    ESP_LOGI(TAG, "BLE host synchronized");

    // Set device name
    rc = ble_svc_gap_device_name_set(BLE_DEVICE_NAME);
    assert(rc == 0);

    // Start advertising
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;

    memset(&fields, 0, sizeof(fields));
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.uuids128 = NULL;
    fields.uuids128_is_complete = 0;
    fields.uuids16 = NULL;
    fields.uuids16_is_complete = 0;
    fields.uuids32 = NULL;
    fields.uuids32_is_complete = 0;
    fields.name = (uint8_t *)BLE_DEVICE_NAME;
    fields.name_len = strlen(BLE_DEVICE_NAME);
    fields.name_is_complete = 1;
    fields.tx_pwr_lvl_is_present = 0;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.slave_itvl_range = NULL;
    fields.adv_itvl_min = BLE_GAP_ADV_FAST_INTERVAL1_MIN;
    fields.adv_itvl_max = BLE_GAP_ADV_FAST_INTERVAL1_MAX;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error setting advertisement data: %d", rc);
        return;
    }

    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min = BLE_GAP_ADV_FAST_INTERVAL1_MIN;
    adv_params.itvl_max = BLE_GAP_ADV_FAST_INTERVAL1_MAX;

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                          &adv_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error enabling advertisement: %d", rc);
        return;
    }

    ESP_LOGI(TAG, "BLE advertising started");
}

/**
 * @brief BLE reset callback
 */
static void ble_on_reset(int reason)
{
    ESP_LOGE(TAG, "BLE reset: reason=%d", reason);
}

// Forward declaration
static int ble_gap_event(struct ble_gap_event *event, void *arg);

/**
 * @brief BLE GAP event handler
 */
static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    int rc;

    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(TAG, "BLE connection %s; status=%d",
                    event->connect.status == 0 ? "established" : "failed",
                    event->connect.status);
            
            if (event->connect.status == 0) {
                s_conn_handle = event->connect.conn_handle;
                rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
                assert(rc == 0);
                ESP_LOGI(TAG, "Connection parameters: interval=%d, latency=%d, timeout=%d",
                        desc.conn_itvl, desc.conn_latency, desc.supervision_timeout);
            } else {
                s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "BLE disconnect: reason=%d", event->disconnect.reason);
            s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            
            // Restart advertising
            ble_on_sync();
            break;

        case BLE_GAP_EVENT_CONN_UPDATE:
            ESP_LOGI(TAG, "BLE connection updated");
            rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
            assert(rc == 0);
            ESP_LOGI(TAG, "Connection parameters: interval=%d, latency=%d, timeout=%d",
                    desc.conn_itvl, desc.conn_latency, desc.supervision_timeout);
            break;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(TAG, "BLE advertising complete");
            break;

        default:
            break;
    }

    return 0;
}

/**
 * @brief BLE host task
 */
static void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE host task started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/**
 * @brief Initialize BLE task
 * 
 * Original: sub_4201000E initialization
 */
esp_err_t ble_task_init(void)
{
    int rc;

    ESP_LOGI(TAG, "Initializing BLE");

    // Initialize NimBLE
    ESP_ERROR_CHECK(esp_nimble_hci_and_controller_init());

    nimble_port_init();

    // Initialize GATT server
    ble_svc_gap_init();
    ble_svc_gatt_init();

    // Register GATT services
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to count GATT services: %d", rc);
        return ESP_FAIL;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to add GATT services: %d", rc);
        return ESP_FAIL;
    }

    // Get characteristic handles
    ble_uuid16_t rx_uuid = BLE_UUID16_INIT(BLE_CHAR_RX_UUID);
    ble_uuid16_t tx_uuid = BLE_UUID16_INIT(BLE_CHAR_TX_UUID);
    
    rc = ble_gatts_find_chr(&rx_uuid.u, NULL, NULL, &s_char_rx_handle);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to find RX characteristic: %d", rc);
    }
    
    rc = ble_gatts_find_chr(&tx_uuid.u, NULL, NULL, &s_char_tx_handle);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to find TX characteristic: %d", rc);
    }

    // Set callbacks
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.gatts_register_cb = NULL;
    ble_hs_cfg.store_status_cb = NULL;

    // Create data processing handle
    s_data_handle = data_process_create(
        ble_send_callback,
        ble_receive_callback
    );
    if (s_data_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create data process handle");
        return ESP_FAIL;
    }

    // Start BLE host task
    nimble_port_freertos_init(ble_host_task);

    ESP_LOGI(TAG, "BLE task initialized");
    return ESP_OK;
}

