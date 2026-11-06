/**
 * @file    app_tasks.c
 * @brief   Sensor, Decision, Alert and HealthMonitor tasks.
 *
 * Data flow:
 *   SensorTask --sensorQueue--> DecisionTask --alertQueue--> AlertTask (LED)
 *        |                           |
 *        +--- CAN 0x100 ---+         +--- CAN 0x200 ---+
 *                          v                           v
 *                     canTxQueue -> CAN_TxTask -> bxCAN -> (loopback) -> CAN_RxTask
 *   HealthMonitorTask: heartbeats, stack, heap, queue drops, CAN errors -> CAN 0x300
 *   LoggerTask: the only task that writes to USART1.
 */
#include "app.h"

#include <string.h>

#define SENSOR_QUEUE_DEPTH  8U
#define ALERT_QUEUE_DEPTH   8U

static osMessageQueueId_t s_sensorQueue = NULL;
static osMessageQueueId_t s_alertQueue  = NULL;
static volatile uint32_t  s_heartbeat[APP_TASK_COUNT];
static volatile uint32_t  s_sensorQueueDrops = 0U;
static volatile uint32_t  s_alertQueueDrops  = 0U;

volatile AppDebugView_t g_adas_debug;

/* ------------------------------------------------------------------------- */
/* Startup                                                                    */
/* ------------------------------------------------------------------------- */
void App_PrintBootBanner(void)
{
    const char *reset = "UNKNOWN";

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST) != 0U)      { reset = "LOW-POWER"; }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) != 0U) { reset = "WWDG"; }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != 0U) { reset = "IWDG"; }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) != 0U)  { reset = "SOFTWARE"; }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST) != 0U)  { reset = "POWER-ON"; }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) != 0U)  { reset = "NRST PIN"; }
    __HAL_RCC_CLEAR_RESET_FLAGS();

    AppLog_Printf("");
    AppLog_Printf("=== STM32 Mini ADAS (FreeRTOS + CAN) ===");
    AppLog_Printf("Build %s %s | SYSCLK %lu Hz | reset: %s",
                  __DATE__, __TIME__, (unsigned long)HAL_RCC_GetSysClockFreq(), reset);
    AppLog_Printf("CAN mode: %s, 500 kbit/s",
                  (AppCan_IsLoopback() != 0U) ? "SILENT LOOPBACK (no transceiver needed)" : "NORMAL");
    AppLog_Printf("Format: DATA,ms,lane,dist_m,speed_kmh,thr_m,lane_w,obst_w,ovspd_w,risk");
}

void App_CreateRtosObjects(void)
{
    s_sensorQueue = osMessageQueueNew(SENSOR_QUEUE_DEPTH, sizeof(AdasInput_t), NULL);
    s_alertQueue  = osMessageQueueNew(ALERT_QUEUE_DEPTH, sizeof(AdasOutput_t), NULL);
    if ((s_sensorQueue == NULL) || (s_alertQueue == NULL))
    {
        Error_Handler();
    }
    AppLog_CreateQueue();
    AppCan_CreateQueues();
}

void App_Heartbeat(AppTaskId_t id)
{
    if ((unsigned)id < (unsigned)APP_TASK_COUNT)
    {
        s_heartbeat[id]++;
    }
}

uint32_t App_GetHeartbeat(AppTaskId_t id)
{
    return ((unsigned)id < (unsigned)APP_TASK_COUNT) ? s_heartbeat[id] : 0U;
}

/* ------------------------------------------------------------------------- */
/* SensorTask: simulated sensor front end                                     */
/* ------------------------------------------------------------------------- */
/* Deterministic drive scenario (one step per APP_SENSOR_PERIOD_MS). Replace
 * this table with real sensor drivers (HC-SR04 / TF-Luna / camera over UART)
 * without touching the rest of the pipeline. */
typedef struct
{
    uint8_t lane;
    uint8_t dist_m;
    uint8_t speed_kmh;
} ScenarioStep_t;

