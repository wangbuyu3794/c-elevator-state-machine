@echo off
setlocal

set DLL=%TEMP%\c_elevator_state_machine_gui.dll

echo Building GUI bridge DLL...

gcc -Wall -Wextra -shared -Iinclude ^
    src\elevator.c ^
    src\elevator_request.c ^
    src\elevator_door.c ^
    src\elevator_power.c ^
    src\elevator_safety.c ^
    src\elevator_status.c ^
    src\elevator_names.c ^
    src\elevator_debug.c ^
    -o "%DLL%"

if errorlevel 1 (
    echo GUI DLL build failed.
    pause
    exit /b 1
)

echo GUI DLL build succeeded: %DLL%
python ui\elevator_web.py "%DLL%"

if errorlevel 1 (
    echo GUI failed to start.
    pause
    exit /b 1
)

pause
exit /b 0
