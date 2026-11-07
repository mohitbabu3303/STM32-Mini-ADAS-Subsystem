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
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
    SAFE,
    LOW_RISK,
    MEDIUM_RISK,
    HIGH_RISK,
    CRITICAL_RISK
} RiskLevel_t;

typedef struct
{
    int lane_status;
    int obstacle_distance;
    int speed;
} SensorData_t;

typedef struct
{
    int lane_warning;
    int obstacle_warning;
    RiskLevel_t risk_level;
} AlertData_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LANE_ALERT_FLAG     (1U << 0)
#define OBSTACLE_ALERT_FLAG (1U << 1)

/* STM32F407G-DISC1 user LEDs (port D) */
#define LED_GREEN_PIN   GPIO_PIN_12   /* heartbeat              */
#define LED_ORANGE_PIN  GPIO_PIN_13   /* lane departure warning */
#define LED_RED_PIN     GPIO_PIN_14   /* obstacle warning       */
#define LED_BLUE_PIN    GPIO_PIN_15   /* HIGH/CRITICAL risk     */
#define LED_PORT        GPIOD

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* USER CODE BEGIN PV */
osMessageQueueId_t sensorQueue;
osMessageQueueId_t filterQueue;
osMessageQueueId_t alertQueue;
osEventFlagsId_t   alertFlags;
osMutexId_t        printfMutex;   /* serialises printf output from all tasks */

/* Stacks raised to 256 words: printf with newlib needs more than 128 words */
const osThreadAttr_t SensorTask_attributes   = { .name = "SensorTask",   .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityAboveNormal };
const osThreadAttr_t FilterTask_attributes   = { .name = "FilterTask",   .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityNormal };
const osThreadAttr_t DecisionTask_attributes = { .name = "DecisionTask", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityHigh };
const osThreadAttr_t AlertTask_attributes    = { .name = "AlertTask",    .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityLow };
const osThreadAttr_t LoggerTask_attributes   = { .name = "LoggerTask",   .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityLow };

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
void StartDefaultTask(void *argument);

/* USER CODE BEGIN PFP */
void SensorTask(void *argument);
void FilterTask(void *argument);
void DecisionTask(void *argument);
void AlertTask(void *argument);
void LoggerTask(void *argument);

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
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  printfMutex = osMutexNew(NULL);
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  sensorQueue = osMessageQueueNew(5, sizeof(SensorData_t), NULL);
  filterQueue = osMessageQueueNew(5, sizeof(SensorData_t), NULL);
  alertQueue  = osMessageQueueNew(5, sizeof(AlertData_t),  NULL);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  osThreadNew(SensorTask,   NULL, &SensorTask_attributes);
  osThreadNew(FilterTask,   NULL, &FilterTask_attributes);
  osThreadNew(DecisionTask, NULL, &DecisionTask_attributes);
  osThreadNew(AlertTask,    NULL, &AlertTask_attributes);
  osThreadNew(LoggerTask,   NULL, &LoggerTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  alertFlags = osEventFlagsNew(NULL);

  /* If any RTOS object failed to allocate, stop here with all LEDs on */
  if (!sensorQueue || !filterQueue || !alertQueue || !alertFlags)
  {
    HAL_GPIO_WritePin(LED_PORT, LED_GREEN_PIN | LED_ORANGE_PIN | LED_RED_PIN | LED_BLUE_PIN, GPIO_PIN_SET);
    Error_Handler();
  }
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

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
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* Discovery board LEDs PD12..PD15 as push-pull outputs */
  GPIO_InitTypeDef led_gpio = {0};
  __HAL_RCC_GPIOD_CLK_ENABLE();
  HAL_GPIO_WritePin(LED_PORT, LED_GREEN_PIN | LED_ORANGE_PIN | LED_RED_PIN | LED_BLUE_PIN, GPIO_PIN_RESET);
  led_gpio.Pin   = LED_GREEN_PIN | LED_ORANGE_PIN | LED_RED_PIN | LED_BLUE_PIN;
  led_gpio.Mode  = GPIO_MODE_OUTPUT_PP;
  led_gpio.Pull  = GPIO_NOPULL;
  led_gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_PORT, &led_gpio);

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* printf -> USART1 (PA9 TX, 115200 8N1) and ITM/SWV port 0 */
int _write(int file, char *ptr, int len)
{
    int locked = (printfMutex != NULL) && (osKernelGetState() == osKernelRunning);
    if (locked) osMutexAcquire(printfMutex, osWaitForever);
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    for (int i = 0; i < len; i++)
    {
        ITM_SendChar((uint32_t)ptr[i]);
    }
    if (locked) osMutexRelease(printfMutex);
    return len;
}

