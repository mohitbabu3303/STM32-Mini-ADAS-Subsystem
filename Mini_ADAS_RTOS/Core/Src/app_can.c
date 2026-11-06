/**
 * @file    app_can.c
 * @brief   CAN driver glue: filter setup, TX task, ISR-safe RX path, RX task.
 *
 * Design:
 *  - Producers call AppCan_Send() -> canTxQueue (non-blocking).
 *  - CAN_TxTask owns the bxCAN TX mailboxes.
 *  - RX FIFO0 interrupt copies frames into canRxQueue (no printf, no
 *    blocking in the ISR) and CAN_RxTask decodes them in thread context.
 *
 * Default mode is SILENT LOOPBACK (set in CubeMX/MX_CAN_Init): every frame we
 * send is received by our own RX FIFO and nothing is driven on PA12. This
 * lets the whole CAN path run and be debugged on a bare Blue Pill without a
 * transceiver or a second node. For a real bus: add a transceiver
 * (e.g. TJA1050/SN65HVD230), switch the mode to CAN_MODE_NORMAL in CubeMX.
 */
#include "app.h"

#include <string.h>

#define CAN_TX_QUEUE_DEPTH      12U
#define CAN_RX_QUEUE_DEPTH      12U
#define CAN_TX_MAILBOX_WAIT_MS  10U

static osMessageQueueId_t     s_txQueue = NULL;
static osMessageQueueId_t     s_rxQueue = NULL;
static volatile AppCanStats_t s_stats;

uint8_t AppCan_IsLoopback(void)
{
    return (uint8_t)((hcan.Init.Mode == CAN_MODE_LOOPBACK) ||
                     (hcan.Init.Mode == CAN_MODE_SILENT_LOOPBACK));
}

/* Must run BEFORE any FreeRTOS object is created: creating the first queue
 * masks interrupts (BASEPRI) until the scheduler starts, which freezes
 * HAL_GetTick() and would make HAL timeouts inside HAL_CAN_Start hang. */
HAL_StatusTypeDef AppCan_Start(void)
{
    CAN_FilterTypeDef filter = {0};

    /* Bank 0, 32-bit mask mode, mask = 0 -> accept every standard ID. */
    filter.FilterBank           = 0U;
    filter.FilterMode           = CAN_FILTERMODE_IDMASK;
    filter.FilterScale          = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh         = 0x0000U;
    filter.FilterIdLow          = 0x0000U;
    filter.FilterMaskIdHigh     = 0x0000U;
    filter.FilterMaskIdLow      = 0x0000U;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation     = ENABLE;
    filter.SlaveStartFilterBank = 14U; /* ignored on single-CAN F103 */

    if (HAL_CAN_ConfigFilter(&hcan, &filter) != HAL_OK)
    {
        return HAL_ERROR;
    }
    if (HAL_CAN_Start(&hcan) != HAL_OK)
    {
        return HAL_ERROR;
    }
    return HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
}

void AppCan_CreateQueues(void)
{
    s_txQueue = osMessageQueueNew(CAN_TX_QUEUE_DEPTH, sizeof(CanFrame_t), NULL);
    s_rxQueue = osMessageQueueNew(CAN_RX_QUEUE_DEPTH, sizeof(CanFrame_t), NULL);
    if ((s_txQueue == NULL) || (s_rxQueue == NULL))
    {
        Error_Handler();
    }
}

void AppCan_Send(const CanFrame_t *frame)
{
    if ((s_txQueue == NULL) || (osMessageQueuePut(s_txQueue, frame, 0U, 0U) != osOK))
    {
        s_stats.tx_queue_drop++;
    }
}

void AppCan_GetStats(AppCanStats_t *out)
{
    /* Counters are 32-bit and written by a single context each, so a field
     * copy is consistent enough for diagnostics. */
    out->tx_ok         = s_stats.tx_ok;
    out->tx_err        = s_stats.tx_err;
    out->tx_queue_drop = s_stats.tx_queue_drop;
    out->rx            = s_stats.rx;
    out->rx_queue_drop = s_stats.rx_queue_drop;
    out->rx_decode_err = s_stats.rx_decode_err;
    out->rx_sensor     = s_stats.rx_sensor;
    out->rx_alert      = s_stats.rx_alert;
    out->rx_status     = s_stats.rx_status;
}

