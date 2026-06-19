#include "elevator.h"

#include <stdio.h>
static int Elevator_Abs(int value)
{
    return value < 0 ? -value : value;
}

static int Elevator_HasRequestAtIndex(const Elevator *elevator, int index)
{
    if (elevator == NULL || index < 0 || index >= TOTAL_FLOOR_COUNT)
    {
        return 0;
    }

    return elevator->hallUpRequests[index] ||
           elevator->hallDownRequests[index] ||
           elevator->carFloorRequests[index];
}

static int Elevator_GetCurrentFloorIndex(const Elevator *elevator)
{
    if (elevator == NULL)
    {
        return -1;
    }

    return Elevator_FloorToIndex(elevator->currentFloor);
}

static void Elevator_UpdateOverallDoorState(Elevator *elevator)
{
    int index;

    if (elevator == NULL)
    {
        return;
    }

    index = Elevator_GetCurrentFloorIndex(elevator);
    if (index >= 0 && elevator->landingDoors[index] == DOOR_OPEN)
    {
        elevator->door = DOOR_OPEN;
        return;
    }

    elevator->door = elevator->carDoor;
}

static void Elevator_ForceDoorsOpenAtCurrentFloor(Elevator *elevator)
{
    int index;

    if (elevator == NULL)
    {
        return;
    }

    index = Elevator_GetCurrentFloorIndex(elevator);
    if (index < 0 || !elevator->isAlignedWithFloor)
    {
        return;
    }

    elevator->landingDoorLocked[index] = 0;
    elevator->landingDoors[index] = DOOR_OPEN;
    elevator->carDoor = DOOR_OPEN;
    Elevator_UpdateOverallDoorState(elevator);
}

static int Elevator_HasRequestAbove(const Elevator *elevator, int floor)
{
    int i;
    int currentIndex;

    if (elevator == NULL)
    {
        return 0;
    }

    currentIndex = Elevator_FloorToIndex(floor);
    if (currentIndex < 0)
    {
        return 0;
    }

    for (i = currentIndex + 1; i < TOTAL_FLOOR_COUNT; i++)
    {
        if (Elevator_HasRequestAtIndex(elevator, i))
        {
            return 1;
        }
    }

    return 0;
}

static int Elevator_HasRequestBelow(const Elevator *elevator, int floor)
{
    int i;
    int currentIndex;

    if (elevator == NULL)
    {
        return 0;
    }

    currentIndex = Elevator_FloorToIndex(floor);
    if (currentIndex < 0)
    {
        return 0;
    }

    for (i = currentIndex - 1; i >= 0; i--)
    {
        if (Elevator_HasRequestAtIndex(elevator, i))
        {
            return 1;
        }
    }

    return 0;
}

static int Elevator_ShouldServeCurrentFloor(const Elevator *elevator)
{
    int index;

    if (elevator == NULL)
    {
        return 0;
    }

    index = Elevator_FloorToIndex(elevator->currentFloor);
    if (index < 0)
    {
        return 0;
    }

    if (elevator->carFloorRequests[index])
    {
        return 1;
    }

    if (elevator->direction == DIRECTION_UP)
    {
        if (elevator->hallUpRequests[index])
        {
            return 1;
        }

        return elevator->hallDownRequests[index] &&
               !Elevator_HasRequestAbove(elevator, elevator->currentFloor);
    }

    if (elevator->direction == DIRECTION_DOWN)
    {
        if (elevator->hallDownRequests[index])
        {
            return 1;
        }

        return elevator->hallUpRequests[index] &&
               !Elevator_HasRequestBelow(elevator, elevator->currentFloor);
    }

    return Elevator_HasRequestAtIndex(elevator, index);
}

