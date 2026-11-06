/**
 * @file    test.c
 * @brief   Software-in-the-Loop test runner for the Mini ADAS firmware logic.
 *
 * This file compiles the SAME adas_logic.c / adas_can.c that run on the
 * STM32 (Mini_ADAS_RTOS/Core/Src), so a PASS here means the firmware's
 * decision logic and CAN encoding are correct for these scenarios.
 *
 * Build & run (Linux, macOS, Windows MinGW):
 *   make            (or see the gcc line in the Makefile)
 *   ./adas_test     -> prints PASS/FAIL, writes log.csv, exit code 0 = all pass
 *
 * Timing note: time_ns is measured on the HOST PC (averaged over many calls).
 * It shows the relative cost of the logic, not the execution time on the MCU.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "adas_logic.h"
#include "adas_can.h"

#define TIMING_ITERATIONS  200000UL

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond, msg)                                                     \
    do {                                                                     \
        g_checks++;                                                          \
        if (!(cond)) {                                                       \
            g_failures++;                                                    \
            printf("  FAIL: %s (%s:%d)\n", (msg), __FILE__, __LINE__);       \
        }                                                                    \
    } while (0)

static int risk_from_name(const char *name, AdasRisk_t *risk)
{
    int r;
    for (r = 0; r < (int)ADAS_RISK_COUNT; r++)
    {
        if (strcmp(name, adas_risk_name((AdasRisk_t)r)) == 0)
        {
            *risk = (AdasRisk_t)r;
            return 0;
        }
    }
    return -1;
}

/* Average host execution time of adas_evaluate() for one input, in ns. */
static double measure_ns(const AdasInput_t *in)
{
    volatile uint8_t sink = 0U;
    AdasOutput_t out;
    unsigned long i;
    clock_t start = clock();

    for (i = 0UL; i < TIMING_ITERATIONS; i++)
    {
        adas_evaluate(in, &out);
        sink = (uint8_t)(sink + (uint8_t)out.risk);
    }
    (void)sink;
    return ((double)(clock() - start) / CLOCKS_PER_SEC) * 1e9 / (double)TIMING_ITERATIONS;
}

/* ------------------------------------------------------------------------- */
static int run_scenarios(const char *csv_path, const char *log_path)
{
    FILE *fp = fopen(csv_path, "r");
    FILE *log;
    char line[256];
    int passed = 0;
    int total = 0;

    if (fp == NULL)
    {
        printf("Error: cannot open %s (run from the ADAS_Testing folder)\n", csv_path);
        return -1;
    }
    log = fopen(log_path, "w");
    if (log == NULL)
    {
        printf("Error: cannot create %s\n", log_path);
        fclose(fp);
        return -1;
    }

    fprintf(log, "id,lane,dist,speed,threshold,expected_alert,actual_alert,"
                 "expected_risk,actual_risk,result,time_ns\n");

    if (fgets(line, sizeof(line), fp) == NULL) /* header */
    {
        fclose(fp);
        fclose(log);
        return -1;
    }

    printf("\n[1] Scenario tests (%s)\n", csv_path);
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        int id, lane, dist, speed, expected_alert;
        char risk_name[16];
        AdasRisk_t expected_risk;
        AdasInput_t in;
        AdasOutput_t out;
        int ok;
        double ns;

        if (sscanf(line, "%d,%d,%d,%d,%d,%15[^,]", &id, &lane, &dist, &speed,
                   &expected_alert, risk_name) != 6)
        {
            continue; /* blank or malformed line */
        }
        if ((lane < 0) || (lane > 1) || (dist < 0) || (dist > 255) ||
            (speed < 0) || (speed > 255) || (risk_from_name(risk_name, &expected_risk) != 0))
        {
            printf("  Test %2d: INVALID INPUT ROW\n", id);
            g_failures++;
            continue;
        }

        in.lane_departure      = (uint8_t)lane;
        in.obstacle_distance_m = (uint8_t)dist;
        in.speed_kmh           = (uint8_t)speed;
        adas_evaluate(&in, &out);
        ns = measure_ns(&in);

        ok = ((int)adas_any_warning(&out) == expected_alert) && (out.risk == expected_risk);
        total++;
        g_checks++;
        if (ok)
        {
            passed++;
        }
        else
        {
            g_failures++;
        }

        printf("  Test %2d: %s  lane=%d dist=%3d speed=%3d -> risk=%-8s (expected %s)\n",
               id, ok ? "PASS" : "FAIL", lane, dist, speed,
               adas_risk_name(out.risk), adas_risk_name(expected_risk));

        fprintf(log, "%d,%d,%d,%d,%u,%d,%u,%s,%s,%s,%.1f\n",
                id, lane, dist, speed, out.threshold_m, expected_alert,
                adas_any_warning(&out), adas_risk_name(expected_risk),
                adas_risk_name(out.risk), ok ? "PASS" : "FAIL", ns);
    }

    fclose(fp);
    fclose(log);
    printf("  -> %d/%d scenarios passed, results in %s\n", passed, total, log_path);
    return (passed == total) ? 0 : 1;
}

