#include "elevator.h"

#include <stdio.h>
static int Elevator_Abs(int value)
{
    return value < 0 ? -value : value;
}

static void Elevator_FinishStop(Elevator *elevator)
{
    Elevator_ClearRequest(elevator, elevator->currentFloor);
    Elevator_OpenDoor(elevator);
    Elevator_HoldDoor(elevator);
    Elevator_CloseDoor(elevator);

    elevator->targetFloor = NO_TARGET_FLOOR;

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

    for (i = 0; i < TOTAL_FLOOR_COUNT; i++)
    {
        elevator->floorRequests[i] = 0;
    }
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
        if (elevator->floorRequests[i])
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

    return elevator->floorRequests[index] != 0;
}

int Elevator_AddRequest(Elevator *elevator, int floor)
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

    if (elevator->floorRequests[index])
    {
        printf("[Request] Floor %d is already in the request table.\n", floor);
        return 0;
    }

    elevator->floorRequests[index] = 1;
    printf("[Request] Added floor %d.\n", floor);
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
    if (index < 0 || !elevator->floorRequests[index])
    {
        return 0;
    }

    elevator->floorRequests[index] = 0;
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
            if (floor > elevator->currentFloor && elevator->floorRequests[i])
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
            if (floor < elevator->currentFloor && elevator->floorRequests[i])
            {
                return floor;
            }
        }
    }

    for (i = 0; i < TOTAL_FLOOR_COUNT; i++)
    {
        if (elevator->floorRequests[i])
        {
            int distance;

            floor = Elevator_IndexToFloor(i);
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

    if (!Elevator_HasAnyRequest(elevator))
    {
        elevator->targetFloor = NO_TARGET_FLOOR;
        elevator->direction = DIRECTION_NONE;
        elevator->state = ELEVATOR_IDLE;
        printf("[Idle] No pending requests.\n");
        return;
    }

    if (Elevator_HasRequestAtFloor(elevator, elevator->currentFloor))
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

    if (elevator->currentFloor == elevator->targetFloor)
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
        Elevator_RunOneStep(elevator);
    }

    elevator->state = ELEVATOR_IDLE;
    elevator->direction = DIRECTION_NONE;
    elevator->targetFloor = NO_TARGET_FLOOR;
    printf("[Done] All requests finished. Total time: %ds.\n", elevator->totalTimeSeconds);
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
    printf("Total time    : %d seconds\n", elevator->totalTimeSeconds);
    Elevator_PrintRequests(elevator);
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

    printf("Pending floors: ");
    for (i = 0; i < TOTAL_FLOOR_COUNT; i++)
    {
        if (elevator->floorRequests[i])
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