static int Elevator_ClearServedRequestsAtCurrentFloor(Elevator *elevator)
{
    int index;
    int cleared = 0;

    if (elevator == NULL)
    {
        return 0;
    }

    index = Elevator_FloorToIndex(elevator->currentFloor);
    if (index < 0)
    {
        return 0;
    }

    if (elevator->carFloorRequests[index])
    {
        elevator->carFloorRequests[index] = 0;
        elevator->carRequestCreatedAt[index] = 0;
        cleared = 1;
    }

    if (elevator->direction == DIRECTION_UP)
    {
        if (elevator->hallUpRequests[index])
        {
            elevator->hallUpRequests[index] = 0;
            elevator->hallUpRequestCreatedAt[index] = 0;
            cleared = 1;
        }

        if (elevator->hallDownRequests[index] &&
            !Elevator_HasRequestAbove(elevator, elevator->currentFloor))
        {
            elevator->hallDownRequests[index] = 0;
            elevator->hallDownRequestCreatedAt[index] = 0;
            cleared = 1;
        }
    }
    else if (elevator->direction == DIRECTION_DOWN)
    {
        if (elevator->hallDownRequests[index])
        {
            elevator->hallDownRequests[index] = 0;
            elevator->hallDownRequestCreatedAt[index] = 0;
            cleared = 1;
        }

        if (elevator->hallUpRequests[index] &&
            !Elevator_HasRequestBelow(elevator, elevator->currentFloor))
        {
            elevator->hallUpRequests[index] = 0;
            elevator->hallUpRequestCreatedAt[index] = 0;
            cleared = 1;
        }
    }
    else
    {
        if (elevator->hallUpRequests[index])
        {
            elevator->hallUpRequests[index] = 0;
            elevator->hallUpRequestCreatedAt[index] = 0;
            cleared = 1;
        }

        if (elevator->hallDownRequests[index])
        {
            elevator->hallDownRequests[index] = 0;
            elevator->hallDownRequestCreatedAt[index] = 0;
            cleared = 1;
        }
    }

    return cleared;
}

static void Elevator_ResetIdleTimer(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    elevator->idleTimeSeconds = 0;
}

static void Elevator_ClearAllCarRequests(Elevator *elevator)
{
    int i;

    if (elevator == NULL)
    {
        return;
    }

    for (i = 0; i < TOTAL_FLOOR_COUNT; i++)
    {
        elevator->carFloorRequests[i] = 0;
        elevator->carRequestCreatedAt[i] = 0;
    }
}

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

static void Elevator_RecordCompletedRequest(Elevator *elevator, int floor)
{
    int index;
    int waitTime;
    int hasRecordedRequest = 0;

    if (elevator == NULL)
    {
        return;
    }

    index = Elevator_FloorToIndex(floor);
    if (index < 0 || !Elevator_HasRequestAtIndex(elevator, index))
    {
        return;
    }

    if (elevator->hallUpRequests[index] &&
        (elevator->direction == DIRECTION_NONE ||
         elevator->direction == DIRECTION_UP ||
         (elevator->direction == DIRECTION_DOWN &&
          !Elevator_HasRequestBelow(elevator, elevator->currentFloor))))
    {
        waitTime = elevator->totalTimeSeconds - elevator->hallUpRequestCreatedAt[index];
        if (waitTime < 0)
        {
            waitTime = 0;
        }

        elevator->completedRequestCount++;
        elevator->totalWaitTimeSeconds += waitTime;
        if (waitTime > elevator->longestWaitTimeSeconds)
        {
            elevator->longestWaitTimeSeconds = waitTime;
        }
        printf("[Stats] Hall up request at floor %d waited %d seconds before service.\n",
               floor,
               waitTime);
        hasRecordedRequest = 1;
    }

    if (elevator->hallDownRequests[index] &&
        (elevator->direction == DIRECTION_NONE ||
         elevator->direction == DIRECTION_DOWN ||
         (elevator->direction == DIRECTION_UP &&
          !Elevator_HasRequestAbove(elevator, elevator->currentFloor))))
    {
        waitTime = elevator->totalTimeSeconds - elevator->hallDownRequestCreatedAt[index];
        if (waitTime < 0)
        {
            waitTime = 0;
        }

        elevator->completedRequestCount++;
        elevator->totalWaitTimeSeconds += waitTime;
        if (waitTime > elevator->longestWaitTimeSeconds)
        {
            elevator->longestWaitTimeSeconds = waitTime;
        }
        printf("[Stats] Hall down request at floor %d waited %d seconds before service.\n",
               floor,
               waitTime);
        hasRecordedRequest = 1;
    }

    if (elevator->carFloorRequests[index])
    {
        waitTime = elevator->totalTimeSeconds - elevator->carRequestCreatedAt[index];
        if (waitTime < 0)
        {
            waitTime = 0;
        }

        elevator->completedRequestCount++;
        elevator->totalWaitTimeSeconds += waitTime;
        if (waitTime > elevator->longestWaitTimeSeconds)
        {
            elevator->longestWaitTimeSeconds = waitTime;
        }
        printf("[Stats] Car request for floor %d waited %d seconds before service.\n",
               floor,
               waitTime);
        hasRecordedRequest = 1;
    }

    if (!hasRecordedRequest)
    {
        printf("[Stats] No request statistics recorded for floor %d.\n", floor);
    }
}

