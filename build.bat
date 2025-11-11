@echo off
REM Build script for ESP32ModbusBridge (Windows)
REM Detects ESP-IDF installation and builds the firmware

setlocal enabledelayedexpansion

echo ESP32ModbusBridge Build Script
echo ==================================

REM Get target chip from argument or use default
set TARGET=esp32
if not "%~1"=="" set TARGET=%~1

echo Target chip: %TARGET%

REM Change to script directory
cd /d "%~dp0"

REM Check for Python
where python >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: Python not found. Please install Python 3.6 or later.
    exit /b 1
)

REM Detect ESP-IDF installation
if "%IDF_PATH%"=="" (
    REM Try common installation locations
    if exist "%USERPROFILE%\esp\esp-idf" (
        set "IDF_PATH=%USERPROFILE%\esp\esp-idf"
        echo Found ESP-IDF at: %IDF_PATH%
    ) else if exist "C:\esp\esp-idf" (
        set "IDF_PATH=C:\esp\esp-idf"
        echo Found ESP-IDF at: %IDF_PATH%
    ) else if exist "%USERPROFILE%\.espressif\esp-idf" (
        set "IDF_PATH=%USERPROFILE%\.espressif\esp-idf"
        echo Found ESP-IDF at: %IDF_PATH%
    ) else (
        echo Error: ESP-IDF not found.
        echo Please set IDF_PATH environment variable or install ESP-IDF.
        echo Installation guide: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/
        exit /b 1
    )
) else (
    echo Using ESP-IDF from IDF_PATH: %IDF_PATH%
)

REM Verify IDF_PATH exists
if not exist "%IDF_PATH%" (
    echo Error: IDF_PATH directory does not exist: %IDF_PATH%
    exit /b 1
)

REM Check if ESP-IDF is properly set up
if not exist "%IDF_PATH%\export.bat" (
    echo Error: ESP-IDF installation appears incomplete at: %IDF_PATH%
    exit /b 1
)

REM Set up ESP-IDF environment
echo Setting up ESP-IDF environment...
call "%IDF_PATH%\export.bat" >nul 2>&1

REM Verify idf.py is available
where idf.py >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: idf.py not found. Please ensure ESP-IDF is properly installed.
    echo Try running: %IDF_PATH%\export.bat
    exit /b 1
)

REM Set target
echo Setting target to %TARGET%...
idf.py set-target %TARGET%
if %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to set target
    exit /b 1
)

REM Build the project
echo Building project...
idf.py build
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

REM Build was successful
echo.
echo Build successful!
echo.
echo Build artifacts:
echo   - Bootloader: build\bootloader\bootloader.bin
echo   - Partition table: build\partition_table\partition-table.bin

REM Find main application binary
for %%f in (build\*.bin) do (
    if /i not "%%~nxf"=="bootloader.bin" (
        if /i not "%%~nxf"=="partition-table.bin" (
            echo   - Application: %%f
            goto :found
        )
    )
)
:found

echo.
echo To flash the firmware:
echo   idf.py -p COM3 flash monitor
echo.
echo Or use the flash script:
echo   flash.bat COM3

endlocal