static const ScenarioStep_t k_scenario[] =
{
    /* lane dist  speed */
    { 0U, 255U,  20U },  /* parking lot, ADAS inactive          */
    { 1U, 255U,  25U },  /* lane line crossed at low speed       */
    { 0U, 255U,  50U },  /* town driving, clear road             */
    { 0U, 255U,  80U },
    { 0U, 120U,  90U },  /* vehicle ahead detected               */
    { 0U,  80U,  95U },
    { 0U,  60U, 100U },
    { 0U,  45U, 100U },  /* inside 50 m threshold (>100 km/h)    */
    { 0U,  35U, 100U },
    { 0U,  25U,  90U },
    { 0U,  15U,  70U },  /* driver braking                       */
    { 0U,   8U,  50U },
    { 0U,   4U,  35U },  /* collision imminent                   */
    { 0U,  30U,  40U },  /* gap opens again                      */
    { 1U, 255U,  70U },  /* lane drift on country road           */
    { 1U, 255U,  75U },
    { 0U, 255U, 110U },  /* motorway                             */
    { 0U, 255U, 130U },  /* overspeed                            */
    { 0U, 255U, 140U },
    { 0U,  90U, 125U },
    { 0U, 255U,  90U },
};
#define SCENARIO_LEN  (sizeof(k_scenario) / sizeof(k_scenario[0]))

static uint32_t s_lcg = 0x1234ABCDU;

static int8_t sensor_noise_m(void)
{
    /* +-2 m pseudo-random measurement noise (LCG, no libc rand()). */
    s_lcg = (s_lcg * 1103515245U) + 12345U;
    return (int8_t)((int32_t)((s_lcg >> 16) % 5U) - 2);
}

void AppTask_Sensor(void)
{
    AdasDistanceFilter_t filter;
    AdasInput_t          sample;
    CanFrame_t           frame;
    uint32_t             step = 0U;
    uint8_t              seq  = 0U;
    uint32_t             next_wake;
    int16_t              raw;

    adas_filter_init(&filter);
    next_wake = osKernelGetTickCount();

    for (;;)
    {
        const ScenarioStep_t *s = &k_scenario[step];

        raw = (int16_t)s->dist_m;
        if (s->dist_m != ADAS_NO_OBSTACLE_M)
        {
            raw = (int16_t)(raw + sensor_noise_m());
            if (raw < 0)   { raw = 0; }
            if (raw > 254) { raw = 254; }
        }

        sample.lane_departure      = s->lane;
        sample.speed_kmh           = s->speed_kmh;
        sample.obstacle_distance_m = adas_filter_update(&filter, (uint8_t)raw);

        if (osMessageQueuePut(s_sensorQueue, &sample, 0U, 0U) != osOK)
        {
            s_sensorQueueDrops++;
        }

        adas_can_pack_sensor(&sample, seq++, &frame);
        AppCan_Send(&frame);

        step = (step + 1U) % SCENARIO_LEN;
        App_Heartbeat(APP_TASK_SENSOR);

        /* Fixed-rate sampling (no drift), unlike osDelay(). */
        next_wake += APP_SENSOR_PERIOD_MS;
        (void)osDelayUntil(next_wake);
    }
}

/* ------------------------------------------------------------------------- */
/* DecisionTask: runs the shared ADAS logic on every sample                   */
/* ------------------------------------------------------------------------- */
void AppTask_Decision(void)
{
    AdasInput_t  in;
    AdasOutput_t out;
    CanFrame_t   frame;
    uint8_t      seq = 0U;

    for (;;)
    {
        if (osMessageQueueGet(s_sensorQueue, &in, NULL, APP_TASK_POLL_MS) == osOK)
        {
            adas_evaluate(&in, &out);
            g_adas_debug.input  = in;
            g_adas_debug.output = out;
            g_adas_debug.samples++;

            if (osMessageQueuePut(s_alertQueue, &out, 0U, 0U) != osOK)
            {
                s_alertQueueDrops++;
            }

            adas_can_pack_alert(&out, seq++, &frame);
            AppCan_Send(&frame);

            /* Machine-readable line for ADAS_Testing/dashboard.py */
            AppLog_Printf("DATA,%lu,%u,%u,%u,%u,%u,%u,%u,%s",
                          (unsigned long)osKernelGetTickCount(),
                          in.lane_departure, in.obstacle_distance_m, in.speed_kmh,
                          out.threshold_m, out.lane_warning, out.obstacle_warning,
                          out.overspeed_warning, adas_risk_name(out.risk));
        }
        App_Heartbeat(APP_TASK_DECISION);
    }
}

/* ------------------------------------------------------------------------- */
/* AlertTask: drives the alert LED (PA5) and prints alerts on change          */
/* ------------------------------------------------------------------------- */
#define ALERT_TICK_MS  100U

