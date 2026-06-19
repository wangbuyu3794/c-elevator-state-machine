#include "elevator.h"

#include <stdio.h>

static void Elevator_RunIdleStep(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    elevator->targetFloor = NO_TARGET_FLOOR;
    elevator->direction = DIRECTION_NONE;
    elevator->state = ELEVATOR_IDLE;
    elevator->idleTimeSeconds += IDLE_STEP_SECONDS;
    elevator->totalTimeSeconds += IDLE_STEP_SECONDS;

    printf("[Idle] No pending requests. Idle time %ds/%ds.\n",
           elevator->idleTimeSeconds,
           IDLE_RETURN_DELAY_SECONDS);

    if (elevator->currentFloor != LOBBY_FLOOR &&
        elevator->idleTimeSeconds >= IDLE_RETURN_DELAY_SECONDS)
    {
        elevator->targetFloor = LOBBY_FLOOR;
        Elevator_UpdateDirection(elevator);
        printf("[Idle] Returning to lobby floor %d for standby.\n", LOBBY_FLOOR);

        while (elevator->currentFloor != LOBBY_FLOOR)
        {
            if (!Elevator_CanMove(elevator))
            {
                printf("[Safety] Lobby return stopped by safety status.\n");
                return;
            }

            Elevator_MoveOneFloor(elevator);
        }

        elevator->targetFloor = NO_TARGET_FLOOR;
        elevator->direction = DIRECTION_NONE;
        elevator->state = ELEVATOR_IDLE;
        Elevator_ResetIdleTimer(elevator);
        printf("[Idle] Elevator is now standing by at lobby floor %d.\n", LOBBY_FLOOR);
    }
}

void Elevator_UpdateSafetyState(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    if (elevator->isPowerOff)
    {
        elevator->state = ELEVATOR_POWER_OFF;
        elevator->direction = DIRECTION_NONE;
        elevator->targetFloor = NO_TARGET_FLOOR;
        return;
    }

    if (elevator->isRecovering)
    {
        elevator->state = ELEVATOR_RECOVERING;
        elevator->direction = DIRECTION_NONE;
        return;
    }

    if (elevator->isAdminPaused)
    {
        elevator->state = ELEVATOR_PAUSED;
        elevator->direction = DIRECTION_NONE;
        elevator->targetFloor = NO_TARGET_FLOOR;
        return;
    }

    if (elevator->fault != FAULT_NONE)
    {
        elevator->state = ELEVATOR_FAULT;
        elevator->direction = DIRECTION_NONE;
        elevator->targetFloor = NO_TARGET_FLOOR;
        return;
    }

    if (elevator->isEmergencyCallActive)
    {
        elevator->state = ELEVATOR_PAUSED;
        elevator->direction = DIRECTION_NONE;
        elevator->targetFloor = NO_TARGET_FLOOR;
        return;
    }

    if (elevator->isOverloaded || elevator->isDoorBlocked || elevator->isDoorOpenButtonHeld)
    {
        Elevator_ForceDoorsOpenAtCurrentFloor(elevator);
        elevator->state = ELEVATOR_DOOR_HOLDING;
        elevator->direction = DIRECTION_NONE;
        return;
    }

    if (elevator->door == DOOR_OPEN)
    {
        Elevator_CloseDoor(elevator);
    }

    if (elevator->door == DOOR_CLOSED)
    {
        elevator->state = ELEVATOR_IDLE;
    }
}

static void Elevator_FinishStop(Elevator *elevator)
{
    Elevator_RecordCompletedRequest(elevator, elevator->currentFloor);
    Elevator_ClearServedRequestsAtCurrentFloor(elevator);
    Elevator_OpenDoor(elevator);
    Elevator_HoldDoor(elevator);
    Elevator_CloseDoor(elevator);

    elevator->targetFloor = NO_TARGET_FLOOR;

    if (elevator->door != DOOR_CLOSED)
    {
        elevator->direction = DIRECTION_NONE;
        return;
    }

    if (Elevator_HasAnyRequest(elevator))
    {
        elevator->state = ELEVATOR_IDLE;
    }
    else
    {
        elevator->state = ELEVATOR_IDLE;
        elevator->direction = DIRECTION_NONE;
    }
}

