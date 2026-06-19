#include "elevator.h"

#include <stdio.h>

static int Elevator_GetNextFloorInDirection(int floor, ElevatorDirection direction)
{
    int nextFloor = floor;

    if (direction == DIRECTION_UP)
    {
        nextFloor++;
        if (nextFloor == 0)
        {
            nextFloor = 1;
        }
    }
    else if (direction == DIRECTION_DOWN)
    {
        nextFloor--;
        if (nextFloor == 0)
        {
            nextFloor = MIN_FLOOR;
        }
    }

    if (!Elevator_IsValidFloor(nextFloor))
    {
        return floor;
    }

    return nextFloor;
}

void Elevator_PowerOff(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    elevator->isMainPowerOn = 0;
    elevator->isPowerOff = 1;
    elevator->isPowerFailure = 0;
    elevator->isBackupPowerActive = 0;
    elevator->isRecovering = 0;
    elevator->direction = DIRECTION_NONE;
    elevator->targetFloor = NO_TARGET_FLOOR;
    Elevator_UpdateSafetyState(elevator);
    printf("[Power] Main power switched off by operator.\n");
}

void Elevator_RestorePower(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    if (elevator->isMainPowerOn && !elevator->isPowerOff)
    {
        printf("[Power] Power is already on.\n");
        return;
    }

    elevator->isMainPowerOn = 1;
    elevator->isPowerOff = 0;
    elevator->isPowerFailure = 0;
    elevator->isBackupPowerActive = 0;
    elevator->isRecovering = 1;
    elevator->direction = DIRECTION_NONE;
    elevator->targetFloor = NO_TARGET_FLOOR;
    printf("[Power] Power restored. Starting safety recovery.\n");
    Elevator_RunRecovery(elevator);
}

void Elevator_SetBackupPowerAvailable(Elevator *elevator, int isAvailable)
{
    if (elevator == NULL)
    {
        return;
    }

    elevator->isBackupPowerAvailable = isAvailable ? 1 : 0;
    printf("[Power] Backup power available: %s.\n",
           elevator->isBackupPowerAvailable ? "yes" : "no");
}

void Elevator_SimulatePowerFailure(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    if (elevator->direction == DIRECTION_NONE && elevator->targetFloor != NO_TARGET_FLOOR)
    {
        Elevator_UpdateDirection(elevator);
    }

    elevator->directionBeforePowerFailure = elevator->direction;
    elevator->isMainPowerOn = 0;
    elevator->isPowerOff = 1;
    elevator->isPowerFailure = 1;
    elevator->isRecovering = 0;
    elevator->targetFloor = NO_TARGET_FLOOR;
    elevator->direction = DIRECTION_NONE;
    Elevator_UpdateSafetyState(elevator);

    printf("[Power] Sudden power failure. Normal scheduling stopped.\n");

    if (elevator->isBetweenFloors)
    {
        if (elevator->directionBeforePowerFailure != DIRECTION_NONE)
        {
            elevator->rescueFloor = Elevator_GetNextFloorInDirection(
                elevator->currentFloor,
                elevator->directionBeforePowerFailure);
            elevator->safeFloor = elevator->rescueFloor;
        }

        if (elevator->isBackupPowerAvailable)
        {
            Elevator_RunBackupRescue(elevator);
        }
        else
        {
            printf("[Power] Backup power unavailable. Elevator is waiting for external rescue.\n");
        }
    }
}

void Elevator_RunBackupRescue(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    if (!elevator->isPowerFailure)
    {
        printf("[Backup] No power failure rescue is needed now.\n");
        return;
    }

    if (!elevator->isBetweenFloors)
    {
        printf("[Backup] Elevator is already aligned with a floor.\n");
        return;
    }

    if (!elevator->isBackupPowerAvailable)
    {
        printf("[Backup] Backup power is unavailable.\n");
        return;
    }

    if (!Elevator_IsValidFloor(elevator->rescueFloor))
    {
        printf("[Backup] Rescue floor %d is invalid.\n", elevator->rescueFloor);
        return;
    }

    elevator->isBackupPowerActive = 1;
    elevator->state = ELEVATOR_RECOVERING;
    printf("[Backup] Backup power active. Moving slowly to rescue floor %d.\n",
           elevator->rescueFloor);

    elevator->currentFloor = elevator->rescueFloor;
    elevator->safeFloor = elevator->rescueFloor;
    elevator->totalTimeSeconds += RECOVERY_MOVE_TIME;
    elevator->isBetweenFloors = 0;
    elevator->isAlignedWithFloor = 1;

    printf("[Backup] Reached rescue floor %d. Time +%ds, total %ds.\n",
           elevator->currentFloor,
           RECOVERY_MOVE_TIME,
           elevator->totalTimeSeconds);

    Elevator_OpenDoor(elevator);
    Elevator_HoldDoor(elevator);
    Elevator_ClearAllCarRequests(elevator);

    elevator->isBackupPowerActive = 0;
    elevator->isRecovering = 0;
    elevator->state = ELEVATOR_POWER_OFF;
    elevator->direction = DIRECTION_NONE;
    elevator->targetFloor = NO_TARGET_FLOOR;
    printf("[Backup] Passengers can leave. Elevator waits here until main power is restored.\n");
}

void Elevator_SetBetweenFloors(Elevator *elevator, int safeFloor)
{
    if (elevator == NULL)
    {
        return;
    }

    if (!Elevator_IsValidFloor(safeFloor))
    {
        printf("[Recovery] Safe floor %d is invalid.\n", safeFloor);
        return;
    }

    elevator->isBetweenFloors = 1;
    elevator->isAlignedWithFloor = 0;
    elevator->safeFloor = safeFloor;
    elevator->rescueFloor = safeFloor;
    printf("[Recovery] Elevator is now simulated between floors. Nearest safe floor: %d.\n",
           safeFloor);
}

void Elevator_RunRecovery(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    if (elevator->isPowerOff)
    {
        printf("[Recovery] Cannot recover while power is off.\n");
        return;
    }

    if (!elevator->isRecovering && !elevator->isBetweenFloors)
    {
        printf("[Recovery] No recovery is needed now.\n");
        return;
    }

    elevator->isRecovering = 1;
    elevator->state = ELEVATOR_RECOVERING;
    elevator->direction = DIRECTION_NONE;
    elevator->targetFloor = NO_TARGET_FLOOR;

    printf("[Recovery] Running safety self-check.\n");

    if (elevator->isBetweenFloors)
    {
        printf("[Recovery] Elevator is between floors. Moving slowly to safe floor %d.\n",
               elevator->safeFloor);
        elevator->currentFloor = elevator->safeFloor;
        elevator->totalTimeSeconds += RECOVERY_MOVE_TIME;
        elevator->isBetweenFloors = 0;
        elevator->isAlignedWithFloor = 1;
        printf("[Recovery] Reached safe floor %d. Time +%ds, total %ds.\n",
               elevator->currentFloor,
               RECOVERY_MOVE_TIME,
               elevator->totalTimeSeconds);
    }
    else
    {
        printf("[Recovery] Elevator is already at safe floor %d.\n", elevator->currentFloor);
    }

    Elevator_OpenDoor(elevator);
    Elevator_HoldDoor(elevator);
    Elevator_CloseDoor(elevator);

    elevator->isRecovering = 0;
    Elevator_UpdateSafetyState(elevator);

    if (Elevator_CanMove(elevator))
    {
        printf("[Recovery] Safety recovery finished. Elevator can return to normal scheduling.\n");
    }
    else
    {
        printf("[Recovery] Recovery finished, but another safety condition still blocks movement.\n");
    }
}
