/**
 * @file data_process.h
 * @brief Data processing module
 * 
 * This module handles protocol parsing and data processing.
 * Original functions:
 * - sub_420116AA -> data_process_create
 * - sub_42010E36 -> data_process_receive
 * - sub_42011012 -> data_process_send
 * - sub_42013626 -> parse_data_transmission_frame
 * - sub_4201364E -> parse_set_param_frame
 * - sub_4201368C -> parse_get_param_frame
 */

#ifndef DATA_PROCESS_H
#define DATA_PROCESS_H

#include "esp_err.h"
#include "function_codes.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct data_process_handle* data_process_handle_t;

/**
 * @brief Create data processing module
 * 
 * Original: sub_420116AA
 */
data_process_handle_t data_process_create(void (*send_callback)(const uint8_t *data, size_t len),
                                          void (*receive_callback)(const uint8_t *data, size_t len));

/**
 * @brief Receive and process data
 * 
 * Original: sub_42010E36
 */
esp_err_t data_process_receive(data_process_handle_t handle, const uint8_t *data, size_t len);

/**
 * @brief Send data
 * 
 * Original: sub_42011012
 */
esp_err_t data_process_send(data_process_handle_t handle, uint8_t func_code, const uint8_t *data, size_t len);

/**
 * @brief Destroy data processing module
 * 
 * @param handle Handle to destroy
 */
void data_process_destroy(data_process_handle_t handle);

/**
 * @brief Parse data transmission frame
 * 
 * Original: sub_42013626
 */
int parse_data_transmission_frame(const uint8_t *frame, size_t frame_len, 
                                  uint8_t **data_out, uint16_t *data_len);

/**
 * @brief Parse set parameter frame
 * 
 * Original: sub_4201364E
 */
int parse_set_param_frame(const uint8_t *frame, size_t frame_len,
                          uint16_t *param_id, uint16_t *data_len, uint8_t **data_out);

/**
 * @brief Parse get parameter frame
 * 
 * Original: sub_4201368C
 */
int parse_get_param_frame(const uint8_t *frame, size_t frame_len,
                          uint16_t *param_id, uint16_t *expected_len);

#ifdef __cplusplus
}
#endif

#endif // DATA_PROCESS_H

