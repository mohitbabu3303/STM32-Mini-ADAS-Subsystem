/**
 * @file    adas_logic.c
 * @brief   Platform-independent ADAS decision logic (shared by firmware + SIL).
 */
#include "adas_logic.h"

#include <stddef.h>

static AdasRisk_t max_risk(AdasRisk_t a, AdasRisk_t b)
{
    return (a > b) ? a : b;
}

uint8_t adas_obstacle_threshold_m(uint8_t speed_kmh)
{
    /* Rough stopping-distance table: faster vehicle -> earlier warning. */
    if (speed_kmh <= 30U)  { return 10U; }
    if (speed_kmh <= 50U)  { return 20U; }
    if (speed_kmh <= 80U)  { return 30U; }
    if (speed_kmh <= 100U) { return 40U; }
    return 50U;
}

void adas_evaluate(const AdasInput_t *in, AdasOutput_t *out)
{
    AdasRisk_t risk = ADAS_RISK_SAFE;

    if ((in == NULL) || (out == NULL))
    {
        return;
    }

    out->active            = (in->speed_kmh >= ADAS_MIN_ACTIVE_SPEED_KMH) ? 1U : 0U;
    out->threshold_m       = adas_obstacle_threshold_m(in->speed_kmh);
    out->lane_warning      = 0U;
    out->obstacle_warning  = 0U;
    out->overspeed_warning = 0U;

    if (out->active == 0U)
    {
        /* Low-speed manoeuvring (parking, traffic jam): no warnings. */
        out->risk = ADAS_RISK_SAFE;
        return;
    }

    out->lane_warning      = (in->lane_departure != 0U) ? 1U : 0U;
    out->overspeed_warning = (in->speed_kmh > ADAS_OVERSPEED_KMH) ? 1U : 0U;
    out->obstacle_warning  = (in->obstacle_distance_m < out->threshold_m) ? 1U : 0U;

    /* Overall risk = worst of the individual contributions. */
    if (out->lane_warning != 0U)
    {
        risk = max_risk(risk, ADAS_RISK_LOW);
    }
    if (out->overspeed_warning != 0U)
    {
        risk = max_risk(risk, ADAS_RISK_MEDIUM);
    }
    if (out->obstacle_warning != 0U)
    {
        if (in->obstacle_distance_m <= ADAS_CRITICAL_DISTANCE_M)
        {
            risk = max_risk(risk, ADAS_RISK_CRITICAL);
        }
        else if (in->obstacle_distance_m < (uint8_t)(out->threshold_m / 2U))
        {
            risk = max_risk(risk, ADAS_RISK_HIGH);
        }
        else
        {
            risk = max_risk(risk, ADAS_RISK_MEDIUM);
        }
    }

    out->risk = risk;
}

uint8_t adas_any_warning(const AdasOutput_t *out)
{
    if (out == NULL)
    {
        return 0U;
    }
    return (uint8_t)((out->lane_warning | out->obstacle_warning | out->overspeed_warning) != 0U);
}

const char *adas_risk_name(AdasRisk_t risk)
{
    static const char *const names[ADAS_RISK_COUNT] =
    {
        "SAFE", "LOW", "MEDIUM", "HIGH", "CRITICAL"
    };
    return ((unsigned)risk < (unsigned)ADAS_RISK_COUNT) ? names[risk] : "INVALID";
}

const char *adas_risk_action(AdasRisk_t risk)
{
    static const char *const actions[ADAS_RISK_COUNT] =
    {
        "No action", "Stay alert", "Reduce speed", "Slow down immediately", "APPLY BRAKES!"
    };
    return ((unsigned)risk < (unsigned)ADAS_RISK_COUNT) ? actions[risk] : "INVALID";
}

void adas_filter_init(AdasDistanceFilter_t *f)
{
    uint8_t i;

    if (f == NULL)
    {
        return;
    }
    for (i = 0U; i < ADAS_FILTER_LEN; i++)
    {
        f->samples[i] = 0U;
    }
    f->sum   = 0U;
    f->index = 0U;
    f->count = 0U;
}

uint8_t adas_filter_update(AdasDistanceFilter_t *f, uint8_t raw_distance_m)
{
    if (f == NULL)
    {
        return raw_distance_m;
    }

    /* "No obstacle" is not averaged: it resets the filter so a newly
     * appearing target is reported without lag from stale samples. */
    if (raw_distance_m == ADAS_NO_OBSTACLE_M)
    {
        adas_filter_init(f);
        return ADAS_NO_OBSTACLE_M;
    }

    uint8_t avg;

    f->sum = (uint16_t)(f->sum - f->samples[f->index] + raw_distance_m);
    f->samples[f->index] = raw_distance_m;
    f->index = (uint8_t)((f->index + 1U) % ADAS_FILTER_LEN);
    if (f->count < ADAS_FILTER_LEN)
    {
        f->count++;
    }
    avg = (uint8_t)(f->sum / f->count);

    /* Fail-safe: a plain moving average lags behind an approaching target
     * (at 4 m it would still report ~12 m and miss CRITICAL). Returning
     * min(raw, average) reacts immediately when the gap closes and only
     * smooths noise while the target moves away - the safe direction. */
    return (raw_distance_m < avg) ? raw_distance_m : avg;
}
