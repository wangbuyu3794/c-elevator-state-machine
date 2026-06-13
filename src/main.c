#include "elevator.h"

#include <stdio.h>

static void PrintMenu(void)
{
    printf("C Elevator State Machine\n");
    printf("1. Add floor request\n");
    printf("2. Run one step\n");
    printf("3. Run until idle\n");
    printf("4. Print status\n");
    printf("0. Exit\n");
    printf("Choose: ");
}

int main(void)
{
    Elevator elevator;
    int choice;
    int floor;

    Elevator_Init(&elevator);
    printf("Elevator initialized at floor %d.\n\n", elevator.currentFloor);

    while (1)
    {
        PrintMenu();

        if (scanf("%d", &choice) != 1)
        {
            printf("Invalid input. Program stopped.\n");
            break;
        }

        if (choice == 0)
        {
            printf("Goodbye.\n");
            break;
        }

        switch (choice)
        {
        case 1:
            printf("Enter floor (-1 or 1-34): ");
            if (scanf("%d", &floor) != 1)
            {
                printf("Invalid floor input. Program stopped.\n");
                return 1;
            }
            Elevator_AddRequest(&elevator, floor);
            break;
        case 2:
            Elevator_RunOneStep(&elevator);
            break;
        case 3:
            Elevator_RunUntilIdle(&elevator);
            break;
        case 4:
            Elevator_PrintStatus(&elevator);
            break;
        default:
            printf("Unknown menu option.\n");
            break;
        }

        printf("\n");
    }

    return 0;
}
