/**
 * @file    app_log.c
 * @brief   Thread-safe logging: tasks format into a queue, only LoggerTask
 *          touches the UART.
 *
 * Why: the original code called printf() from several tasks. printf ->
 * HAL_UART_Transmit is blocking and not re-entrant, so lines got interleaved
 * and higher-priority tasks were blocked for milliseconds at 9600 baud.
 * Now each task formats into its own stack buffer and posts the finished line
 * with a zero timeout; if the queue is full the line is dropped and counted
 * (a real-time task must never block on logging).
 */
#include "app.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define LOG_LINE_MAX     88U
#define LOG_QUEUE_DEPTH  16U
#define LOG_UART_TIMEOUT 100U

typedef struct
{
    char text[LOG_LINE_MAX];
} LogLine_t;

static osMessageQueueId_t s_logQueue = NULL;
static volatile uint32_t  s_dropped  = 0U;

void AppLog_CreateQueue(void)
{
    s_logQueue = osMessageQueueNew(LOG_QUEUE_DEPTH, sizeof(LogLine_t), NULL);
    if (s_logQueue == NULL)
    {
        Error_Handler();
    }
}

void AppLog_Printf(const char *fmt, ...)
{
    LogLine_t line;
    va_list   args;
    int       n;

    va_start(args, fmt);
    n = vsnprintf(line.text, sizeof(line.text) - 2U, fmt, args);
    va_end(args);

    if (n < 0)
    {
        return;
    }
    if ((size_t)n > (sizeof(line.text) - 3U))
    {
        n = (int)(sizeof(line.text) - 3U); /* truncated */
    }
    line.text[n]      = '\r';
    line.text[n + 1]  = '\n';
    line.text[n + 2]  = '\0';

    if ((s_logQueue == NULL) || (osKernelGetState() != osKernelRunning))
    {
        /* Before the scheduler runs (boot, early errors): write directly. */
        (void)HAL_UART_Transmit(&huart1, (uint8_t *)line.text,
                                (uint16_t)strlen(line.text), LOG_UART_TIMEOUT);
        return;
    }

    if (osMessageQueuePut(s_logQueue, &line, 0U, 0U) != osOK)
    {
        s_dropped++;
    }
}

uint32_t AppLog_Dropped(void)
{
    return s_dropped;
}

void AppTask_Logger(void)
{
    LogLine_t line;

    for (;;)
    {
        if (osMessageQueueGet(s_logQueue, &line, NULL, APP_TASK_POLL_MS) == osOK)
        {
            (void)HAL_UART_Transmit(&huart1, (uint8_t *)line.text,
                                    (uint16_t)strlen(line.text), LOG_UART_TIMEOUT);
        }
        App_Heartbeat(APP_TASK_LOGGER);
    }
}

/* printf() support: anything that still uses printf goes through the logger
 * path's UART directly. Kept so stray debug printf calls do not hard-fault.
 * Prefer AppLog_Printf() in application code. */
int _write(int file, char *ptr, int len)
{
    (void)file;
    (void)HAL_UART_Transmit(&huart1, (uint8_t *)ptr, (uint16_t)len, LOG_UART_TIMEOUT);
    return len;
}
