#ifndef ELEVATOR_INTERNAL_H
#define ELEVATOR_INTERNAL_H

#include "elevator.h"

/*
 * Internal module cooperation API.
 *
 * These functions are shared by the C source files in src/, but they are not
 * intended to be called by main.c or a future GUI directly.
 */

int Elevator_FindNextTarget(const Elevator *elevator);
int Elevator_ShouldServeCurrentFloor(const Elevator *elevator);
int Elevator_ClearServedRequestsAtCurrentFloor(Elevator *elevator);
void Elevator_RecordCompletedRequest(Elevator *elevator, int floor);
void Elevator_ResetIdleTimer(Elevator *elevator);
void Elevator_ClearAllCarRequests(Elevator *elevator);

void Elevator_ForceDoorsOpenAtCurrentFloor(Elevator *elevator);
int Elevator_CanCloseDoor(const Elevator *elevator);
int Elevator_AreAllLandingDoorsLocked(const Elevator *elevator);

int Elevator_CanMove(const Elevator *elevator);
void Elevator_UpdateSafetyState(Elevator *elevator);

#endif
