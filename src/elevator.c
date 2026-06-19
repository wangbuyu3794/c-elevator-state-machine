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

    if (elevator->isOverloaded || elevator->isDoorBlocked)
    {
        elevator->door = DOOR_OPEN;
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
    elevator->state = ELEVATOR_IDLE;
    elevator->direction = DIRECTION_NONE;
    elevator->door = DOOR_CLOSED;
    elevator->fault = FAULT_NONE;
    elevator->currentLoadKg = 0;
    elevator->isOverloaded = 0;
    elevator->isDoorBlocked = 0;
    elevator->isAdminPaused = 0;
    elevator->isPowerOff = 0;
    elevator->isRecovering = 0;
    elevator->isBetweenFloors = 0;
    elevator->safeFloor = 1;

    for (i = 0; i < TOTAL_FLOOR_COUNT; i++)
    {
        elevator->hallUpRequests[i] = 0;
        elevator->hallDownRequests[i] = 0;
        elevator->carFloorRequests[i] = 0;
        elevator->hallUpRequestCreatedAt[i] = 0;
        elevator->hallDownRequestCreatedAt[i] = 0;
        elevator->carRequestCreatedAt[i] = 0;
    }

    elevator->completedRequestCount = 0;
    elevator->totalWaitTimeSeconds = 0;
    elevator->longestWaitTimeSeconds = 0;
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
    elevator->door = DOOR_CLOSED;
    elevator->totalTimeSeconds += MOVE_TIME_PER_FLOOR;

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
        elevator->targetFloor = NO_TARGET_FLOOR;
        elevator->direction = DIRECTION_NONE;
        elevator->state = ELEVATOR_IDLE;
        printf("[Idle] No pending requests.\n");
        return;
    }

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
    if (elevator == NULL)
    {
        return;
    }

    elevator->state = ELEVATOR_DOOR_OPENING;
    elevator->door = DOOR_OPEN;
    elevator->totalTimeSeconds += DOOR_OPEN_TIME;
    printf("[Door] Opening at floor %d. Time +%ds, total %ds.\n",
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
    elevator->door = DOOR_OPEN;
    elevator->totalTimeSeconds += DOOR_HOLD_TIME;
    printf("[Door] Holding open. Time +%ds, total %ds.\n",
           DOOR_HOLD_TIME,
           elevator->totalTimeSeconds);
}

void Elevator_CloseDoor(Elevator *elevator)
{
    if (elevator == NULL)
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
    elevator->door = DOOR_CLOSED;
    elevator->totalTimeSeconds += DOOR_CLOSE_TIME;
    printf("[Door] Closing. Time +%ds, total %ds.\n",
           DOOR_CLOSE_TIME,
           elevator->totalTimeSeconds);
}

void Elevator_PrintStatus(const Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

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
    printf("Load          : %d/%d kg\n", elevator->currentLoadKg, MAX_LOAD_KG);
    printf("Door blocked  : %s\n", elevator->isDoorBlocked ? "Yes" : "No");
    printf("Admin paused  : %s\n", elevator->isAdminPaused ? "Yes" : "No");
    printf("Power off     : %s\n", elevator->isPowerOff ? "Yes" : "No");
    printf("Recovering    : %s\n", elevator->isRecovering ? "Yes" : "No");
    printf("Between floors: %s\n", elevator->isBetweenFloors ? "Yes" : "No");
    printf("Safe floor    : %d\n", elevator->safeFloor);
    printf("Fault         : %s\n", Elevator_GetFaultName(elevator->fault));
    printf("Total time    : %d seconds\n", elevator->totalTimeSeconds);
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

    if (elevator->door != DOOR_CLOSED)
    {
        return 0;
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

    if (elevator->isPowerOff)
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

    if (elevator->isDoorBlocked)
    {
        return 0;
    }

    return 1;
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
        elevator->door = DOOR_OPEN;
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

    elevator->isPowerOff = 1;
    elevator->isRecovering = 0;
    elevator->direction = DIRECTION_NONE;
    elevator->targetFloor = NO_TARGET_FLOOR;
    Elevator_UpdateSafetyState(elevator);
    printf("[Power] Power off. Elevator stopped immediately.\n");
}

void Elevator_RestorePower(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    if (!elevator->isPowerOff)
    {
        printf("[Power] Power is already on.\n");
        return;
    }

    elevator->isPowerOff = 0;
    elevator->isRecovering = 1;
    elevator->direction = DIRECTION_NONE;
    elevator->targetFloor = NO_TARGET_FLOOR;
    printf("[Power] Power restored. Starting safety recovery.\n");
    Elevator_RunRecovery(elevator);
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
    elevator->safeFloor = safeFloor;
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
