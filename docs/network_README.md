# Network Modules

## Note on Stub Files

The files in this directory (`tcp_client.c/h`, `tcp_server.c/h`, `tls_client.c/h`, `tls_server.c/h`) are **intentionally minimal wrappers**.

### Why They Exist

These files are included in the build system for potential future use or compatibility, but the actual network functionality is implemented in the task files:

- **TCP Client**: Implemented in `src/tasks/tcp_client_task.c`
- **TCP Server**: Implemented in `src/tasks/tcp_server_task.c`
- **TLS functionality**: Integrated directly in the TCP client/server tasks

### Current Status

- `tcp_client_init()` - Returns ESP_OK (not called anywhere)
- `tcp_server_init()` - Returns ESP_OK (not called anywhere)
- `tls_client_init()` - Returns ESP_OK (not called anywhere)
- `tls_server_init()` - Returns ESP_OK (not called anywhere)

### Future Use

If you need to add a network abstraction layer in the future, these files provide the structure. For now, all TCP/TLS operations are handled directly in the task implementations using ESP-IDF's `lwip` and `esp_tls` APIs.