static void Elevator_UpdateSafetyState(Elevator *elevator)
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

int Elevator_HasAnyRequest(const Elevator *elevator)
{
    int i;

    if (elevator == NULL)
    {
        return 0;
    }

    for (i = 0; i < TOTAL_FLOOR_COUNT; i++)
    {
        if (Elevator_HasRequestAtIndex(elevator, i))
        {
            return 1;
        }
    }

    return 0;
}

int Elevator_HasRequestAtFloor(const Elevator *elevator, int floor)
{
    int index;

    if (elevator == NULL)
    {
        return 0;
    }

    index = Elevator_FloorToIndex(floor);
    if (index < 0)
    {
        return 0;
    }

    return Elevator_HasRequestAtIndex(elevator, index);
}

int Elevator_AddRequest(Elevator *elevator, int floor)
{
    return Elevator_AddCarRequest(elevator, floor);
}

int Elevator_PressHallUpButton(Elevator *elevator, int floor)
{
    return Elevator_AddHallUpRequest(elevator, floor);
}

int Elevator_PressHallDownButton(Elevator *elevator, int floor)
{
    return Elevator_AddHallDownRequest(elevator, floor);
}

int Elevator_PressCarFloorButton(Elevator *elevator, int floor)
{
    return Elevator_AddCarRequest(elevator, floor);
}

int Elevator_IsValidHallUpFloor(int floor)
{
    return floor >= MIN_FLOOR && floor < MAX_FLOOR && floor != 0;
}

int Elevator_IsValidHallDownFloor(int floor)
{
    return floor > MIN_FLOOR && floor <= MAX_FLOOR && floor != 0;
}

int Elevator_AddHallUpRequest(Elevator *elevator, int floor)
{
    int index;

    if (elevator == NULL)
    {
        return 0;
    }

    if (!elevator->isMainPowerOn || elevator->isPowerOff)
    {
        printf("[Request] Hall up request ignored because main power is off.\n");
        return 0;
    }

    if (!Elevator_IsValidHallUpFloor(floor))
    {
        printf("[Request] Floor %d cannot make a hall up request.\n", floor);
        return 0;
    }

    index = Elevator_FloorToIndex(floor);
    if (index < 0)
    {
        printf("[Request] Floor %d is invalid.\n", floor);
        return 0;
    }

    if (elevator->hallUpRequests[index])
    {
        printf("[Request] Hall up request at floor %d already exists.\n", floor);
        return 0;
    }

    elevator->hallUpRequests[index] = 1;
    elevator->hallUpRequestCreatedAt[index] = elevator->totalTimeSeconds;
    Elevator_ResetIdleTimer(elevator);
    printf("[Request] Added hall up request at floor %d.\n", floor);
    return 1;
}

