@echo off
setlocal

set OUTPUT=%TEMP%\c_elevator_state_machine.exe

gcc -Wall -Wextra -Iinclude ^
    src\main.c ^
    src\elevator.c ^
    src\elevator_request.c ^
    src\elevator_door.c ^
    src\elevator_power.c ^
    src\elevator_safety.c ^
    src\elevator_status.c ^
    src\elevator_names.c ^
    -o "%OUTPUT%"

if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

echo Build succeeded: %OUTPUT%
