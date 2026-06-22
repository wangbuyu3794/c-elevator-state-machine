#include "elevator.h"

#include <stdio.h>

static void PrintMenu(void)
{
    printf("C Elevator State Machine\n");
    printf("1. Press car floor button\n");
    printf("2. Run one step\n");
    printf("3. Run until idle\n");
    printf("4. Print status\n");
    printf("5. Set load\n");
    printf("6. Set door blocked\n");
    printf("7. Set fault\n");
    printf("8. Clear fault\n");
    printf("9. Admin pause\n");
    printf("10. Admin resume\n");
    printf("11. Power off\n");
    printf("12. Restore power\n");
    printf("13. Simulate between floors\n");
    printf("14. Run recovery\n");
    printf("15. Print statistics\n");
    printf("16. Press hall up button\n");
    printf("17. Press hall down button\n");
    printf("18. Simulate power failure\n");
    printf("19. Set backup power available\n");
    printf("20. Run backup rescue\n");
    printf("21. Hold door open button\n");
    printf("22. Release door open button\n");
    printf("23. Press door close button\n");
    printf("24. Press emergency call button\n");
    printf("25. Clear emergency call\n");
    printf("0. Exit\n");
    printf("Choose: ");
}

int main(void)
{
    Elevator elevator;
    int choice;
    int floor;
    int loadKg;
    int isBlocked;
    int isAvailable;
    int faultChoice;
    int safeFloor;
    ElevatorEventResult eventResult;

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
            printf("Enter car target floor (-1 or 1-34): ");
            if (scanf("%d", &floor) != 1)
            {
                printf("Invalid floor input. Program stopped.\n");
                return 1;
            }
            eventResult = Elevator_PressCarFloorButton(&elevator, floor);
            printf("Event result: %s\n", Elevator_GetEventResultName(eventResult));
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
        case 5:
            printf("Enter current load in kg: ");
            if (scanf("%d", &loadKg) != 1)
            {
                printf("Invalid load input. Program stopped.\n");
                return 1;
            }
            Elevator_SetLoad(&elevator, loadKg);
            break;
        case 6:
            printf("Door blocked? 1 = yes, 0 = no: ");
            if (scanf("%d", &isBlocked) != 1)
            {
                printf("Invalid door blocked input. Program stopped.\n");
                return 1;
            }
            Elevator_SetDoorBlocked(&elevator, isBlocked);
            break;
        case 7:
            printf("Fault type: 1 = door, 2 = motor, 3 = sensor, 4 = unknown: ");
            if (scanf("%d", &faultChoice) != 1)
            {
                printf("Invalid fault input. Program stopped.\n");
                return 1;
            }
            switch (faultChoice)
            {
            case 1:
                Elevator_SetFault(&elevator, FAULT_DOOR);
                break;
            case 2:
                Elevator_SetFault(&elevator, FAULT_MOTOR);
                break;
            case 3:
                Elevator_SetFault(&elevator, FAULT_SENSOR);
                break;
            case 4:
                Elevator_SetFault(&elevator, FAULT_UNKNOWN);
                break;
            default:
                printf("Unknown fault type.\n");
                break;
            }
            break;
        case 8:
            Elevator_ClearFault(&elevator);
            break;
        case 9:
            Elevator_AdminPause(&elevator);
            break;
        case 10:
            Elevator_AdminResume(&elevator);
            break;
        case 11:
            Elevator_PowerOff(&elevator);
            break;
        case 12:
            Elevator_RestorePower(&elevator);
            break;
        case 13:
            printf("Enter nearest safe floor (-1 or 1-34): ");
            if (scanf("%d", &safeFloor) != 1)
            {
                printf("Invalid safe floor input. Program stopped.\n");
                return 1;
            }
            Elevator_SetBetweenFloors(&elevator, safeFloor);
            break;
        case 14:
            Elevator_RunRecovery(&elevator);
            break;
        case 15:
            Elevator_PrintStats(&elevator);
            break;
        case 16:
            printf("Enter hall up floor (-1 or 1-33): ");
            if (scanf("%d", &floor) != 1)
            {
                printf("Invalid floor input. Program stopped.\n");
                return 1;
            }
            eventResult = Elevator_PressHallUpButton(&elevator, floor);
            printf("Event result: %s\n", Elevator_GetEventResultName(eventResult));
            break;
        case 17:
            printf("Enter hall down floor (1-34): ");
            if (scanf("%d", &floor) != 1)
            {
                printf("Invalid floor input. Program stopped.\n");
                return 1;
            }
            eventResult = Elevator_PressHallDownButton(&elevator, floor);
            printf("Event result: %s\n", Elevator_GetEventResultName(eventResult));
            break;
        case 18:
            Elevator_SimulatePowerFailure(&elevator);
            break;
        case 19:
            printf("Backup power available? 1 = yes, 0 = no: ");
            if (scanf("%d", &isAvailable) != 1)
            {
                printf("Invalid backup power input. Program stopped.\n");
                return 1;
            }
            Elevator_SetBackupPowerAvailable(&elevator, isAvailable);
            break;
        case 20:
            Elevator_RunBackupRescue(&elevator);
            break;
        case 21:
            Elevator_PressDoorOpenButton(&elevator);
            break;
        case 22:
            Elevator_ReleaseDoorOpenButton(&elevator);
            break;
        case 23:
            Elevator_PressDoorCloseButton(&elevator);
            break;
        case 24:
            Elevator_PressEmergencyCallButton(&elevator);
            break;
        case 25:
            Elevator_ClearEmergencyCall(&elevator);
            break;
        default:
            printf("Unknown menu option.\n");
            break;
        }

        printf("\n");
    }

    return 0;
}
