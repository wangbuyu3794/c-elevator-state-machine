# Manual Verification Scenarios

This folder contains repeatable input scripts for the command-line elevator
program.

Before running a scenario, build the program from the project root:

```cmd
build.bat
```

Then run one scenario:

```cmd
run.bat < scenarios\01_single_car_request.txt
```

The scripts are not full automatic tests yet. They are teaching-friendly
verification scenarios: run them, read the log, and check the expected behavior.

To run all scenarios at once from the project root:

```cmd
run_scenarios.bat
```

The batch script builds the program, runs every `.txt` scenario in this folder,
and writes logs to a temporary directory. It checks whether each scenario process
finishes successfully, but it does not compare log contents yet.

## 01_single_car_request.txt

Purpose:

- Verify a basic car floor request.
- Verify movement, arrival, door open, door hold, door close, and final idle.

Expected observations:

- A car request for floor `5` is accepted.
- The elevator moves upward from floor `1` to floor `5`.
- The request is cleared after service.
- The final compact panel shows no target and no active request.

## 02_multiple_direction_requests.txt

Purpose:

- Verify multiple pending requests.
- Observe direction-aware scheduling.

Expected observations:

- A hall up request at floor `6` is accepted.
- A car request for floor `8` is accepted.
- A hall down request at floor `3` is accepted.
- From floor `1`, the elevator should choose an upward task first.
- The elevator should serve the `6` floor hall up request on the way to the `8` floor car request.
- The `3` floor hall down request should wait until the elevator reverses direction.
- The final statistics should show completed requests.

## 03_overload_blocks_run.txt

Purpose:

- Verify that overload is treated as a safety condition.

Expected observations:

- Load is set to `1200` kg.
- The elevator should refuse normal movement while overloaded.
- The visual run should stop with a safety message.
- After load is reset to `500` kg, the elevator can continue.

## 04_door_blocked_reopen.txt

Purpose:

- Verify door blocked handling.

Expected observations:

- Door blocked is set before a request is served.
- When the elevator tries to close the door, the blocked state should force a
  safe response instead of normal movement completion.
- After clearing the blocked state, the elevator can continue.

## 05_backup_power_rescue.txt

Purpose:

- Verify sudden power failure and backup power rescue.

Expected observations:

- The elevator starts moving toward floor `24`.
- A sudden power failure is simulated.
- The elevator is marked as between floors before backup rescue.
- Backup power is available.
- Backup rescue moves the elevator to a safe rescue floor and opens the doors.

## 06_emergency_call_blocks_run.txt

Purpose:

- Verify emergency call priority.

Expected observations:

- Emergency call is activated before normal movement.
- Normal movement should be blocked while emergency call is active.
- After clearing emergency call, the elevator can continue processing requests.
