#include "elevator.h"

#include <stdio.h>

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