int Elevator_AddHallDownRequest(Elevator *elevator, int floor)
{
    int index;

    if (elevator == NULL)
    {
        return 0;
    }

    if (!elevator->isMainPowerOn || elevator->isPowerOff)
    {
        printf("[Request] Hall down request ignored because main power is off.\n");
        return 0;
    }

    if (!Elevator_IsValidHallDownFloor(floor))
    {
        printf("[Request] Floor %d cannot make a hall down request.\n", floor);
        return 0;
    }

    index = Elevator_FloorToIndex(floor);
    if (index < 0)
    {
        return 0;
    }

    if (elevator->hallDownRequests[index])
    {
        printf("[Request] Hall down request at floor %d already exists.\n", floor);
        return 0;
    }

    elevator->hallDownRequests[index] = 1;
    elevator->hallDownRequestCreatedAt[index] = elevator->totalTimeSeconds;
    Elevator_ResetIdleTimer(elevator);
    printf("[Request] Added hall down request at floor %d.\n", floor);
    return 1;
}

int Elevator_AddCarRequest(Elevator *elevator, int floor)
{
    int index;

    if (elevator == NULL)
    {
        return 0;
    }

    if (!elevator->isMainPowerOn || elevator->isPowerOff)
    {
        printf("[Request] Car floor request ignored because main power is off.\n");
        return 0;
    }

    index = Elevator_FloorToIndex(floor);
    if (index < 0)
    {
        printf("[Request] Floor %d is invalid.\n", floor);
        return 0;
    }

    if (elevator->carFloorRequests[index])
    {
        printf("[Request] Car request for floor %d already exists.\n", floor);
        return 0;
    }

    elevator->carFloorRequests[index] = 1;
    elevator->carRequestCreatedAt[index] = elevator->totalTimeSeconds;
    Elevator_ResetIdleTimer(elevator);
    printf("[Request] Added car request for floor %d.\n", floor);
    return 1;
}

int Elevator_ClearRequest(Elevator *elevator, int floor)
{
    int index;

    if (elevator == NULL)
    {
        return 0;
    }

    index = Elevator_FloorToIndex(floor);
    if (index < 0 || !Elevator_HasRequestAtIndex(elevator, index))
    {
        return 0;
    }

    elevator->hallUpRequests[index] = 0;
    elevator->hallDownRequests[index] = 0;
    elevator->carFloorRequests[index] = 0;
    elevator->hallUpRequestCreatedAt[index] = 0;
    elevator->hallDownRequestCreatedAt[index] = 0;
    elevator->carRequestCreatedAt[index] = 0;
    return 1;
}

int Elevator_FindNextTarget(const Elevator *elevator)
{
    int i;
    int floor;
    int bestFloor = NO_TARGET_FLOOR;
    int bestDistance = MAX_FLOOR - MIN_FLOOR + 1;

    if (elevator == NULL || !Elevator_HasAnyRequest(elevator))
    {
        return NO_TARGET_FLOOR;
    }

    if (elevator->direction == DIRECTION_UP)
    {
        for (i = Elevator_FloorToIndex(elevator->currentFloor) + 1; i < TOTAL_FLOOR_COUNT; i++)
        {
            floor = Elevator_IndexToFloor(i);
            if (floor > elevator->currentFloor &&
                (elevator->carFloorRequests[i] || elevator->hallUpRequests[i]))
            {
                return floor;
            }
        }
    }
    else if (elevator->direction == DIRECTION_DOWN)
    {
        for (i = Elevator_FloorToIndex(elevator->currentFloor) - 1; i >= 0; i--)
        {
            floor = Elevator_IndexToFloor(i);
            if (floor < elevator->currentFloor &&
                (elevator->carFloorRequests[i] || elevator->hallDownRequests[i]))
            {
                return floor;
            }
        }
    }

    for (i = 0; i < TOTAL_FLOOR_COUNT; i++)
    {
        if (Elevator_HasRequestAtIndex(elevator, i))
        {
            int distance;

            floor = Elevator_IndexToFloor(i);
            if (floor == elevator->currentFloor &&
                !Elevator_ShouldServeCurrentFloor(elevator))
            {
                continue;
            }

            distance = Elevator_Abs(floor - elevator->currentFloor);

            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestFloor = floor;
            }
        }
    }

    return bestFloor;
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

