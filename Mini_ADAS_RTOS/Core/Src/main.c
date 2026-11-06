/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
  *
  * Mini ADAS: all application logic lives in app_*.c / adas_*.c. This file
  * only contains CubeMX-generated init code plus calls placed inside
  * USER CODE sections, so it survives "Generate Code" in CubeMX.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan;

UART_HandleTypeDef huart1;

/* Definitions for SensorTask */
osThreadId_t SensorTaskHandle;
const osThreadAttr_t SensorTask_attributes = {
  .name = "SensorTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for DecisionTask */
osThreadId_t DecisionTaskHandle;
const osThreadAttr_t DecisionTask_attributes = {
  .name = "DecisionTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for AlertTask */
osThreadId_t AlertTaskHandle;
const osThreadAttr_t AlertTask_attributes = {
  .name = "AlertTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for CAN_TxTask */
osThreadId_t CAN_TxTaskHandle;
const osThreadAttr_t CAN_TxTask_attributes = {
  .name = "CAN_TxTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for CAN_RxTask */
osThreadId_t CAN_RxTaskHandle;
const osThreadAttr_t CAN_RxTask_attributes = {
  .name = "CAN_RxTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for LoggerTask */
osThreadId_t LoggerTaskHandle;
const osThreadAttr_t LoggerTask_attributes = {
  .name = "LoggerTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for HealthMonitorTask */
osThreadId_t HealthMonitorTaskHandle;
const osThreadAttr_t HealthMonitorTask_attributes = {
  .name = "HealthMonitorTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_CAN_Init(void);
void StartSensorTask(void *argument);
void StartDecisionTask(void *argument);
void StartAlertTask(void *argument);
void StartCAN_TxTask(void *argument);
void StartCAN_RxTask(void *argument);
void StartLoggerTask(void *argument);
void StartHealthMonitorTask(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_CAN_Init();
  /* USER CODE BEGIN 2 */
  /* IMPORTANT: everything that relies on HAL_GetTick() must run here, BEFORE
   * the first FreeRTOS object is created. Creating a queue/thread masks
   * interrupts (incl. SysTick) until osKernelStart(). */
  App_PrintBootBanner();
  if (AppCan_Start() != HAL_OK)
  {
    AppLog_Printf("FATAL: CAN start failed");
    Error_Handler();
  }
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  App_CreateRtosObjects();
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of SensorTask */
  SensorTaskHandle = osThreadNew(StartSensorTask, NULL, &SensorTask_attributes);

  /* creation of DecisionTask */
  DecisionTaskHandle = osThreadNew(StartDecisionTask, NULL, &DecisionTask_attributes);

  /* creation of AlertTask */
  AlertTaskHandle = osThreadNew(StartAlertTask, NULL, &AlertTask_attributes);

  /* creation of CAN_TxTask */
  CAN_TxTaskHandle = osThreadNew(StartCAN_TxTask, NULL, &CAN_TxTask_attributes);

  /* creation of CAN_RxTask */
  CAN_RxTaskHandle = osThreadNew(StartCAN_RxTask, NULL, &CAN_RxTask_attributes);

  /* creation of LoggerTask */
  LoggerTaskHandle = osThreadNew(StartLoggerTask, NULL, &LoggerTask_attributes);

  /* creation of HealthMonitorTask */
  HealthMonitorTaskHandle = osThreadNew(StartHealthMonitorTask, NULL, &HealthMonitorTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* A NULL handle means the FreeRTOS heap is too small: stop here instead of
   * running with missing tasks (this silently happened before). */
  if ((SensorTaskHandle == NULL) || (DecisionTaskHandle == NULL) ||
      (AlertTaskHandle == NULL) || (CAN_TxTaskHandle == NULL) ||
      (CAN_RxTaskHandle == NULL) || (LoggerTaskHandle == NULL) ||
      (HealthMonitorTaskHandle == NULL))
  {
    Error_Handler();
  }
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN_Init(void)
{

  /* USER CODE BEGIN CAN_Init 0 */
  /* Bit timing @ APB1 = 8 MHz:
   *   tq = Prescaler / 8 MHz = 125 ns
   *   bit = 1 (sync) + 13 (BS1) + 2 (BS2) = 16 tq = 2 us  -> 500 kbit/s
   *   sample point = (1 + 13) / 16 = 87.5 % (CiA recommendation)
   * Silent loopback: self-test without transceiver. For a real bus set
   * Mode = Normal in CubeMX and fit a CAN transceiver on PA11/PA12. */
  /* USER CODE END CAN_Init 0 */

  /* USER CODE BEGIN CAN_Init 1 */

  /* USER CODE END CAN_Init 1 */
  hcan.Instance = CAN1;
  hcan.Init.Prescaler = 1;
  hcan.Init.Mode = CAN_MODE_SILENT_LOOPBACK;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_13TQ;
  hcan.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = ENABLE;
  hcan.Init.AutoWakeUp = DISABLE;
  hcan.Init.AutoRetransmission = ENABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN_Init 2 */

  /* USER CODE END CAN_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ALERT_LED_GPIO_Port, ALERT_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : STATUS_LED_Pin */
  GPIO_InitStruct.Pin = STATUS_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(STATUS_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ALERT_LED_Pin */
  GPIO_InitStruct.Pin = ALERT_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ALERT_LED_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartSensorTask */
/**
  * @brief  Function implementing the SensorTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartSensorTask */
void StartSensorTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  (void)argument;
  AppTask_Sensor();
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartDecisionTask */
/**
* @brief Function implementing the DecisionTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDecisionTask */
void StartDecisionTask(void *argument)
{
  /* USER CODE BEGIN StartDecisionTask */
  (void)argument;
  AppTask_Decision();
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDecisionTask */
}

/* USER CODE BEGIN Header_StartAlertTask */
/**
* @brief Function implementing the AlertTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartAlertTask */
void StartAlertTask(void *argument)
{
  /* USER CODE BEGIN StartAlertTask */
  (void)argument;
  AppTask_Alert();
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartAlertTask */
}

/* USER CODE BEGIN Header_StartCAN_TxTask */
/**
* @brief Function implementing the CAN_TxTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCAN_TxTask */
void StartCAN_TxTask(void *argument)
{
  /* USER CODE BEGIN StartCAN_TxTask */
  (void)argument;
  AppTask_CanTx();
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartCAN_TxTask */
}

/* USER CODE BEGIN Header_StartCAN_RxTask */
/**
* @brief Function implementing the CAN_RxTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCAN_RxTask */
void StartCAN_RxTask(void *argument)
{
  /* USER CODE BEGIN StartCAN_RxTask */
  (void)argument;
  AppTask_CanRx();
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartCAN_RxTask */
}

/* USER CODE BEGIN Header_StartLoggerTask */
/**
* @brief Function implementing the LoggerTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLoggerTask */
void StartLoggerTask(void *argument)
{
  /* USER CODE BEGIN StartLoggerTask */
  (void)argument;
  AppTask_Logger();
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartLoggerTask */
}

/* USER CODE BEGIN Header_StartHealthMonitorTask */
/**
* @brief Function implementing the HealthMonitorTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartHealthMonitorTask */
void StartHealthMonitorTask(void *argument)
{
  /* USER CODE BEGIN StartHealthMonitorTask */
  (void)argument;
  AppTask_HealthMonitor();
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartHealthMonitorTask */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  APP_DEBUG_BREAK();   /* debugger attached: halts here, check the call stack */

  /* No debugger: fast-blink the on-board LED (PC13) forever. */
  while (1)
  {
    volatile uint32_t delay;
    HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
    for (delay = 0U; delay < 100000U; delay++)
    {
    }
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  (void)file;
  (void)line;
  APP_DEBUG_BREAK();
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
