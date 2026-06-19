@echo off
setlocal

set OUTPUT_RECORD=%TEMP%\c_elevator_state_machine_path.txt

if not exist "%OUTPUT_RECORD%" (
    echo Build record not found: %OUTPUT_RECORD%
    echo Please run build.bat first.
    pause
    exit /b 1
)

set /p APP=<"%OUTPUT_RECORD%"

if not exist "%APP%" (
    echo Program not found: %APP%
    echo Please run build.bat again.
    pause
    exit /b 1
)

"%APP%"
pause