void Elevator_OpenDoor(Elevator *elevator)
{
    int index;

    if (elevator == NULL)
    {
        return;
    }

    index = Elevator_GetCurrentFloorIndex(elevator);
    if (index < 0 || !elevator->isAlignedWithFloor)
    {
        printf("[Safety] Cannot open doors before floor alignment is confirmed.\n");
        return;
    }

    elevator->state = ELEVATOR_DOOR_OPENING;
    elevator->landingDoorLocked[index] = 0;
    elevator->carDoor = DOOR_OPEN;
    elevator->landingDoors[index] = DOOR_OPEN;
    elevator->door = DOOR_OPEN;
    elevator->totalTimeSeconds += DOOR_OPEN_TIME;
    printf("[Door] Unlocking landing door and opening car/landing doors at floor %d. Time +%ds, total %ds.\n",
           elevator->currentFloor,
           DOOR_OPEN_TIME,
           elevator->totalTimeSeconds);
}

void Elevator_HoldDoor(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    elevator->state = ELEVATOR_DOOR_HOLDING;
    elevator->carDoor = DOOR_OPEN;
    elevator->door = DOOR_OPEN;
    elevator->totalTimeSeconds += DOOR_HOLD_TIME;
    printf("[Door] Holding open. Time +%ds, total %ds.\n",
           DOOR_HOLD_TIME,
           elevator->totalTimeSeconds);
}

void Elevator_CloseDoor(Elevator *elevator)
{
    int index;

    if (elevator == NULL)
    {
        return;
    }

    index = Elevator_GetCurrentFloorIndex(elevator);
    if (index < 0)
    {
        return;
    }

    if (!Elevator_CanCloseDoor(elevator))
    {
        printf("[Safety] Door closing blocked. Door will reopen and hold.\n");
        Elevator_OpenDoor(elevator);
        Elevator_HoldDoor(elevator);
        return;
    }

    elevator->state = ELEVATOR_DOOR_CLOSING;
    elevator->landingDoors[index] = DOOR_CLOSED;
    elevator->carDoor = DOOR_CLOSED;
    elevator->landingDoorLocked[index] = 1;
    elevator->door = DOOR_CLOSED;
    elevator->totalTimeSeconds += DOOR_CLOSE_TIME;
    printf("[Door] Closing car/landing doors and locking landing door. Time +%ds, total %ds.\n",
           DOOR_CLOSE_TIME,
           elevator->totalTimeSeconds);
}