/* ------------------------------------------------------------------------- */
static void test_filter(void)
{
    AdasDistanceFilter_t f;

    printf("\n[2] Distance filter unit tests\n");
    adas_filter_init(&f);
    CHECK(adas_filter_update(&f, 40U) == 40U, "first sample passes through");
    CHECK(adas_filter_update(&f, 20U) == 20U, "approaching target reported immediately (fail-safe)");
    CHECK(adas_filter_update(&f, 30U) == 30U, "receding: average of 3 samples (90/3)");
    CHECK(adas_filter_update(&f, 50U) == 35U, "receding: average of 4 samples (140/4)");
    CHECK(adas_filter_update(&f, 70U) == 42U, "oldest sample drops out of window (170/4)");
    CHECK(adas_filter_update(&f, 4U) == 4U, "sudden close target not hidden by filter lag");
    CHECK(adas_filter_update(&f, ADAS_NO_OBSTACLE_M) == ADAS_NO_OBSTACLE_M,
          "no-obstacle value bypasses the filter");
    CHECK(adas_filter_update(&f, 12U) == 12U, "filter restarts after target loss");
    printf("  -> done\n");
}

static void test_can_codec(void)
{
    static const uint8_t speeds[] = { 0U, 29U, 30U, 80U, 121U, 255U };
    static const uint8_t dists[]  = { 0U, 5U, 19U, 50U, 254U, 255U };
    CanFrame_t f;
    size_t i, j;

    printf("\n[3] CAN codec unit tests\n");

    for (i = 0U; i < sizeof(speeds); i++)
    {
        for (j = 0U; j < sizeof(dists); j++)
        {
            AdasInput_t in = { (uint8_t)(j & 1U), dists[j], speeds[i] };
            AdasInput_t in2;
            AdasOutput_t out, out2;
            uint8_t seq = 0U;

            adas_can_pack_sensor(&in, (uint8_t)(i * 10U + j), &f);
            CHECK((f.id == CAN_ID_SENSOR_DATA) && (f.dlc == CAN_DLC_SENSOR_DATA), "sensor id/dlc");
            CHECK(adas_can_unpack_sensor(&f, &in2, &seq) == 0, "sensor unpack ok");
            CHECK((in2.lane_departure == in.lane_departure) &&
                  (in2.obstacle_distance_m == in.obstacle_distance_m) &&
                  (in2.speed_kmh == in.speed_kmh) && (seq == (uint8_t)(i * 10U + j)),
                  "sensor round trip");

            adas_evaluate(&in, &out);
            adas_can_pack_alert(&out, 7U, &f);
            CHECK(adas_can_unpack_alert(&f, &out2, &seq) == 0, "alert unpack ok");
            CHECK((out2.risk == out.risk) && (out2.active == out.active) &&
                  (out2.lane_warning == out.lane_warning) &&
                  (out2.obstacle_warning == out.obstacle_warning) &&
                  (out2.overspeed_warning == out.overspeed_warning) && (seq == 7U),
                  "alert round trip");
        }
    }

    {
        SysStatus_t s = { SYS_STATE_DEGRADED, SYS_ERR_LOW_STACK | SYS_ERR_CAN_BUS, 0xBEEFU, 42U, 9U };
        SysStatus_t s2;
        adas_can_pack_status(&s, &f);
        CHECK((f.data[2] == 0xEFU) && (f.data[3] == 0xBEU), "status uptime little endian");
        CHECK(adas_can_unpack_status(&f, &s2) == 0, "status unpack ok");
        CHECK((s2.state == s.state) && (s2.error_flags == s.error_flags) &&
              (s2.uptime_s == s.uptime_s) && (s2.min_free_heap_div64 == 42U) && (s2.seq == 9U),
              "status round trip");
    }

    /* Robustness: wrong ID, wrong DLC, out-of-range risk must be rejected. */
    {
        AdasInput_t in = { 0U, 10U, 50U };
        AdasInput_t in2;
        AdasOutput_t out;
        adas_can_pack_sensor(&in, 0U, &f);
        f.dlc = 3U;
        CHECK(adas_can_unpack_sensor(&f, &in2, NULL) == -1, "reject wrong DLC");
        f.dlc = CAN_DLC_SENSOR_DATA;
        f.id  = CAN_ID_DECISION_ALERT;
        CHECK(adas_can_unpack_sensor(&f, &in2, NULL) == -1, "reject wrong ID");
        f.id = CAN_ID_DECISION_ALERT;
        f.dlc = CAN_DLC_DECISION_ALERT;
        f.data[1] = (uint8_t)ADAS_RISK_COUNT;
        CHECK(adas_can_unpack_alert(&f, &out, NULL) == -1, "reject invalid risk value");
    }
    printf("  -> done\n");
}

int main(void)
{
    int scen;

    printf("=== Mini ADAS SIL tests (firmware logic: adas_logic.c, adas_can.c) ===\n");
    scen = run_scenarios("test_cases.csv", "log.csv");
    test_filter();
    test_can_codec();

    printf("\n=== %d checks, %d failures -> %s ===\n",
           g_checks, g_failures, (g_failures == 0 && scen == 0) ? "ALL PASS" : "FAILED");
    return (g_failures == 0 && scen == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