void Elevator_Init(Elevator *elevator)
{
    int i;

    if (elevator == NULL)
    {
        return;
    }

    elevator->currentFloor = 1;
    elevator->targetFloor = NO_TARGET_FLOOR;
    elevator->totalTimeSeconds = 0;
    elevator->idleTimeSeconds = 0;
    elevator->state = ELEVATOR_IDLE;
    elevator->direction = DIRECTION_NONE;
    elevator->door = DOOR_CLOSED;
    elevator->carDoor = DOOR_CLOSED;
    elevator->isAlignedWithFloor = 1;
    elevator->fault = FAULT_NONE;
    elevator->currentLoadKg = 0;
    elevator->isOverloaded = 0;
    elevator->isDoorBlocked = 0;
    elevator->isDoorOpenButtonHeld = 0;
    elevator->isEmergencyCallActive = 0;
    elevator->isAdminPaused = 0;
    elevator->isPowerOff = 0;
    elevator->isMainPowerOn = 1;
    elevator->isBackupPowerAvailable = 1;
    elevator->isBackupPowerActive = 0;
    elevator->isPowerFailure = 0;
    elevator->isRecovering = 0;
    elevator->isBetweenFloors = 0;
    elevator->safeFloor = 1;
    elevator->rescueFloor = 1;
    elevator->directionBeforePowerFailure = DIRECTION_NONE;

    for (i = 0; i < TOTAL_FLOOR_COUNT; i++)
    {
        elevator->hallUpRequests[i] = 0;
        elevator->hallDownRequests[i] = 0;
        elevator->carFloorRequests[i] = 0;
        elevator->landingDoors[i] = DOOR_CLOSED;
        elevator->landingDoorLocked[i] = 1;
        elevator->hallUpRequestCreatedAt[i] = 0;
        elevator->hallDownRequestCreatedAt[i] = 0;
        elevator->carRequestCreatedAt[i] = 0;
    }

    elevator->completedRequestCount = 0;
    elevator->totalWaitTimeSeconds = 0;
    elevator->longestWaitTimeSeconds = 0;
}

void Elevator_GetSnapshot(const Elevator *elevator, ElevatorSnapshot *snapshot)
{
    int i;

    if (elevator == NULL || snapshot == NULL)
    {
        return;
    }

    snapshot->currentFloor = elevator->currentFloor;
    snapshot->targetFloor = elevator->targetFloor;
    snapshot->totalTimeSeconds = elevator->totalTimeSeconds;
    snapshot->idleTimeSeconds = elevator->idleTimeSeconds;
    snapshot->state = elevator->state;
    snapshot->direction = elevator->direction;
    snapshot->door = elevator->door;
    snapshot->carDoor = elevator->carDoor;
    snapshot->isAlignedWithFloor = elevator->isAlignedWithFloor;
    snapshot->fault = elevator->fault;

    snapshot->currentLoadKg = elevator->currentLoadKg;
    snapshot->isOverloaded = elevator->isOverloaded;
    snapshot->isDoorBlocked = elevator->isDoorBlocked;
    snapshot->isDoorOpenButtonHeld = elevator->isDoorOpenButtonHeld;
    snapshot->isEmergencyCallActive = elevator->isEmergencyCallActive;
    snapshot->isAdminPaused = elevator->isAdminPaused;
    snapshot->isPowerOff = elevator->isPowerOff;
    snapshot->isMainPowerOn = elevator->isMainPowerOn;
    snapshot->isBackupPowerAvailable = elevator->isBackupPowerAvailable;
    snapshot->isBackupPowerActive = elevator->isBackupPowerActive;
    snapshot->isPowerFailure = elevator->isPowerFailure;
    snapshot->isRecovering = elevator->isRecovering;
    snapshot->isBetweenFloors = elevator->isBetweenFloors;
    snapshot->safeFloor = elevator->safeFloor;
    snapshot->rescueFloor = elevator->rescueFloor;
    snapshot->directionBeforePowerFailure = elevator->directionBeforePowerFailure;
    snapshot->canMove = Elevator_CanMove(elevator);
    snapshot->canCloseDoor = Elevator_CanCloseDoor(elevator);

    for (i = 0; i < TOTAL_FLOOR_COUNT; i++)
    {
        snapshot->hallUpRequests[i] = elevator->hallUpRequests[i];
        snapshot->hallDownRequests[i] = elevator->hallDownRequests[i];
        snapshot->carFloorRequests[i] = elevator->carFloorRequests[i];
        snapshot->landingDoors[i] = elevator->landingDoors[i];
        snapshot->landingDoorLocked[i] = elevator->landingDoorLocked[i];
    }

    snapshot->completedRequestCount = elevator->completedRequestCount;
    snapshot->totalWaitTimeSeconds = elevator->totalWaitTimeSeconds;
    snapshot->longestWaitTimeSeconds = elevator->longestWaitTimeSeconds;
}

