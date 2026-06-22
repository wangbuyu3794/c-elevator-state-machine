#include "elevator.h"

#include <stdio.h>
#include <stdlib.h>

static void Elevator_ClearConsole(void)
{
#ifdef _WIN32
    system("cls");
#else
    printf("\033[2J\033[H");
#endif
}

void Elevator_RunUntilIdleWithCompactPanel(Elevator *elevator)
{
    ElevatorSnapshot snapshot;

    if (elevator == NULL)
    {
        return;
    }

    while (Elevator_HasAnyRequest(elevator))
    {
        Elevator_GetSnapshot(elevator, &snapshot);
        if (!snapshot.canMove)
        {
            printf("[Safety] Visual run stopped before all requests finished.\n");
            Elevator_PrintCompactVisualPanel(elevator);
            return;
        }

        Elevator_RunOneStep(elevator);

        if (Elevator_HasAnyRequest(elevator))
        {
            Elevator_PrintCompactVisualPanel(elevator);
        }
    }

    Elevator_RunUntilIdle(elevator);
    Elevator_PrintCompactVisualPanel(elevator);
}

void Elevator_RunUntilIdleWithRefreshedCompactPanel(Elevator *elevator)
{
    ElevatorSnapshot snapshot;

    if (elevator == NULL)
    {
        return;
    }

    while (Elevator_HasAnyRequest(elevator))
    {
        Elevator_GetSnapshot(elevator, &snapshot);
        if (!snapshot.canMove)
        {
            Elevator_ClearConsole();
            printf("[Safety] Refreshed visual run stopped before all requests finished.\n");
            Elevator_PrintCompactVisualPanel(elevator);
            return;
        }

        Elevator_RunOneStep(elevator);

        if (Elevator_HasAnyRequest(elevator))
        {
            Elevator_ClearConsole();
            Elevator_PrintCompactVisualPanel(elevator);
        }
    }

    Elevator_RunUntilIdle(elevator);
    Elevator_ClearConsole();
    Elevator_PrintCompactVisualPanel(elevator);
}
