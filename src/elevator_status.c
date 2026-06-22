#include "elevator_internal.h"

#include <stdio.h>

void Elevator_PrintStatus(const Elevator *elevator)
{
    ElevatorSnapshot snapshot;

    if (elevator == NULL)
    {
        return;
    }

    Elevator_GetSnapshot(elevator, &snapshot);

    printf("\n--- Elevator Status ---\n");
    printf("Current floor : %d\n", snapshot.currentFloor);
    printf("Target floor  : %s", snapshot.targetFloor == NO_TARGET_FLOOR ? "None\n" : "");
    if (snapshot.targetFloor != NO_TARGET_FLOOR)
    {
        printf("%d\n", snapshot.targetFloor);
    }
    printf("State         : %s\n", Elevator_GetStateName(snapshot.state));
    printf("Direction     : %s\n", Elevator_GetDirectionName(snapshot.direction));
    printf("Door          : %s\n", Elevator_GetDoorName(snapshot.door));
    printf("Car door      : %s\n", Elevator_GetDoorName(snapshot.carDoor));
    if (snapshot.currentFloorIndex >= 0)
    {
        printf("Landing door  : %s\n", Elevator_GetDoorName(snapshot.currentLandingDoor));
        printf("Landing locked: %s\n", snapshot.currentLandingDoorLocked ? "Yes" : "No");
    }
    printf("Aligned floor : %s\n", snapshot.isAlignedWithFloor ? "Yes" : "No");
    printf("All doors safe: %s\n", snapshot.areAllLandingDoorsLocked ? "Yes" : "No");
    printf("Load          : %d/%d kg\n", snapshot.currentLoadKg, MAX_LOAD_KG);
    printf("Door blocked  : %s\n", snapshot.isDoorBlocked ? "Yes" : "No");
    printf("Open held     : %s\n", snapshot.isDoorOpenButtonHeld ? "Yes" : "No");
    printf("Emergency call: %s\n", snapshot.isEmergencyCallActive ? "Yes" : "No");
    printf("Admin paused  : %s\n", snapshot.isAdminPaused ? "Yes" : "No");
    printf("Power off     : %s\n", snapshot.isPowerOff ? "Yes" : "No");
    printf("Main power    : %s\n", snapshot.isMainPowerOn ? "On" : "Off");
    printf("Power failure : %s\n", snapshot.isPowerFailure ? "Yes" : "No");
    printf("Backup power  : %s\n", snapshot.isBackupPowerAvailable ? "Available" : "Unavailable");
    printf("Backup active : %s\n", snapshot.isBackupPowerActive ? "Yes" : "No");
    printf("Recovering    : %s\n", snapshot.isRecovering ? "Yes" : "No");
    printf("Between floors: %s\n", snapshot.isBetweenFloors ? "Yes" : "No");
    printf("Safe floor    : %d\n", snapshot.safeFloor);
    printf("Rescue floor  : %d\n", snapshot.rescueFloor);
    printf("Power fail dir: %s\n",
           Elevator_GetDirectionName(snapshot.directionBeforePowerFailure));
    printf("Fault         : %s\n", Elevator_GetFaultName(snapshot.fault));
    printf("Can move      : %s\n", snapshot.canMove ? "Yes" : "No");
    printf("Can close door: %s\n", snapshot.canCloseDoor ? "Yes" : "No");
    printf("Has request   : %s\n", snapshot.hasAnyRequest ? "Yes" : "No");
    printf("Total time    : %d seconds\n", snapshot.totalTimeSeconds);
    printf("Idle time     : %d/%d seconds\n",
           snapshot.idleTimeSeconds,
           IDLE_RETURN_DELAY_SECONDS);
    Elevator_PrintRequests(elevator);
    Elevator_PrintStats(elevator);
    printf("-----------------------\n\n");
}

void Elevator_PrintRequests(const Elevator *elevator)
{
    int i;
    int hasRequest = 0;
    ElevatorSnapshot snapshot;

    if (elevator == NULL)
    {
        return;
    }

    Elevator_GetSnapshot(elevator, &snapshot);

    printf("Hall up req   : ");
    for (i = 0; i < TOTAL_FLOOR_COUNT; i++)
    {
        if (snapshot.hallUpRequests[i])
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
        if (snapshot.hallDownRequests[i])
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
        if (snapshot.carFloorRequests[i])
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
    ElevatorSnapshot snapshot;

    if (elevator == NULL)
    {
        return;
    }

    Elevator_GetSnapshot(elevator, &snapshot);

    printf("Completed req : %d\n", snapshot.completedRequestCount);
    printf("Total wait    : %d seconds\n", snapshot.totalWaitTimeSeconds);
    printf("Longest wait  : %d seconds\n", snapshot.longestWaitTimeSeconds);
    printf("Average wait  : %.2f seconds\n", snapshot.averageWaitTimeSeconds);
}
