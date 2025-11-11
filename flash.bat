@echo off
REM Flash script for ESP32ModbusBridge (Windows)
REM Flashes the built firmware to ESP32 over USB

setlocal enabledelayedexpansion

echo ESP32ModbusBridge Flash Script
echo ==================================

REM Get port from argument or use default
set PORT=COM3
if not "%~1"=="" set PORT=%~1

set CHIP=esp32
if not "%~2"=="" set CHIP=%~2

echo Port: %PORT%
echo Chip: %CHIP%

REM Change to script directory
cd /d "%~dp0"

REM Check if build directory exists
if not exist "build" (
    echo Error: Build directory not found. Please run build.bat first.
    exit /b 1
)

REM Detect ESP-IDF installation
if "%IDF_PATH%"=="" (
    REM Try common installation locations
    if exist "%USERPROFILE%\esp\esp-idf" (
        set "IDF_PATH=%USERPROFILE%\esp\esp-idf"
    ) else if exist "C:\esp\esp-idf" (
        set "IDF_PATH=C:\esp\esp-idf"
    ) else if exist "%USERPROFILE%\.espressif\esp-idf" (
        set "IDF_PATH=%USERPROFILE%\.espressif\esp-idf"
    ) else (
        echo Error: ESP-IDF not found. Please set IDF_PATH environment variable.
        exit /b 1
    )
)

REM Set up ESP-IDF environment
call "%IDF_PATH%\export.bat" >nul 2>&1

REM Find main application binary
set MAIN_BIN=
for %%f in (build\*.bin) do (
    if /i not "%%~nxf"=="bootloader.bin" (
        if /i not "%%~nxf"=="partition-table.bin" (
            set "MAIN_BIN=%%f"
            goto :found
        )
    )
)

:found
if "%MAIN_BIN%"=="" (
    echo Error: Main application binary not found in build directory.
    exit /b 1
)

echo Flashing firmware to %PORT%...
echo.

REM Flash using esptool.py
python "%IDF_PATH%\components\esptool_py\esptool\esptool.py" ^
  --chip %CHIP% ^
  --port %PORT% ^
  --baud 921600 ^
  --before default_reset ^
  --after hard_reset ^
  write_flash ^
  -z --flash_mode dio --flash_freq 40m --flash_size 4MB ^
  0x1000 build\bootloader\bootloader.bin ^
  0x8000 build\partition_table\partition-table.bin ^
  0x10000 "%MAIN_BIN%"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Flash complete!
    echo.
    echo To monitor serial output:
    echo   idf.py -p %PORT% monitor
    echo.
    echo Or press Ctrl+] to exit monitor
) else (
    echo Flash failed!
    exit /b 1
)

endlocal

