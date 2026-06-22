#include "elevator_internal.h"

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

void Elevator_ResetIdleTimer(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    elevator->idleTimeSeconds = 0;
}

void Elevator_ClearAllCarRequests(Elevator *elevator)
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

int Elevator_ShouldServeCurrentFloor(const Elevator *elevator)
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

int Elevator_ClearServedRequestsAtCurrentFloor(Elevator *elevator)
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

void Elevator_RecordCompletedRequest(Elevator *elevator, int floor)
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

ElevatorEventResult Elevator_AddRequest(Elevator *elevator, int floor)
{
    return Elevator_AddCarRequest(elevator, floor);
}

ElevatorEventResult Elevator_PressHallUpButton(Elevator *elevator, int floor)
{
    return Elevator_AddHallUpRequest(elevator, floor);
}

ElevatorEventResult Elevator_PressHallDownButton(Elevator *elevator, int floor)
{
    return Elevator_AddHallDownRequest(elevator, floor);
}

ElevatorEventResult Elevator_PressCarFloorButton(Elevator *elevator, int floor)
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

ElevatorEventResult Elevator_AddHallUpRequest(Elevator *elevator, int floor)
{
    int index;

    if (elevator == NULL)
    {
        return ELEVATOR_EVENT_NULL_ELEVATOR;
    }

    if (!elevator->isMainPowerOn || elevator->isPowerOff)
    {
        printf("[Request] Hall up request ignored because main power is off.\n");
        return ELEVATOR_EVENT_POWER_OFF;
    }

    if (!Elevator_IsValidHallUpFloor(floor))
    {
        printf("[Request] Floor %d cannot make a hall up request.\n", floor);
        return ELEVATOR_EVENT_INVALID_FLOOR;
    }

    index = Elevator_FloorToIndex(floor);
    if (index < 0)
    {
        printf("[Request] Floor %d is invalid.\n", floor);
        return ELEVATOR_EVENT_INVALID_FLOOR;
    }

    if (elevator->hallUpRequests[index])
    {
        printf("[Request] Hall up request at floor %d already exists.\n", floor);
        return ELEVATOR_EVENT_REQUEST_EXISTS;
    }

    elevator->hallUpRequests[index] = 1;
    elevator->hallUpRequestCreatedAt[index] = elevator->totalTimeSeconds;
    Elevator_ResetIdleTimer(elevator);
    printf("[Request] Added hall up request at floor %d.\n", floor);
    return ELEVATOR_EVENT_OK;
}

ElevatorEventResult Elevator_AddHallDownRequest(Elevator *elevator, int floor)
{
    int index;

    if (elevator == NULL)
    {
        return ELEVATOR_EVENT_NULL_ELEVATOR;
    }

    if (!elevator->isMainPowerOn || elevator->isPowerOff)
    {
        printf("[Request] Hall down request ignored because main power is off.\n");
        return ELEVATOR_EVENT_POWER_OFF;
    }

    if (!Elevator_IsValidHallDownFloor(floor))
    {
        printf("[Request] Floor %d cannot make a hall down request.\n", floor);
        return ELEVATOR_EVENT_INVALID_FLOOR;
    }

    index = Elevator_FloorToIndex(floor);
    if (index < 0)
    {
        return ELEVATOR_EVENT_INVALID_FLOOR;
    }

    if (elevator->hallDownRequests[index])
    {
        printf("[Request] Hall down request at floor %d already exists.\n", floor);
        return ELEVATOR_EVENT_REQUEST_EXISTS;
    }

    elevator->hallDownRequests[index] = 1;
    elevator->hallDownRequestCreatedAt[index] = elevator->totalTimeSeconds;
    Elevator_ResetIdleTimer(elevator);
    printf("[Request] Added hall down request at floor %d.\n", floor);
    return ELEVATOR_EVENT_OK;
}

ElevatorEventResult Elevator_AddCarRequest(Elevator *elevator, int floor)
{
    int index;

    if (elevator == NULL)
    {
        return ELEVATOR_EVENT_NULL_ELEVATOR;
    }

    if (!elevator->isMainPowerOn || elevator->isPowerOff)
    {
        printf("[Request] Car floor request ignored because main power is off.\n");
        return ELEVATOR_EVENT_POWER_OFF;
    }

    index = Elevator_FloorToIndex(floor);
    if (index < 0)
    {
        printf("[Request] Floor %d is invalid.\n", floor);
        return ELEVATOR_EVENT_INVALID_FLOOR;
    }

    if (elevator->carFloorRequests[index])
    {
        printf("[Request] Car request for floor %d already exists.\n", floor);
        return ELEVATOR_EVENT_REQUEST_EXISTS;
    }

    elevator->carFloorRequests[index] = 1;
    elevator->carRequestCreatedAt[index] = elevator->totalTimeSeconds;
    Elevator_ResetIdleTimer(elevator);
    printf("[Request] Added car request for floor %d.\n", floor);
    return ELEVATOR_EVENT_OK;
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
