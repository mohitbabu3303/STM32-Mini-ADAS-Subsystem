/**
 * @file    adas_simulation.c
 * @brief   PC simulation of the Mini ADAS pipeline using the firmware logic.
 *
 * Runs the same drive scenario, distance filter and decision logic as the
 * STM32 firmware (SensorTask -> DecisionTask -> AlertTask) and prints the
 * same UART line format. Save the output and open it in the dashboard:
 *
 *   cd ADAS_Testing && make adas_sim
 *   ./adas_sim > sim_log.txt
 *   streamlit run dashboard.py      (choose sim_log.txt)
 *
 * The firmware version of the scenario lives in Core/Src/app_tasks.c.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "adas_logic.h"

#define SAMPLE_PERIOD_MS  500U
#define LOOPS             2U

typedef struct
{
    uint8_t lane;
    uint8_t dist_m;
    uint8_t speed_kmh;
} ScenarioStep_t;

/* Same table as k_scenario[] in the firmware (app_tasks.c). */
static const ScenarioStep_t k_scenario[] =
{
    { 0U, 255U,  20U }, { 1U, 255U,  25U }, { 0U, 255U,  50U }, { 0U, 255U,  80U },
    { 0U, 120U,  90U }, { 0U,  80U,  95U }, { 0U,  60U, 100U }, { 0U,  45U, 100U },
    { 0U,  35U, 100U }, { 0U,  25U,  90U }, { 0U,  15U,  70U }, { 0U,   8U,  50U },
    { 0U,   4U,  35U }, { 0U,  30U,  40U }, { 1U, 255U,  70U }, { 1U, 255U,  75U },
    { 0U, 255U, 110U }, { 0U, 255U, 130U }, { 0U, 255U, 140U }, { 0U,  90U, 125U },
    { 0U, 255U,  90U },
};
#define SCENARIO_LEN  (sizeof(k_scenario) / sizeof(k_scenario[0]))

static uint32_t s_lcg = 0x1234ABCDU; /* same seed as firmware -> same noise */

static int8_t sensor_noise_m(void)
{
    s_lcg = (s_lcg * 1103515245U) + 12345U;
    return (int8_t)((int32_t)((s_lcg >> 16) % 5U) - 2);
}

int main(void)
{
    AdasDistanceFilter_t filter;
    AdasOutput_t last;
    uint32_t t_ms = 0U;
    uint32_t n;

    adas_filter_init(&filter);
    memset(&last, 0, sizeof(last));

    printf("=== STM32 Mini ADAS - PC simulation (firmware logic) ===\n");
    printf("Format: DATA,ms,lane,dist_m,speed_kmh,thr_m,lane_w,obst_w,ovspd_w,risk\n");

    for (n = 0U; n < (uint32_t)(SCENARIO_LEN * LOOPS); n++)
    {
        const ScenarioStep_t *s = &k_scenario[n % SCENARIO_LEN];
        AdasInput_t in;
        AdasOutput_t out;
        int16_t raw = (int16_t)s->dist_m;

        /* SensorTask */
        if (s->dist_m != ADAS_NO_OBSTACLE_M)
        {
            raw = (int16_t)(raw + sensor_noise_m());
            if (raw < 0)   { raw = 0; }
            if (raw > 254) { raw = 254; }
        }
        in.lane_departure      = s->lane;
        in.speed_kmh           = s->speed_kmh;
        in.obstacle_distance_m = adas_filter_update(&filter, (uint8_t)raw);

        /* DecisionTask */
        adas_evaluate(&in, &out);
        printf("DATA,%lu,%u,%u,%u,%u,%u,%u,%u,%s\n",
               (unsigned long)t_ms, in.lane_departure, in.obstacle_distance_m, in.speed_kmh,
               out.threshold_m, out.lane_warning, out.obstacle_warning,
               out.overspeed_warning, adas_risk_name(out.risk));

        /* AlertTask: report changes only */
        if ((out.risk != last.risk) || (out.lane_warning != last.lane_warning) ||
            (out.obstacle_warning != last.obstacle_warning) ||
            (out.overspeed_warning != last.overspeed_warning))
        {
            printf("ALERT: risk=%s action=\"%s\"%s%s%s\n",
                   adas_risk_name(out.risk), adas_risk_action(out.risk),
                   out.lane_warning ? " [LANE]" : "",
                   out.obstacle_warning ? " [OBSTACLE]" : "",
                   out.overspeed_warning ? " [OVERSPEED]" : "");
            last = out;
        }
        t_ms += SAMPLE_PERIOD_MS;
    }
    return 0;
}
