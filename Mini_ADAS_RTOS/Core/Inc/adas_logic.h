/**
 * @file    adas_logic.h
 * @brief   Platform-independent ADAS decision logic.
 *
 * This module has NO dependency on HAL or FreeRTOS. The same source file is
 * compiled into the STM32 firmware and into the PC test harness
 * (ADAS_Testing/test.c), so the SIL tests exercise the real firmware logic.
 */
#ifndef ADAS_LOGIC_H
#define ADAS_LOGIC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Calibration constants -------------------------------------------- */
#define ADAS_MIN_ACTIVE_SPEED_KMH   30U   /* ADAS inactive below this speed   */
#define ADAS_OVERSPEED_KMH          120U  /* overspeed warning above this     */
#define ADAS_CRITICAL_DISTANCE_M    5U    /* at/below: collision imminent     */
#define ADAS_NO_OBSTACLE_M          255U  /* sensor value meaning "no target" */
#define ADAS_FILTER_LEN             4U    /* moving-average window (samples)  */

/* ---- Types --------------------------------------------------------------- */
typedef enum
{
    ADAS_RISK_SAFE = 0,
    ADAS_RISK_LOW,
    ADAS_RISK_MEDIUM,
    ADAS_RISK_HIGH,
    ADAS_RISK_CRITICAL,
    ADAS_RISK_COUNT
} AdasRisk_t;

typedef struct
{
    uint8_t lane_departure;      /* 0 = in lane, 1 = drifting out of lane */
    uint8_t obstacle_distance_m; /* 0..254 m, 255 = no obstacle          */
    uint8_t speed_kmh;           /* 0..255 km/h                          */
} AdasInput_t;

typedef struct
{
    uint8_t    active;            /* 1 if speed >= ADAS_MIN_ACTIVE_SPEED_KMH */
    uint8_t    lane_warning;
    uint8_t    obstacle_warning;
    uint8_t    overspeed_warning;
    uint8_t    threshold_m;       /* speed-dependent obstacle threshold      */
    AdasRisk_t risk;
} AdasOutput_t;

typedef struct
{
    uint16_t samples[ADAS_FILTER_LEN];
    uint16_t sum;
    uint8_t  index;
    uint8_t  count;
} AdasDistanceFilter_t;

/* ---- API ------------------------------------------------------------------ */
uint8_t     adas_obstacle_threshold_m(uint8_t speed_kmh);
void        adas_evaluate(const AdasInput_t *in, AdasOutput_t *out);
uint8_t     adas_any_warning(const AdasOutput_t *out);
const char *adas_risk_name(AdasRisk_t risk);
const char *adas_risk_action(AdasRisk_t risk);

void        adas_filter_init(AdasDistanceFilter_t *f);
uint8_t     adas_filter_update(AdasDistanceFilter_t *f, uint8_t raw_distance_m);

#ifdef __cplusplus
}
#endif

#endif /* ADAS_LOGIC_H */