void Elevator_PrintStatus(const Elevator *elevator)
{
    int currentIndex;

    if (elevator == NULL)
    {
        return;
    }

    currentIndex = Elevator_GetCurrentFloorIndex(elevator);

    printf("\n--- Elevator Status ---\n");
    printf("Current floor : %d\n", elevator->currentFloor);
    printf("Target floor  : %s", elevator->targetFloor == NO_TARGET_FLOOR ? "None\n" : "");
    if (elevator->targetFloor != NO_TARGET_FLOOR)
    {
        printf("%d\n", elevator->targetFloor);
    }
    printf("State         : %s\n", Elevator_GetStateName(elevator->state));
    printf("Direction     : %s\n", Elevator_GetDirectionName(elevator->direction));
    printf("Door          : %s\n", Elevator_GetDoorName(elevator->door));
    printf("Car door      : %s\n", Elevator_GetDoorName(elevator->carDoor));
    if (currentIndex >= 0)
    {
        printf("Landing door  : %s\n", Elevator_GetDoorName(elevator->landingDoors[currentIndex]));
        printf("Landing locked: %s\n", elevator->landingDoorLocked[currentIndex] ? "Yes" : "No");
    }
    printf("Aligned floor : %s\n", elevator->isAlignedWithFloor ? "Yes" : "No");
    printf("All doors safe: %s\n", Elevator_AreAllLandingDoorsLocked(elevator) ? "Yes" : "No");
    printf("Load          : %d/%d kg\n", elevator->currentLoadKg, MAX_LOAD_KG);
    printf("Door blocked  : %s\n", elevator->isDoorBlocked ? "Yes" : "No");
    printf("Open held     : %s\n", elevator->isDoorOpenButtonHeld ? "Yes" : "No");
    printf("Emergency call: %s\n", elevator->isEmergencyCallActive ? "Yes" : "No");
    printf("Admin paused  : %s\n", elevator->isAdminPaused ? "Yes" : "No");
    printf("Power off     : %s\n", elevator->isPowerOff ? "Yes" : "No");
    printf("Main power    : %s\n", elevator->isMainPowerOn ? "On" : "Off");
    printf("Power failure : %s\n", elevator->isPowerFailure ? "Yes" : "No");
    printf("Backup power  : %s\n", elevator->isBackupPowerAvailable ? "Available" : "Unavailable");
    printf("Backup active : %s\n", elevator->isBackupPowerActive ? "Yes" : "No");
    printf("Recovering    : %s\n", elevator->isRecovering ? "Yes" : "No");
    printf("Between floors: %s\n", elevator->isBetweenFloors ? "Yes" : "No");
    printf("Safe floor    : %d\n", elevator->safeFloor);
    printf("Rescue floor  : %d\n", elevator->rescueFloor);
    printf("Power fail dir: %s\n",
           Elevator_GetDirectionName(elevator->directionBeforePowerFailure));
    printf("Fault         : %s\n", Elevator_GetFaultName(elevator->fault));
    printf("Total time    : %d seconds\n", elevator->totalTimeSeconds);
    printf("Idle time     : %d/%d seconds\n",
           elevator->idleTimeSeconds,
           IDLE_RETURN_DELAY_SECONDS);
    Elevator_PrintRequests(elevator);
    Elevator_PrintStats(elevator);
    printf("-----------------------\n\n");
}

void Elevator_PrintRequests(const Elevator *elevator)
{
    int i;
    int hasRequest = 0;

    if (elevator == NULL)
    {
        return;
    }

    printf("Hall up req   : ");
    for (i = 0; i < TOTAL_FLOOR_COUNT; i++)
    {
        if (elevator->hallUpRequests[i])
        {
            printf("%d ", Elevator_IndexToFloor(i));
            hasRequest = 1;
        }
    }

    if (!hasRequest)
    {
        printf("None");
    }

    printf("\n");

    hasRequest = 0;
    printf("Hall down req : ");
    for (i = 0; i < TOTAL_FLOOR_COUNT; i++)
    {
        if (elevator->hallDownRequests[i])
        {
            printf("%d ", Elevator_IndexToFloor(i));
            hasRequest = 1;
        }
    }

    if (!hasRequest)
    {
        printf("None");
    }

    printf("\n");

    hasRequest = 0;
    printf("Car floor req : ");
    for (i = 0; i < TOTAL_FLOOR_COUNT; i++)
    {
        if (elevator->carFloorRequests[i])
        {
            printf("%d ", Elevator_IndexToFloor(i));
            hasRequest = 1;
        }
    }

    if (!hasRequest)
    {
        printf("None");
    }

    printf("\n");
}

void Elevator_PrintStats(const Elevator *elevator)
{
    double averageWait;

    if (elevator == NULL)
    {
        return;
    }

    printf("Completed req : %d\n", elevator->completedRequestCount);
    printf("Total wait    : %d seconds\n", elevator->totalWaitTimeSeconds);
    printf("Longest wait  : %d seconds\n", elevator->longestWaitTimeSeconds);

    if (elevator->completedRequestCount == 0)
    {
        printf("Average wait  : 0.00 seconds\n");
        return;
    }

    averageWait = (double)elevator->totalWaitTimeSeconds / elevator->completedRequestCount;
    printf("Average wait  : %.2f seconds\n", averageWait);
}

