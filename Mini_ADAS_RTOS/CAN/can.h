#ifndef __CAN_H
#define __CAN_H

#include "main.h"

extern CAN_HandleTypeDef hcan1;

void MX_CAN1_Init(void);
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);

#endif /* __CAN_H */
