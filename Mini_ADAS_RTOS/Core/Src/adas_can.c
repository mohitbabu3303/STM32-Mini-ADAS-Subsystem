/**
 * @file    adas_can.c
 * @brief   ADAS CAN message pack/unpack (pure C, unit-testable on a PC).
 *
 * All unpack functions return 0 on success and -1 if the ID or DLC does not
 * match, so a corrupted or unexpected frame is never decoded silently.
 */
#include "adas_can.h"

#include <stddef.h>
#include <string.h>

static void frame_init(CanFrame_t *f, uint16_t id, uint8_t dlc)
{
    (void)memset(f, 0, sizeof(*f));
    f->id  = id;
    f->dlc = dlc;
}

void adas_can_pack_sensor(const AdasInput_t *in, uint8_t seq, CanFrame_t *f)
{
    frame_init(f, CAN_ID_SENSOR_DATA, CAN_DLC_SENSOR_DATA);
    f->data[0] = in->lane_departure;
    f->data[1] = in->obstacle_distance_m;
    f->data[2] = in->speed_kmh;
    f->data[3] = seq;
}

int adas_can_unpack_sensor(const CanFrame_t *f, AdasInput_t *in, uint8_t *seq)
{
    if ((f == NULL) || (in == NULL) ||
        (f->id != CAN_ID_SENSOR_DATA) || (f->dlc != CAN_DLC_SENSOR_DATA))
    {
        return -1;
    }
    in->lane_departure      = f->data[0];
    in->obstacle_distance_m = f->data[1];
    in->speed_kmh           = f->data[2];
    if (seq != NULL) { *seq = f->data[3]; }
    return 0;
}

void adas_can_pack_alert(const AdasOutput_t *out, uint8_t seq, CanFrame_t *f)
{
    uint8_t flags = 0U;

    frame_init(f, CAN_ID_DECISION_ALERT, CAN_DLC_DECISION_ALERT);
    if (out->lane_warning != 0U)      { flags |= CAN_ALERT_FLAG_LANE; }
    if (out->obstacle_warning != 0U)  { flags |= CAN_ALERT_FLAG_OBSTACLE; }
    if (out->overspeed_warning != 0U) { flags |= CAN_ALERT_FLAG_OVERSPEED; }
    if (out->active != 0U)            { flags |= CAN_ALERT_FLAG_ACTIVE; }
    f->data[0] = flags;
    f->data[1] = (uint8_t)out->risk;
    f->data[2] = seq;
}

int adas_can_unpack_alert(const CanFrame_t *f, AdasOutput_t *out, uint8_t *seq)
{
    if ((f == NULL) || (out == NULL) ||
        (f->id != CAN_ID_DECISION_ALERT) || (f->dlc != CAN_DLC_DECISION_ALERT) ||
        (f->data[1] >= (uint8_t)ADAS_RISK_COUNT))
    {
        return -1;
    }
    out->lane_warning      = (uint8_t)((f->data[0] & CAN_ALERT_FLAG_LANE) != 0U);
    out->obstacle_warning  = (uint8_t)((f->data[0] & CAN_ALERT_FLAG_OBSTACLE) != 0U);
    out->overspeed_warning = (uint8_t)((f->data[0] & CAN_ALERT_FLAG_OVERSPEED) != 0U);
    out->active            = (uint8_t)((f->data[0] & CAN_ALERT_FLAG_ACTIVE) != 0U);
    out->risk              = (AdasRisk_t)f->data[1];
    out->threshold_m       = 0U; /* not transmitted */
    if (seq != NULL) { *seq = f->data[2]; }
    return 0;
}

void adas_can_pack_status(const SysStatus_t *s, CanFrame_t *f)
{
    frame_init(f, CAN_ID_SYSTEM_STATUS, CAN_DLC_SYSTEM_STATUS);
    f->data[0] = s->state;
    f->data[1] = s->error_flags;
    f->data[2] = (uint8_t)(s->uptime_s & 0xFFU);
    f->data[3] = (uint8_t)(s->uptime_s >> 8);
    f->data[4] = s->min_free_heap_div64;
    f->data[5] = s->seq;
}

int adas_can_unpack_status(const CanFrame_t *f, SysStatus_t *s)
{
    if ((f == NULL) || (s == NULL) ||
        (f->id != CAN_ID_SYSTEM_STATUS) || (f->dlc != CAN_DLC_SYSTEM_STATUS))
    {
        return -1;
    }
    s->state               = f->data[0];
    s->error_flags         = f->data[1];
    s->uptime_s            = (uint16_t)(f->data[2] | ((uint16_t)f->data[3] << 8));
    s->min_free_heap_div64 = f->data[4];
    s->seq                 = f->data[5];
    return 0;
}