const char *Elevator_GetStateName(ElevatorState state)
{
    switch (state)
    {
    case ELEVATOR_IDLE:
        return "Idle";
    case ELEVATOR_MOVING:
        return "Moving";
    case ELEVATOR_DOOR_OPENING:
        return "Door opening";
    case ELEVATOR_DOOR_HOLDING:
        return "Door holding";
    case ELEVATOR_DOOR_CLOSING:
        return "Door closing";
    case ELEVATOR_FAULT:
        return "Fault";
    case ELEVATOR_PAUSED:
        return "Paused";
    case ELEVATOR_POWER_OFF:
        return "Power off";
    case ELEVATOR_RECOVERING:
        return "Recovering";
    default:
        return "Unknown";
    }
}

const char *Elevator_GetDirectionName(ElevatorDirection direction)
{
    switch (direction)
    {
    case DIRECTION_NONE:
        return "None";
    case DIRECTION_UP:
        return "Up";
    case DIRECTION_DOWN:
        return "Down";
    default:
        return "Unknown";
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

int Elevator_AreAllLandingDoorsLocked(const Elevator *elevator)
{
    int i;

    if (elevator == NULL)
    {
        return 0;
    }

    for (i = 0; i < TOTAL_FLOOR_COUNT; i++)
    {
        if (elevator->landingDoors[i] != DOOR_CLOSED ||
            !elevator->landingDoorLocked[i])
        {
            return 0;
        }
    }

    return 1;
}

int Elevator_CanCloseDoor(const Elevator *elevator)
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

    if (elevator->isPowerOff)
    {
        return 0;
    }

    if (elevator->fault != FAULT_NONE)
    {
        return 0;
    }

    if (!elevator->isAlignedWithFloor)
    {
        return 0;
    }

    if (elevator->isOverloaded)
    {
        return 0;
    }

    if (elevator->isDoorBlocked)
    {
        return 0;
    }

    if (elevator->isDoorOpenButtonHeld)
    {
        return 0;
    }

    return 1;
}

void Elevator_PressDoorOpenButton(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    elevator->isDoorOpenButtonHeld = 1;
    printf("[Cabin] Door open button held.\n");

    if (!elevator->isPowerOff && elevator->fault == FAULT_NONE && elevator->isAlignedWithFloor)
    {
        Elevator_ForceDoorsOpenAtCurrentFloor(elevator);
        elevator->state = ELEVATOR_DOOR_HOLDING;
    }
}

void Elevator_ReleaseDoorOpenButton(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    elevator->isDoorOpenButtonHeld = 0;
    printf("[Cabin] Door open button released.\n");
    Elevator_UpdateSafetyState(elevator);
}

void Elevator_PressDoorCloseButton(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    printf("[Cabin] Door close button pressed.\n");
    Elevator_CloseDoor(elevator);
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

void Elevator_SetDoorBlocked(Elevator *elevator, int isBlocked)
{
    if (elevator == NULL)
    {
        return;
    }

    elevator->isDoorBlocked = isBlocked ? 1 : 0;
    printf("[Safety] Door blocked: %s.\n", elevator->isDoorBlocked ? "yes" : "no");

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

const char *Elevator_GetFaultName(FaultType fault)
{
    switch (fault)
    {
    case FAULT_NONE:
        return "None";
    case FAULT_DOOR:
        return "Door fault";
    case FAULT_MOTOR:
        return "Motor fault";
    case FAULT_SENSOR:
        return "Sensor fault";
    case FAULT_UNKNOWN:
        return "Unknown fault";
    default:
        return "Invalid fault";
    }
}

const char *Elevator_GetDoorName(DoorState door)
{
    switch (door)
    {
    case DOOR_CLOSED:
        return "Closed";
    case DOOR_OPEN:
        return "Open";
    default:
        return "Unknown";
    }
}
