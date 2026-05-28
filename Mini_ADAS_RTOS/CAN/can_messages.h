#ifndef __CAN_MESSAGES_H
#define __CAN_MESSAGES_H

#include <stdint.h>

/* CAN Message IDs for ADAS System */
#define CAN_ID_SENSOR_DATA    0x100  /* Sensor data: obstacles, lanes */
#define CAN_ID_DECISION_ALERT 0x200  /* Decision engine alerts */
#define CAN_ID_SYSTEM_STATUS  0x300  /* System health and status */

/* CAN Message Structures */
typedef struct
{
    uint8_t lane_status;
    uint8_t obstacle_distance;
    uint8_t reserved[6];
} CAN_SensorData_t;

typedef struct
{
    uint8_t lane_warning;
    uint8_t obstacle_warning;
    uint8_t risk_level;
    uint8_t reserved[5];
} CAN_AlertData_t;

typedef struct
{
    uint8_t system_state;
    uint8_t task_count;
    uint8_t error_flags;
    uint8_t reserved[5];
} CAN_SystemStatus_t;

#endif /* __CAN_MESSAGES_H */
