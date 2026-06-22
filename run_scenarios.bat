@echo off
setlocal EnableDelayedExpansion

set APP=%TEMP%\c_elevator_state_machine_scenarios.exe
set LOG_DIR=%TEMP%\c_elevator_state_machine_scenario_logs

if not exist "%LOG_DIR%" (
    mkdir "%LOG_DIR%"
)

echo Building scenario runner...

gcc -Wall -Wextra -Iinclude ^
    src\main.c ^
    src\elevator.c ^
    src\elevator_request.c ^
    src\elevator_door.c ^
    src\elevator_power.c ^
    src\elevator_safety.c ^
    src\elevator_status.c ^
    src\elevator_names.c ^
    src\elevator_debug.c ^
    -o "%APP%"

if errorlevel 1 (
    echo Build failed.
    pause
    exit /b 1
)

echo Build succeeded: %APP%
echo Logs will be written to: %LOG_DIR%
echo.

set TOTAL=0
set FAILED=0

for %%F in (scenarios\*.txt) do (
    set /a TOTAL+=1
    echo [RUN] %%F
    "%APP%" < "%%F" > "%LOG_DIR%\%%~nF.log" 2>&1

    if errorlevel 1 (
        set /a FAILED+=1
        echo [FAIL] %%F
        echo        See: %LOG_DIR%\%%~nF.log
    ) else (
        echo [PASS] %%F
    )
    echo.
)

echo Scenario summary:
echo   Total : !TOTAL!
echo   Failed: !FAILED!
echo   Logs  : %LOG_DIR%

if not "!FAILED!"=="0" (
    pause
    exit /b 1
)

echo All scenarios finished without process errors.
pause
exit /b 0