void AppTask_Alert(void)
{
    AdasOutput_t current;
    AdasOutput_t last;
    uint32_t     tick = 0U;
    GPIO_PinState led;

    (void)memset(&current, 0, sizeof(current));
    last = current;

    for (;;)
    {
        if (osMessageQueueGet(s_alertQueue, &current, NULL, ALERT_TICK_MS) == osOK)
        {
            if ((current.risk != last.risk) ||
                (current.lane_warning != last.lane_warning) ||
                (current.obstacle_warning != last.obstacle_warning) ||
                (current.overspeed_warning != last.overspeed_warning))
            {
                AppLog_Printf("ALERT: risk=%s action=\"%s\"%s%s%s",
                              adas_risk_name(current.risk), adas_risk_action(current.risk),
                              (current.lane_warning != 0U) ? " [LANE]" : "",
                              (current.obstacle_warning != 0U) ? " [OBSTACLE]" : "",
                              (current.overspeed_warning != 0U) ? " [OVERSPEED]" : "");
                last = current;
            }
        }
        tick++;

        /* LED pattern by risk: off / solid / 2.5 Hz blink / 5 Hz blink. */
        switch (last.risk)
        {
        case ADAS_RISK_SAFE:     led = GPIO_PIN_RESET; break;
        case ADAS_RISK_LOW:
        case ADAS_RISK_MEDIUM:   led = GPIO_PIN_SET;   break;
        case ADAS_RISK_HIGH:     led = ((tick / 2U) % 2U != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET; break;
        case ADAS_RISK_CRITICAL: led = (tick % 2U != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET; break;
        default:                 led = GPIO_PIN_RESET; break;
        }
        HAL_GPIO_WritePin(ALERT_LED_GPIO_Port, ALERT_LED_Pin, led);

        App_Heartbeat(APP_TASK_ALERT);
    }
}

/* ------------------------------------------------------------------------- */
/* HealthMonitorTask: system supervision, reported on UART and CAN 0x300      */
/* ------------------------------------------------------------------------- */
static const char *const k_task_names[APP_TASK_COUNT] =
{
    "Sensor", "Decision", "Alert", "CAN_Tx", "CAN_Rx", "Logger", "Health"
};

static osThreadId_t task_handle(AppTaskId_t id)
{
    switch (id)
    {
    case APP_TASK_SENSOR:   return SensorTaskHandle;
    case APP_TASK_DECISION: return DecisionTaskHandle;
    case APP_TASK_ALERT:    return AlertTaskHandle;
    case APP_TASK_CAN_TX:   return CAN_TxTaskHandle;
    case APP_TASK_CAN_RX:   return CAN_RxTaskHandle;
    case APP_TASK_LOGGER:   return LoggerTaskHandle;
    case APP_TASK_HEALTH:   return HealthMonitorTaskHandle;
    default:                return NULL;
    }
}

void AppTask_HealthMonitor(void)
{
    uint32_t      last_hb[APP_TASK_COUNT];
    uint8_t       stall_count[APP_TASK_COUNT];
    AppCanStats_t can;
    AppCanStats_t can_prev;
    SysStatus_t   status;
    CanFrame_t    frame;
    uint32_t      next_wake;
    uint32_t      period = 0U;
    uint32_t      prev_drops = 0U;
    uint8_t       prev_flags = 0U;
    uint32_t      i;

    (void)memset(&status, 0, sizeof(status));
    for (i = 0U; i < (uint32_t)APP_TASK_COUNT; i++)
    {
        last_hb[i]     = App_GetHeartbeat((AppTaskId_t)i);
        stall_count[i] = 0U;
    }
    AppCan_GetStats(&can_prev);
    next_wake = osKernelGetTickCount();

    for (;;)
    {
        uint8_t     flags = 0U;
        uint32_t    min_stack = 0xFFFFFFFFU;
        const char *min_stack_task = "-";
        uint32_t    drops;
        uint32_t    esr;
        size_t      heap_min;

        next_wake += APP_HEALTH_PERIOD_MS;
        (void)osDelayUntil(next_wake);
        period++;
        App_Heartbeat(APP_TASK_HEALTH);

        /* Alive indicator: on-board LED PC13 toggles every second. */
        HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);

        /* 1) Task liveness */
        for (i = 0U; i < (uint32_t)APP_TASK_HEALTH; i++)
        {
            uint32_t hb = App_GetHeartbeat((AppTaskId_t)i);
            if (hb == last_hb[i])
            {
                if (stall_count[i] < 0xFFU) { stall_count[i]++; }
                if (stall_count[i] == APP_STALL_PERIODS)
                {
                    AppLog_Printf("HEALTH: task %s stalled", k_task_names[i]);
                }
            }
            else
            {
                stall_count[i] = 0U;
            }
            if (stall_count[i] >= APP_STALL_PERIODS) { flags |= SYS_ERR_TASK_STALLED; }
            last_hb[i] = hb;
        }

        /* 2) Stack head-room (bytes never used since start) */
        for (i = 0U; i < (uint32_t)APP_TASK_COUNT; i++)
        {
            osThreadId_t h = task_handle((AppTaskId_t)i);
            if (h != NULL)
            {
                uint32_t space = osThreadGetStackSpace(h);
                if (space < min_stack)
                {
                    min_stack = space;
                    min_stack_task = k_task_names[i];
                }
            }
        }
        if (min_stack < APP_MIN_STACK_BYTES) { flags |= SYS_ERR_LOW_STACK; }

        /* 3) Heap low-water mark */
        heap_min = xPortGetMinimumEverFreeHeapSize();
        if (heap_min < APP_MIN_HEAP_BYTES) { flags |= SYS_ERR_LOW_HEAP; }

        /* 4) Queue overflows since last period */
        AppCan_GetStats(&can);
        drops = AppLog_Dropped() + s_sensorQueueDrops + s_alertQueueDrops +
                can.tx_queue_drop + can.rx_queue_drop;
        if (drops != prev_drops) { flags |= SYS_ERR_QUEUE_OVERFLOW; }
        prev_drops = drops;

        /* 5) CAN controller error state (bxCAN ESR register) */
        esr = hcan.Instance->ESR;
        if (((esr & CAN_ESR_BOFF) != 0U) || (((esr & CAN_ESR_TEC) >> CAN_ESR_TEC_Pos) > 127U) ||
            (can.tx_err != can_prev.tx_err))
        {
            flags |= SYS_ERR_CAN_BUS;
        }

        /* 6) Loopback self-test: frames sent must come back */
        if ((AppCan_IsLoopback() != 0U) && (can.tx_ok != can_prev.tx_ok) && (can.rx == can_prev.rx))
        {
            flags |= SYS_ERR_CAN_LOOPBACK;
        }
        can_prev = can;

        /* Report */
        if (flags != prev_flags)
        {
            AppLog_Printf("HEALTH: state %s flags=0x%02X",
                          (flags == 0U) ? "OK" : "DEGRADED", flags);
            prev_flags = flags;
        }
        if ((period % APP_HEALTH_REPORT_EVERY) == 0U)
        {
            AppLog_Printf("HEALTH: up=%lus heap_min=%uB stack_min=%luB(%s) can tx=%lu rx=%lu err=%lu drops=%lu",
                          (unsigned long)(osKernelGetTickCount() / 1000U), (unsigned)heap_min,
                          (unsigned long)min_stack, min_stack_task,
                          (unsigned long)can.tx_ok, (unsigned long)can.rx,
                          (unsigned long)can.tx_err, (unsigned long)drops);
        }

        g_adas_debug.health_flags   = flags;
        g_adas_debug.uptime_s       = osKernelGetTickCount() / 1000U;
        g_adas_debug.heap_min_free  = (uint32_t)heap_min;
        g_adas_debug.stack_min_free = min_stack;
        g_adas_debug.can_tx_ok      = can.tx_ok;
        g_adas_debug.can_rx         = can.rx;
        g_adas_debug.can_tx_err     = can.tx_err;
        g_adas_debug.queue_drops    = drops;

        status.state               = (flags == 0U) ? (uint8_t)SYS_STATE_OK : (uint8_t)SYS_STATE_DEGRADED;
        status.error_flags         = flags;
        status.uptime_s            = (uint16_t)(osKernelGetTickCount() / 1000U);
        status.min_free_heap_div64 = (uint8_t)(((heap_min / 64U) > 255U) ? 255U : (heap_min / 64U));
        adas_can_pack_status(&status, &frame);
        status.seq++;
        AppCan_Send(&frame);
    }
}