int Elevator_IsValidFloor(int floor)
{
    return floor == MIN_FLOOR || (floor >= 1 && floor <= MAX_FLOOR);
}

int Elevator_FloorToIndex(int floor)
{
    if (!Elevator_IsValidFloor(floor))
    {
        return -1;
    }

    if (floor == MIN_FLOOR)
    {
        return 0;
    }

    return floor;
}

int Elevator_IndexToFloor(int index)
{
    if (index < 0 || index >= TOTAL_FLOOR_COUNT)
    {
        return NO_TARGET_FLOOR;
    }

    if (index == 0)
    {
        return MIN_FLOOR;
    }

    return index;
}

void Elevator_UpdateDirection(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    if (elevator->targetFloor == NO_TARGET_FLOOR ||
        elevator->targetFloor == elevator->currentFloor)
    {
        elevator->direction = DIRECTION_NONE;
    }
    else if (elevator->targetFloor > elevator->currentFloor)
    {
        elevator->direction = DIRECTION_UP;
    }
    else
    {
        elevator->direction = DIRECTION_DOWN;
    }
}

void Elevator_MoveOneFloor(Elevator *elevator)
{
    if (elevator == NULL || elevator->targetFloor == NO_TARGET_FLOOR)
    {
        return;
    }

    if (!Elevator_CanMove(elevator))
    {
        printf("[Safety] Elevator cannot move now. Check safety status.\n");
        return;
    }

    Elevator_UpdateDirection(elevator);

    if (elevator->direction == DIRECTION_UP)
    {
        elevator->currentFloor++;
    }
    else if (elevator->direction == DIRECTION_DOWN)
    {
        elevator->currentFloor--;
    }

    if (elevator->currentFloor == 0)
    {
        elevator->currentFloor += elevator->direction == DIRECTION_UP ? 1 : -1;
    }

    elevator->state = ELEVATOR_MOVING;
    elevator->isAlignedWithFloor = 0;
    elevator->carDoor = DOOR_CLOSED;
    elevator->door = DOOR_CLOSED;
    elevator->totalTimeSeconds += MOVE_TIME_PER_FLOOR;
    elevator->isAlignedWithFloor = 1;

    printf("[Move] Arrived at floor %d. Time +%ds, total %ds.\n",
           elevator->currentFloor,
           MOVE_TIME_PER_FLOOR,
           elevator->totalTimeSeconds);
}

void Elevator_RunOneStep(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    if (!Elevator_CanMove(elevator))
    {
        printf("[Safety] Normal scheduling is paused by safety status.\n");
        return;
    }

    if (!Elevator_HasAnyRequest(elevator))
    {
        Elevator_RunIdleStep(elevator);
        return;
    }

    Elevator_ResetIdleTimer(elevator);

    if (Elevator_ShouldServeCurrentFloor(elevator))
    {
        printf("[Arrive] Serving request at floor %d.\n", elevator->currentFloor);
        Elevator_FinishStop(elevator);
        return;
    }

    elevator->targetFloor = Elevator_FindNextTarget(elevator);
    Elevator_UpdateDirection(elevator);

    if (elevator->targetFloor == elevator->currentFloor)
    {
        printf("[Arrive] Serving request at floor %d.\n", elevator->currentFloor);
        Elevator_FinishStop(elevator);
        return;
    }

    Elevator_MoveOneFloor(elevator);

    if (elevator->currentFloor == elevator->targetFloor &&
        Elevator_ShouldServeCurrentFloor(elevator))
    {
        printf("[Arrive] Target floor %d reached.\n", elevator->currentFloor);
        Elevator_FinishStop(elevator);
    }
}

