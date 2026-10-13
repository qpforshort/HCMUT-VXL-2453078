/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Lab 2 - Exercise 1
  *
  * D1: PA5, active LOW, always ON
  * EN0: PA6, EN1: PA7, active LOW through PNP
  * Segments a-g: PB0-PB6, common anode
  * Left display: 1, right display: 2
  * Switching interval: 500 ms
  * TIM2 interrupt period: 10 ms
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define STATUS_LED_PIN       GPIO_PIN_5

#define EN0_PIN              GPIO_PIN_6
#define EN1_PIN              GPIO_PIN_7

#define ENABLE_PINS          (EN0_PIN | EN1_PIN)

#define SEGMENT_PINS         (GPIO_PIN_0 | GPIO_PIN_1 | \
                              GPIO_PIN_2 | GPIO_PIN_3 | \
                              GPIO_PIN_4 | GPIO_PIN_5 | \
                              GPIO_PIN_6)

#define TIMER_CYCLE_MS       10U
#define SWITCH_PERIOD_MS     500U
#define SWITCH_TICKS         (SWITCH_PERIOD_MS / TIMER_CYCLE_MS)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */

/* Bit 0 = a, bit 1 = b, ..., bit 6 = g */
static const uint8_t digitPattern[10] =
{
  0x3F, /* 0 */
  0x06, /* 1 */
  0x5B, /* 2 */
  0x4F, /* 3 */
  0x66, /* 4 */
  0x6D, /* 5 */
  0x7D, /* 6 */
  0x07, /* 7 */
  0x7F, /* 8 */
  0x6F  /* 9 */
};

static uint16_t scanCounter = SWITCH_TICKS;

/* Left digit is shown first; right digit is next */
static uint8_t nextDigit = 1U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);

/* USER CODE BEGIN PFP */

void display7SEG(int num);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void display7SEG(int num)
{
  /* Common anode: HIGH turns all segments OFF */
  HAL_GPIO_WritePin(GPIOB,
                   SEGMENT_PINS,
                   GPIO_PIN_SET);

  if (num < 0 || num > 9)
  {
    return;
  }

  /* Pull the required segment cathodes LOW */
  HAL_GPIO_WritePin(GPIOB,
                   (uint16_t)digitPattern[num],
                   GPIO_PIN_RESET);
}

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

  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* Release PB3/PB4 from JTAG; retain SWD */
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_NOJTAG();

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();

  /* USER CODE BEGIN 2 */

  /* Keep D1 ON: PA5 is active LOW */
  HAL_GPIO_WritePin(GPIOA,
                   STATUS_LED_PIN,
                   GPIO_PIN_RESET);

  /* Disable both digits before setting the initial pattern */
  HAL_GPIO_WritePin(GPIOA,
                   ENABLE_PINS,
                   GPIO_PIN_SET);

  /* Initially display 1 on the left digit */
  display7SEG(1);

  HAL_GPIO_WritePin(GPIOA,
                   EN0_PIN,
                   GPIO_PIN_RESET);

  nextDigit = 1U;
  scanCounter = SWITCH_TICKS;

  /* Start TIM2 without a pending update event */
  __HAL_TIM_SET_COUNTER(&htim2, 0U);
  __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);

  if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    /* Exercise 1: display scanning is handled by TIM2 interrupt. */
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

  /** Initializes the RCC Oscillators
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue =
      RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType =
      RCC_CLOCKTYPE_HCLK |
      RCC_CLOCKTYPE_SYSCLK |
      RCC_CLOCKTYPE_PCLK1 |
      RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct,
                         FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{
  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */

  /*
   * Timer clock: 8 MHz
   * 8 MHz / (7999 + 1) / (9 + 1) = 100 Hz
   * Interrupt period = 10 ms
   */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 9;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;

  if (HAL_TIM_ConfigClockSource(&htim2,
                               &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

  if (HAL_TIMEx_MasterConfigSynchronization(
          &htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* Digit enables: HIGH = OFF */
  HAL_GPIO_WritePin(GPIOA,
                   ENABLE_PINS,
                   GPIO_PIN_SET);

  /* D1: LOW = ON */
  HAL_GPIO_WritePin(GPIOA,
                   STATUS_LED_PIN,
                   GPIO_PIN_RESET);

  /* Segment cathodes: HIGH = OFF */
  HAL_GPIO_WritePin(GPIOB,
                   SEGMENT_PINS,
                   GPIO_PIN_SET);

  /* PA5: D1; PA6/PA7: digit enables */
  GPIO_InitStruct.Pin = STATUS_LED_PIN | ENABLE_PINS;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* PB0-PB6: segment a-g outputs */
  GPIO_InitStruct.Pin = SEGMENT_PINS;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance != TIM2)
  {
    return;
  }

  /* One callback every 10 ms */
  if (scanCounter > 0U)
  {
    scanCounter--;
  }

  /* Switch digit after 50 interrupts = 500 ms */
  if (scanCounter == 0U)
  {
    scanCounter = SWITCH_TICKS;

    /* Blank both digits before changing segment data */
    HAL_GPIO_WritePin(GPIOA,
                     ENABLE_PINS,
                     GPIO_PIN_SET);

    if (nextDigit == 0U)
    {
      /* Left digit displays 1 */
      display7SEG(1);

      HAL_GPIO_WritePin(GPIOA,
                       EN0_PIN,
                       GPIO_PIN_RESET);

      nextDigit = 1U;
    }
    else
    {
      /* Right digit displays 2 */
      display7SEG(2);

      HAL_GPIO_WritePin(GPIOA,
                       EN1_PIN,
                       GPIO_PIN_RESET);

      nextDigit = 0U;
    }
  }

  /* D1 is not toggled, so it stays ON */
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */

  __disable_irq();

  while (1)
  {
  }

  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the source file and line of an assert error.
  * @param  file: pointer to the source file name
  * @param  line: assert error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */

  (void)file;
  (void)line;

  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
