#include "elevator.h"

#include <stdio.h>

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

void Elevator_ForceDoorsOpenAtCurrentFloor(Elevator *elevator)
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

    if (elevator->door == DOOR_CLOSED &&
        elevator->carDoor == DOOR_CLOSED &&
        elevator->landingDoors[index] == DOOR_CLOSED &&
        elevator->landingDoorLocked[index])
    {
        printf("[Door] Doors are already closed and locked.\n");
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
