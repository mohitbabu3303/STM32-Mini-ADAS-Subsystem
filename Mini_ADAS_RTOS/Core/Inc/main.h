/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "can.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/

extern CAN_HandleTypeDef hcan1;

extern osThreadId_t SensorTaskHandle;
extern osThreadId_t DecisionTaskHandle;
extern osThreadId_t CAN_TxTaskHandle;
extern osThreadId_t CAN_RxTaskHandle;
extern osThreadId_t LoggerTaskHandle;
extern osThreadId_t DashboardTaskHandle;
extern osThreadId_t HealthMonitorTaskHandle;

typedef struct
{
    int lane_status;
    int obstacle_distance;
} SensorData_t;

typedef struct
{
    int lane_warning;
    int obstacle_warning;
} AlertData_t;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
void StartSensorTask(void *argument);
void StartDecisionTask(void *argument);
void StartCAN_TxTask(void *argument);
void StartCAN_RxTask(void *argument);
void StartLoggerTask(void *argument);
void StartDashboardTask(void *argument);
void StartHealthMonitorTask(void *argument);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