void AppTask_CanTx(void)
{
    CanFrame_t          frame;
    CAN_TxHeaderTypeDef header;
    uint32_t            mailbox;
    uint32_t            waited;

    for (;;)
    {
        if (osMessageQueueGet(s_txQueue, &frame, NULL, APP_TASK_POLL_MS) == osOK)
        {
            header.StdId              = frame.id;
            header.ExtId              = 0U;
            header.IDE                = CAN_ID_STD;
            header.RTR                = CAN_RTR_DATA;
            header.DLC                = frame.dlc;
            header.TransmitGlobalTime = DISABLE;

            /* 3 hardware mailboxes; wait briefly if all are busy. */
            waited = 0U;
            while ((HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0U) &&
                   (waited < CAN_TX_MAILBOX_WAIT_MS))
            {
                osDelay(1U);
                waited++;
            }

            if (HAL_CAN_AddTxMessage(&hcan, &header, frame.data, &mailbox) == HAL_OK)
            {
                s_stats.tx_ok++;
            }
            else
            {
                s_stats.tx_err++;
            }
        }
        App_Heartbeat(APP_TASK_CAN_TX);
    }
}

/* Runs in interrupt context (USB_LP_CAN1_RX0_IRQn, priority 5 so FreeRTOS
 * "FromISR" APIs are allowed). Keep it short: copy and queue only. */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcanp)
{
    CAN_RxHeaderTypeDef header;
    CanFrame_t          frame;

    while (HAL_CAN_GetRxFifoFillLevel(hcanp, CAN_RX_FIFO0) > 0U)
    {
        if (HAL_CAN_GetRxMessage(hcanp, CAN_RX_FIFO0, &header, frame.data) != HAL_OK)
        {
            break;
        }
        if ((header.IDE != CAN_ID_STD) || (header.RTR != CAN_RTR_DATA))
        {
            continue; /* only standard data frames are used */
        }
        frame.id  = (uint16_t)header.StdId;
        frame.dlc = (uint8_t)header.DLC;
        s_stats.rx++;

        if ((s_rxQueue == NULL) || (osMessageQueuePut(s_rxQueue, &frame, 0U, 0U) != osOK))
        {
            s_stats.rx_queue_drop++;
        }
    }
}

void AppTask_CanRx(void)
{
    CanFrame_t   frame;
    AdasInput_t  sensor;
    AdasOutput_t alert;
    SysStatus_t  status;
    uint8_t      seq;
    uint8_t      first_sensor = 1U;
    uint8_t      first_alert  = 1U;
    uint8_t      first_status = 1U;

    for (;;)
    {
        if (osMessageQueueGet(s_rxQueue, &frame, NULL, APP_TASK_POLL_MS) == osOK)
        {
            switch (frame.id)
            {
            case CAN_ID_SENSOR_DATA:
                if (adas_can_unpack_sensor(&frame, &sensor, &seq) == 0)
                {
                    s_stats.rx_sensor++;
                    if (first_sensor != 0U)
                    {
                        first_sensor = 0U;
                        AppLog_Printf("CAN RX OK 0x100 SENSOR seq=%u dist=%u spd=%u",
                                      seq, sensor.obstacle_distance_m, sensor.speed_kmh);
                    }
                }
                else
                {
                    s_stats.rx_decode_err++;
                }
                break;

            case CAN_ID_DECISION_ALERT:
                if (adas_can_unpack_alert(&frame, &alert, &seq) == 0)
                {
                    s_stats.rx_alert++;
                    if (first_alert != 0U)
                    {
                        first_alert = 0U;
                        AppLog_Printf("CAN RX OK 0x200 ALERT seq=%u risk=%s",
                                      seq, adas_risk_name(alert.risk));
                    }
                }
                else
                {
                    s_stats.rx_decode_err++;
                }
                break;

            case CAN_ID_SYSTEM_STATUS:
                if (adas_can_unpack_status(&frame, &status) == 0)
                {
                    s_stats.rx_status++;
                    if (first_status != 0U)
                    {
                        first_status = 0U;
                        AppLog_Printf("CAN RX OK 0x300 STATUS state=%u flags=0x%02X",
                                      status.state, status.error_flags);
                    }
                }
                else
                {
                    s_stats.rx_decode_err++;
                }
                break;

            default:
                /* Other nodes' traffic on a real bus: ignored. */
                break;
            }
        }
        App_Heartbeat(APP_TASK_CAN_RX);
    }
}