void Elevator_RunUntilIdle(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    while (Elevator_HasAnyRequest(elevator))
    {
        if (!Elevator_CanMove(elevator))
        {
            printf("[Safety] Run stopped before all requests finished.\n");
            return;
        }

        Elevator_RunOneStep(elevator);
    }

    elevator->direction = DIRECTION_NONE;
    elevator->targetFloor = NO_TARGET_FLOOR;

    if (elevator->door == DOOR_CLOSED &&
        !elevator->isOverloaded &&
        !elevator->isDoorBlocked &&
        !elevator->isAdminPaused &&
        elevator->fault == FAULT_NONE)
    {
        elevator->state = ELEVATOR_IDLE;
        printf("[Done] All requests finished. Total time: %ds.\n", elevator->totalTimeSeconds);
    }
    else
    {
        printf("[Safety] Requests finished, but elevator is waiting for safety recovery.\n");
    }
}

int Elevator_CanMove(const Elevator *elevator)
{
    if (elevator == NULL)
    {
        return 0;
    }

    if (elevator->isAdminPaused)
    {
        return 0;
    }

    if (elevator->isEmergencyCallActive)
    {
        return 0;
    }

    if (elevator->isPowerOff || elevator->isRecovering)
    {
        return 0;
    }

    if (elevator->fault != FAULT_NONE)
    {
        return 0;
    }

    if (elevator->isOverloaded)
    {
        return 0;
    }

    if (!elevator->isAlignedWithFloor)
    {
        return 0;
    }

    if (elevator->door != DOOR_CLOSED || elevator->carDoor != DOOR_CLOSED)
    {
        return 0;
    }

    if (!Elevator_AreAllLandingDoorsLocked(elevator))
    {
        return 0;
    }

    return 1;
}

void Elevator_PressEmergencyCallButton(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    elevator->isEmergencyCallActive = 1;
    Elevator_UpdateSafetyState(elevator);
    printf("[Emergency] Emergency call button pressed. Normal scheduling paused.\n");
}

void Elevator_ClearEmergencyCall(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    elevator->isEmergencyCallActive = 0;
    Elevator_UpdateSafetyState(elevator);
    printf("[Emergency] Emergency call cleared.\n");
}

void Elevator_SetLoad(Elevator *elevator, int loadKg)
{
    if (elevator == NULL)
    {
        return;
    }

    if (loadKg < 0)
    {
        loadKg = 0;
    }

    elevator->currentLoadKg = loadKg;
    elevator->isOverloaded = loadKg > MAX_LOAD_KG;

    if (elevator->isOverloaded)
    {
        Elevator_ForceDoorsOpenAtCurrentFloor(elevator);
        printf("[Safety] Overload: %d kg. Door stays open.\n", loadKg);
    }
    else
    {
        printf("[Safety] Load set to %d kg.\n", loadKg);
    }

    Elevator_UpdateSafetyState(elevator);
}

void Elevator_SetFault(Elevator *elevator, FaultType fault)
{
    if (elevator == NULL)
    {
        return;
    }

    elevator->fault = fault;
    if (fault == FAULT_NONE)
    {
        printf("[Fault] Fault cleared.\n");
        Elevator_UpdateSafetyState(elevator);
        return;
    }

    printf("[Fault] Elevator entered fault state: %s.\n", Elevator_GetFaultName(fault));
    Elevator_UpdateSafetyState(elevator);
}

void Elevator_ClearFault(Elevator *elevator)
{
    Elevator_SetFault(elevator, FAULT_NONE);
}

void Elevator_AdminPause(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    elevator->isAdminPaused = 1;
    Elevator_UpdateSafetyState(elevator);
    printf("[Admin] Elevator paused by administrator.\n");
}

void Elevator_AdminResume(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    elevator->isAdminPaused = 0;
    Elevator_UpdateSafetyState(elevator);
    printf("[Admin] Elevator resumed by administrator.\n");
}
