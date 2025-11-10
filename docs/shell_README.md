# Shell Modules

## Note on Stub Files

The files `command_parser.c/h` and `command_handlers.c/h` are **intentionally minimal wrappers**.

### Why They Exist

These files are included in the build system for potential future use, but the actual command parsing and handling is implemented in:

- **Command Parsing**: Implemented in `src/shell/terminal_service.c` (function `terminal_process_command`)
- **Command Handlers**: Implemented in `src/shell/terminal_service.c` (functions `cmd_lpts1` through `cmd_shell`)

### Current Status

- `command_parser_init()` - Returns ESP_OK (not called anywhere)
- `command_handlers_init()` - Returns ESP_OK (not called anywhere)

### Implementation

All terminal commands are handled directly in `terminal_service.c`:
- LPTS1-7: Set parameter commands
- LPTQ1-7: Query parameter commands
- SHELL: Shell mode enable

### Future Use

If you need to refactor command parsing into a separate module in the future, these files provide the structure. For now, all command handling is integrated in the terminal service.

