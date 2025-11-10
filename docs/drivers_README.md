# Driver Modules

## Note on Stub Files

The files in this directory (`uart_driver.c/h` and `rs485_driver.c/h`) are **intentionally minimal wrappers**.

### Why They Exist

These files are included in the build system for potential future use or compatibility, but the actual driver functionality is implemented directly in the task files:

- **UART functionality**: Implemented in `src/tasks/uart_rx_task.c` and `src/tasks/rs485_task.c`
- **RS485 functionality**: Implemented in `src/tasks/rs485_task.c`

### Current Status

- `uart_driver_init()` - Returns ESP_OK (not called anywhere)
- `rs485_driver_init()` - Returns ESP_OK (not called anywhere)

### Future Use

If you need to add a driver abstraction layer in the future, these files provide the structure. For now, all UART/RS485 operations use ESP-IDF's `driver/uart.h` directly in the task implementations.

