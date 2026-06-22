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

#define MAX_LOAD_KG 1000
#define RECOVERY_MOVE_TIME 5
#define LOBBY_FLOOR 1
#define IDLE_RETURN_DELAY_SECONDS 60
#define IDLE_STEP_SECONDS 10

typedef enum
{
    ELEVATOR_IDLE,
    ELEVATOR_MOVING,
    ELEVATOR_DOOR_OPENING,
    ELEVATOR_DOOR_HOLDING,
    ELEVATOR_DOOR_CLOSING,
    ELEVATOR_FAULT,
    ELEVATOR_PAUSED,
    ELEVATOR_POWER_OFF,
    ELEVATOR_RECOVERING
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

typedef enum
{
    FAULT_NONE,
    FAULT_DOOR,
    FAULT_MOTOR,
    FAULT_SENSOR,
    FAULT_UNKNOWN
} FaultType;

typedef enum
{
    ELEVATOR_EVENT_NULL_ELEVATOR = 0,
    ELEVATOR_EVENT_OK = 1,
    ELEVATOR_EVENT_INVALID_FLOOR = -1,
    ELEVATOR_EVENT_REQUEST_EXISTS = -2,
    ELEVATOR_EVENT_POWER_OFF = -3,
    ELEVATOR_EVENT_DOOR_NOT_ALIGNED = -4,
    ELEVATOR_EVENT_DOOR_CLOSE_BLOCKED = -5,
    ELEVATOR_EVENT_ALREADY_ACTIVE = -6,
    ELEVATOR_EVENT_NO_RECOVERY_NEEDED = -7,
    ELEVATOR_EVENT_BACKUP_POWER_UNAVAILABLE = -8,
    ELEVATOR_EVENT_NOT_BETWEEN_FLOORS = -9,
    ELEVATOR_EVENT_INVALID_FAULT = -10,
    ELEVATOR_EVENT_RECOVERY_BLOCKED = -11
} ElevatorEventResult;

typedef struct
{
    int currentFloor;
    int targetFloor;
    int totalTimeSeconds;
    int idleTimeSeconds;
    ElevatorState state;
    ElevatorDirection direction;
    DoorState door;
    DoorState carDoor;
    DoorState landingDoors[TOTAL_FLOOR_COUNT];
    int landingDoorLocked[TOTAL_FLOOR_COUNT];
    int isAlignedWithFloor;
    FaultType fault;

    int currentLoadKg;
    int isOverloaded;
    int isDoorBlocked;
    int isDoorOpenButtonHeld;
    int isEmergencyCallActive;
    int isAdminPaused;
    int isPowerOff;
    int isMainPowerOn;
    int isBackupPowerAvailable;
    int isBackupPowerActive;
    int isPowerFailure;
    int isRecovering;
    int isBetweenFloors;
    int safeFloor;
    int rescueFloor;
    ElevatorDirection directionBeforePowerFailure;

    /*
     * Hall requests come from the outside panel on each floor.
     * Car requests come from the buttons inside the elevator.
     */
    int hallUpRequests[TOTAL_FLOOR_COUNT];
    int hallDownRequests[TOTAL_FLOOR_COUNT];
    int carFloorRequests[TOTAL_FLOOR_COUNT];

    int hallUpRequestCreatedAt[TOTAL_FLOOR_COUNT];
    int hallDownRequestCreatedAt[TOTAL_FLOOR_COUNT];
    int carRequestCreatedAt[TOTAL_FLOOR_COUNT];

    int completedRequestCount;
    int totalWaitTimeSeconds;
    int longestWaitTimeSeconds;
} Elevator;

/*
 * A snapshot is a read-only copy of the elevator state.
 *
 * The command-line menu can print it, and a future GUI can use it to draw
 * the inside panel, outside floor buttons, request lights, and status display.
 */
typedef struct
{
    int currentFloor;
    int targetFloor;
    int currentFloorIndex;
    int targetFloorIndex;
    int totalTimeSeconds;
    int idleTimeSeconds;
    ElevatorState state;
    ElevatorDirection direction;
    DoorState door;
    DoorState carDoor;
    DoorState landingDoors[TOTAL_FLOOR_COUNT];
    int landingDoorLocked[TOTAL_FLOOR_COUNT];
    DoorState currentLandingDoor;
    int currentLandingDoorLocked;
    int areAllLandingDoorsLocked;
    int isAlignedWithFloor;
    FaultType fault;

    int currentLoadKg;
    int isOverloaded;
    int isDoorBlocked;
    int isDoorOpenButtonHeld;
    int isEmergencyCallActive;
    int isAdminPaused;
    int isPowerOff;
    int isMainPowerOn;
    int isBackupPowerAvailable;
    int isBackupPowerActive;
    int isPowerFailure;
    int isRecovering;
    int isBetweenFloors;
    int safeFloor;
    int rescueFloor;
    ElevatorDirection directionBeforePowerFailure;
    int canMove;
    int canCloseDoor;

    int hallUpRequests[TOTAL_FLOOR_COUNT];
    int hallDownRequests[TOTAL_FLOOR_COUNT];
    int carFloorRequests[TOTAL_FLOOR_COUNT];
    int hasAnyRequest;

    int completedRequestCount;
    int totalWaitTimeSeconds;
    int longestWaitTimeSeconds;
    double averageWaitTimeSeconds;
} ElevatorSnapshot;

void Elevator_Init(Elevator *elevator);
void Elevator_GetSnapshot(const Elevator *elevator, ElevatorSnapshot *snapshot);

/* Floor helpers */
int Elevator_IsValidFloor(int floor);
int Elevator_FloorToIndex(int floor);
int Elevator_IndexToFloor(int index);

/* Request state helpers and button events */
int Elevator_HasAnyRequest(const Elevator *elevator);
int Elevator_HasRequestAtFloor(const Elevator *elevator, int floor);
ElevatorEventResult Elevator_AddRequest(Elevator *elevator, int floor);
ElevatorEventResult Elevator_AddHallUpRequest(Elevator *elevator, int floor);
ElevatorEventResult Elevator_AddHallDownRequest(Elevator *elevator, int floor);
ElevatorEventResult Elevator_AddCarRequest(Elevator *elevator, int floor);
ElevatorEventResult Elevator_PressHallUpButton(Elevator *elevator, int floor);
ElevatorEventResult Elevator_PressHallDownButton(Elevator *elevator, int floor);
ElevatorEventResult Elevator_PressCarFloorButton(Elevator *elevator, int floor);
int Elevator_ClearRequest(Elevator *elevator, int floor);
int Elevator_IsValidHallUpFloor(int floor);
int Elevator_IsValidHallDownFloor(int floor);

/* Simulation step controls */
void Elevator_UpdateDirection(Elevator *elevator);
void Elevator_MoveOneFloor(Elevator *elevator);
void Elevator_RunOneStep(Elevator *elevator);
void Elevator_RunUntilIdle(Elevator *elevator);

/* Door and cabin button events */
void Elevator_OpenDoor(Elevator *elevator);
void Elevator_HoldDoor(Elevator *elevator);
void Elevator_CloseDoor(Elevator *elevator);
ElevatorEventResult Elevator_PressDoorOpenButton(Elevator *elevator);
ElevatorEventResult Elevator_ReleaseDoorOpenButton(Elevator *elevator);
ElevatorEventResult Elevator_PressDoorCloseButton(Elevator *elevator);
ElevatorEventResult Elevator_PressEmergencyCallButton(Elevator *elevator);
ElevatorEventResult Elevator_ClearEmergencyCall(Elevator *elevator);

/* Safety, management, and power controls */
ElevatorEventResult Elevator_SetLoad(Elevator *elevator, int loadKg);
ElevatorEventResult Elevator_SetDoorBlocked(Elevator *elevator, int isBlocked);
ElevatorEventResult Elevator_SetFault(Elevator *elevator, FaultType fault);
ElevatorEventResult Elevator_ClearFault(Elevator *elevator);
ElevatorEventResult Elevator_AdminPause(Elevator *elevator);
ElevatorEventResult Elevator_AdminResume(Elevator *elevator);
ElevatorEventResult Elevator_PowerOff(Elevator *elevator);
ElevatorEventResult Elevator_RestorePower(Elevator *elevator);
ElevatorEventResult Elevator_SimulatePowerFailure(Elevator *elevator);
ElevatorEventResult Elevator_RunBackupRescue(Elevator *elevator);
ElevatorEventResult Elevator_SetBackupPowerAvailable(Elevator *elevator, int isAvailable);
ElevatorEventResult Elevator_SetBetweenFloors(Elevator *elevator, int safeFloor);
ElevatorEventResult Elevator_RunRecovery(Elevator *elevator);

/* Command-line display helpers */
void Elevator_PrintStatus(const Elevator *elevator);
void Elevator_PrintRequests(const Elevator *elevator);
void Elevator_PrintStats(const Elevator *elevator);
void Elevator_PrintVisualPanel(const Elevator *elevator);

/* GUI-friendly display helpers */
const char *Elevator_GetStateName(ElevatorState state);
const char *Elevator_GetDirectionName(ElevatorDirection direction);
const char *Elevator_GetDoorName(DoorState door);
const char *Elevator_GetFaultName(FaultType fault);
const char *Elevator_GetEventResultName(ElevatorEventResult result);
int Elevator_IsEventSuccess(ElevatorEventResult result);

#endif