/* Simulated sensors (was myTask03 / "autofilled") */
void SensorTask(void *argument)
{
    SensorData_t data;
    for (;;)
    {
        data.lane_status       = rand() % 2;
        data.obstacle_distance = rand() % 100;
        data.speed             = 30 + rand() % 100;
        osMessageQueuePut(sensorQueue, &data, 0, 0);

        printf("LANE=%d, DIST=%d, SPEED=%d\r\n",
               data.lane_status, data.obstacle_distance, data.speed);

        osDelay(1000);
    }
}

void FilterTask(void *argument)
{
    SensorData_t data;
    for (;;)
    {
        if (osMessageQueueGet(sensorQueue, &data, NULL, osWaitForever) == osOK)
        {
            /* Simple noise filter */
            data.obstacle_distance = data.obstacle_distance + (rand() % 5 - 2);
            osMessageQueuePut(filterQueue, &data, 0, 0);
        }
    }
}

/* Was StartTask04 */
void DecisionTask(void *argument)
{
    SensorData_t data;
    AlertData_t  alert;
    for (;;)
    {
        if (osMessageQueueGet(filterQueue, &data, NULL, osWaitForever) == osOK)
        {
            alert.lane_warning     = (data.lane_status == 1);
            alert.obstacle_warning = (data.obstacle_distance < 20);
            alert.risk_level       = SAFE;

            if (data.speed > 120 || data.obstacle_distance < 5)        alert.risk_level = CRITICAL_RISK;
            else if (data.speed > 100 || data.obstacle_distance < 10)  alert.risk_level = HIGH_RISK;
            else if (data.speed > 80  || data.obstacle_distance < 20)  alert.risk_level = MEDIUM_RISK;
            else if (data.speed > 50  || data.obstacle_distance < 40)  alert.risk_level = LOW_RISK;

            /* FIX: the original never sent the alert, so AlertTask read garbage */
            osMessageQueuePut(alertQueue, &alert, 0, 0);

            uint32_t f = 0;
            if (alert.lane_warning)     f |= LANE_ALERT_FLAG;
            if (alert.obstacle_warning) f |= OBSTACLE_ALERT_FLAG;
            if (f) osEventFlagsSet(alertFlags, f);
        }
    }
}

/* Was StartTask05 - now also drives the Discovery LEDs */
void AlertTask(void *argument)
{
    for (;;)
    {
        AlertData_t alert = {0};

        uint32_t flags = osEventFlagsWait(alertFlags,
                                          LANE_ALERT_FLAG | OBSTACLE_ALERT_FLAG,
                                          osFlagsWaitAny, 1500);

        /* FIX: drain the queue and keep only the newest alert. Without this,
           alerts from cycles with no warning flag pile up and later get
           reported against the wrong sensor reading. */
        AlertData_t tmp;
        int have_alert = 0;
        while (osMessageQueueGet(alertQueue, &tmp, NULL, 0) == osOK)
        {
            alert = tmp;
            have_alert = 1;
        }

        if (flags & osFlagsError)   /* timeout: no warnings -> warning LEDs off */
        {
            HAL_GPIO_WritePin(LED_PORT, LED_ORANGE_PIN | LED_RED_PIN | LED_BLUE_PIN, GPIO_PIN_RESET);
            continue;
        }

        if (!have_alert)
        {
            alert.risk_level = SAFE;
        }

        HAL_GPIO_WritePin(LED_PORT, LED_ORANGE_PIN, (flags & LANE_ALERT_FLAG)     ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_PORT, LED_RED_PIN,    (flags & OBSTACLE_ALERT_FLAG) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_PORT, LED_BLUE_PIN,   (alert.risk_level >= HIGH_RISK) ? GPIO_PIN_SET : GPIO_PIN_RESET);

        if (flags & LANE_ALERT_FLAG)     printf("ALERT: Lane Deviation!\r\n");
        if (flags & OBSTACLE_ALERT_FLAG) printf("ALERT: Obstacle Too Close!\r\n");

        switch (alert.risk_level)
        {
            case SAFE:          printf("RISK LEVEL: SAFE\r\n"); break;
            case LOW_RISK:      printf("RISK LEVEL: LOW\r\nACTION: Stay Alert\r\n"); break;
            case MEDIUM_RISK:   printf("RISK LEVEL: MEDIUM\r\nACTION: Reduce Speed\r\n"); break;
            case HIGH_RISK:     printf("RISK LEVEL: HIGH\r\nACTION: Slow Down Immediately\r\n"); break;
            case CRITICAL_RISK: printf("RISK LEVEL: CRITICAL\r\nACTION: APPLY BRAKES!\r\n"); break;
        }
        printf("--------------------------\r\n");
    }
}

void LoggerTask(void *argument)
{
    for (;;)
    {
        printf("SYSTEM STATUS: Running... free heap=%u B\r\n", (unsigned)xPortGetFreeHeapSize());
        osDelay(2000);
    }
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Heartbeat: green LED toggles while the scheduler is alive */
  for(;;)
  {
    HAL_GPIO_TogglePin(LED_PORT, LED_GREEN_PIN);
    osDelay(500);
  }
  /* USER CODE END 5 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
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
  while (1)
  {
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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
