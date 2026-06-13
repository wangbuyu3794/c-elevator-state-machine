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
    printf("Fault         : %s\n", Elevator_GetFaultName(elevator->fault));
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
    case ELEVATOR_FAULT:
        return "Fault";
    case ELEVATOR_PAUSED:
        return "Paused";
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
        elevator->state = ELEVATOR_DOOR_HOLDING;
        printf("[Safety] Overload: %d kg. Door stays open.\n", loadKg);
    }
    else
    {
        printf("[Safety] Load set to %d kg.\n", loadKg);
        if (elevator->door == DOOR_OPEN && Elevator_CanCloseDoor(elevator))
        {
            Elevator_CloseDoor(elevator);
            elevator->state = ELEVATOR_IDLE;
        }
    }
}

void Elevator_SetDoorBlocked(Elevator *elevator, int isBlocked)
{
    if (elevator == NULL)
    {
        return;
    }

    elevator->isDoorBlocked = isBlocked ? 1 : 0;
    printf("[Safety] Door blocked: %s.\n", elevator->isDoorBlocked ? "yes" : "no");

    if (!elevator->isDoorBlocked && elevator->door == DOOR_OPEN && Elevator_CanCloseDoor(elevator))
    {
        Elevator_CloseDoor(elevator);
        elevator->state = ELEVATOR_IDLE;
    }
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
        if (!elevator->isAdminPaused)
        {
            elevator->state = ELEVATOR_IDLE;
        }
        return;
    }

    elevator->state = ELEVATOR_FAULT;
    elevator->direction = DIRECTION_NONE;
    elevator->targetFloor = NO_TARGET_FLOOR;
    printf("[Fault] Elevator entered fault state: %s.\n", Elevator_GetFaultName(fault));
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
    elevator->state = ELEVATOR_PAUSED;
    elevator->direction = DIRECTION_NONE;
    elevator->targetFloor = NO_TARGET_FLOOR;
    printf("[Admin] Elevator paused by administrator.\n");
}

void Elevator_AdminResume(Elevator *elevator)
{
    if (elevator == NULL)
    {
        return;
    }

    elevator->isAdminPaused = 0;
    if (elevator->fault == FAULT_NONE)
    {
        elevator->state = ELEVATOR_IDLE;
    }
    printf("[Admin] Elevator resumed by administrator.\n");
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
