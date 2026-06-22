#include "elevator_internal.h"

const char *Elevator_GetStateName(ElevatorState state)
{
    switch (state)
    {
    case ELEVATOR_IDLE:
        return "Idle";
    case ELEVATOR_MOVING:
        return "Moving";
    case ELEVATOR_DOOR_OPENING:
        return "Door opening";
    case ELEVATOR_DOOR_HOLDING:
        return "Door holding";
    case ELEVATOR_DOOR_CLOSING:
        return "Door closing";
    case ELEVATOR_FAULT:
        return "Fault";
    case ELEVATOR_PAUSED:
        return "Paused";
    case ELEVATOR_POWER_OFF:
        return "Power off";
    case ELEVATOR_RECOVERING:
        return "Recovering";
    default:
        return "Unknown";
    }
}

const char *Elevator_GetDirectionName(ElevatorDirection direction)
{
    switch (direction)
    {
    case DIRECTION_NONE:
        return "None";
    case DIRECTION_UP:
        return "Up";
    case DIRECTION_DOWN:
        return "Down";
    default:
        return "Unknown";
    }
}

const char *Elevator_GetFaultName(FaultType fault)
{
    switch (fault)
    {
    case FAULT_NONE:
        return "None";
    case FAULT_DOOR:
        return "Door fault";
    case FAULT_MOTOR:
        return "Motor fault";
    case FAULT_SENSOR:
        return "Sensor fault";
    case FAULT_UNKNOWN:
        return "Unknown fault";
    default:
        return "Invalid fault";
    }
}

const char *Elevator_GetDoorName(DoorState door)
{
    switch (door)
    {
    case DOOR_CLOSED:
        return "Closed";
    case DOOR_OPEN:
        return "Open";
    default:
        return "Unknown";
    }
}

const char *Elevator_GetEventResultName(ElevatorEventResult result)
{
    switch (result)
    {
    case ELEVATOR_EVENT_OK:
        return "OK";
    case ELEVATOR_EVENT_NULL_ELEVATOR:
        return "Null elevator";
    case ELEVATOR_EVENT_INVALID_FLOOR:
        return "Invalid floor";
    case ELEVATOR_EVENT_REQUEST_EXISTS:
        return "Request already exists";
    case ELEVATOR_EVENT_POWER_OFF:
        return "Power is off";
    case ELEVATOR_EVENT_DOOR_NOT_ALIGNED:
        return "Door is not aligned with floor";
    case ELEVATOR_EVENT_DOOR_CLOSE_BLOCKED:
        return "Door close blocked";
    default:
        return "Unknown event result";
    }
}
