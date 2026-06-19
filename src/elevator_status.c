#include "elevator_internal.h"

#include <stdio.h>

void Elevator_PrintStatus(const Elevator *elevator)
{
    int currentIndex;

    if (elevator == NULL)
    {
        return;
    }

    currentIndex = Elevator_FloorToIndex(elevator->currentFloor);

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
