#include "elevator.h"

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
