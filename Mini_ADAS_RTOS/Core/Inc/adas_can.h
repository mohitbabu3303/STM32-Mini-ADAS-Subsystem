/**
 * @file    adas_can.h
 * @brief   ADAS CAN message definitions and pack/unpack functions.
 *
 * Pure C (no HAL), so the encoding can be unit-tested on a PC.
 * Byte order for multi-byte signals: little endian (Intel), as in most
 * automotive DBC files.
 *
 *  ID     Name           DLC  Layout
 *  0x100  SENSOR_DATA     4   [0] lane_departure  [1] distance_m  [2] speed_kmh  [3] seq
 *  0x200  DECISION_ALERT  3   [0] flags (b0 lane, b1 obstacle, b2 overspeed, b3 active)
 *                             [1] risk level (AdasRisk_t)         [2] seq
 *  0x300  SYSTEM_STATUS   6   [0] state (0 OK, 1 DEGRADED)  [1] error flags
 *                             [2..3] uptime_s (LE)  [4] min free heap / 64  [5] seq
 */
#ifndef ADAS_CAN_H
#define ADAS_CAN_H

#include <stdint.h>
#include "adas_logic.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CAN_ID_SENSOR_DATA      0x100U
#define CAN_ID_DECISION_ALERT   0x200U
#define CAN_ID_SYSTEM_STATUS    0x300U

#define CAN_DLC_SENSOR_DATA     4U
#define CAN_DLC_DECISION_ALERT  3U
#define CAN_DLC_SYSTEM_STATUS   6U

/* Alert flag bits (byte 0 of 0x200) */
#define CAN_ALERT_FLAG_LANE       (1U << 0)
#define CAN_ALERT_FLAG_OBSTACLE   (1U << 1)
#define CAN_ALERT_FLAG_OVERSPEED  (1U << 2)
#define CAN_ALERT_FLAG_ACTIVE     (1U << 3)

/* System status error flags (byte 1 of 0x300) */
#define SYS_ERR_TASK_STALLED      (1U << 0)
#define SYS_ERR_LOW_STACK         (1U << 1)
#define SYS_ERR_LOW_HEAP          (1U << 2)
#define SYS_ERR_QUEUE_OVERFLOW    (1U << 3)
#define SYS_ERR_CAN_BUS           (1U << 4)
#define SYS_ERR_CAN_LOOPBACK      (1U << 5)

typedef enum
{
    SYS_STATE_OK = 0,
    SYS_STATE_DEGRADED = 1
} SysState_t;

/* Generic CAN frame used by the RTOS queues (standard 11-bit IDs only). */
typedef struct
{
    uint16_t id;
    uint8_t  dlc;
    uint8_t  data[8];
} CanFrame_t;

typedef struct
{
    uint8_t    state;
    uint8_t    error_flags;
    uint16_t   uptime_s;
    uint8_t    min_free_heap_div64;
    uint8_t    seq;
} SysStatus_t;

void    adas_can_pack_sensor(const AdasInput_t *in, uint8_t seq, CanFrame_t *f);
int     adas_can_unpack_sensor(const CanFrame_t *f, AdasInput_t *in, uint8_t *seq);

void    adas_can_pack_alert(const AdasOutput_t *out, uint8_t seq, CanFrame_t *f);
int     adas_can_unpack_alert(const CanFrame_t *f, AdasOutput_t *out, uint8_t *seq);

void    adas_can_pack_status(const SysStatus_t *s, CanFrame_t *f);
int     adas_can_unpack_status(const CanFrame_t *f, SysStatus_t *s);

#ifdef __cplusplus
}
#endif

#endif /* ADAS_CAN_H */
