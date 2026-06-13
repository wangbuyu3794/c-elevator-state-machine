#ifndef ELEVATOR_H
#define ELEVATOR_H

#define MIN_FLOOR -1
#define MAX_FLOOR 34
#define TOTAL_FLOOR_COUNT 35

/*
 * There is no floor 0 in this project, so 0 can safely mean
 * "the elevator currently has no target floor".
 */
#define NO_TARGET_FLOOR 0

#define MOVE_TIME_PER_FLOOR 3
#define DOOR_OPEN_TIME 2
#define DOOR_HOLD_TIME 5
#define DOOR_CLOSE_TIME 2

typedef enum
{
    ELEVATOR_IDLE,
    ELEVATOR_MOVING,
    ELEVATOR_DOOR_OPENING,
    ELEVATOR_DOOR_HOLDING,
    ELEVATOR_DOOR_CLOSING
} ElevatorState;

typedef enum
{
    DIRECTION_NONE,
    DIRECTION_UP,
    DIRECTION_DOWN
} ElevatorDirection;

typedef enum
{
    DOOR_CLOSED,
    DOOR_OPEN
} DoorState;

typedef struct
{
    int currentFloor;
    int targetFloor;
    int totalTimeSeconds;
    ElevatorState state;
    ElevatorDirection direction;
    DoorState door;

    /*
     * Each array element stores whether a floor has a pending request.
     * 1 means requested, 0 means not requested.
     */
    int floorRequests[TOTAL_FLOOR_COUNT];
} Elevator;

void Elevator_Init(Elevator *elevator);

int Elevator_IsValidFloor(int floor);
int Elevator_FloorToIndex(int floor);
int Elevator_IndexToFloor(int index);

int Elevator_HasAnyRequest(const Elevator *elevator);
int Elevator_HasRequestAtFloor(const Elevator *elevator, int floor);
int Elevator_AddRequest(Elevator *elevator, int floor);
int Elevator_ClearRequest(Elevator *elevator, int floor);

int Elevator_FindNextTarget(const Elevator *elevator);
void Elevator_UpdateDirection(Elevator *elevator);
void Elevator_MoveOneFloor(Elevator *elevator);
void Elevator_RunOneStep(Elevator *elevator);
void Elevator_RunUntilIdle(Elevator *elevator);

void Elevator_OpenDoor(Elevator *elevator);
void Elevator_HoldDoor(Elevator *elevator);
void Elevator_CloseDoor(Elevator *elevator);

void Elevator_PrintStatus(const Elevator *elevator);
void Elevator_PrintRequests(const Elevator *elevator);

const char *Elevator_GetStateName(ElevatorState state);
const char *Elevator_GetDirectionName(ElevatorDirection direction);
const char *Elevator_GetDoorName(DoorState door);

#endif
