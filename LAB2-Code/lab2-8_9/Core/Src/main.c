/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Lab 2 - Exercises 9 and 10
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

enum
{
  TIMER_CLOCK = 0,
  TIMER_7SEG,
  TIMER_MATRIX,
  TIMER_ANIMATION,
  TIMER_COUNT
};

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* 9: static A; 10: circular left scrolling */
#define LAB_TASK              10

#if LAB_TASK != 9 && LAB_TASK != 10
#error "LAB_TASK must be 9 or 10"
#endif

/* TIM2 interrupt period */
#define TIMER_CYCLE_MS        10U

#define CLOCK_PERIOD_MS       1000U
#define SEG_SCAN_PERIOD_MS    250U
#define MATRIX_SCAN_MS        10U
#define ANIMATION_PERIOD_MS   400U

#define MAX_LED               4
#define MAX_LED_MATRIX        8

/* DOT = PA4; separate LED = PA5 */
#define DOT_PIN               GPIO_PIN_4
#define STATUS_LED_PIN        GPIO_PIN_5

/* Seven-segment enables: PA6-PA9 */
#define SEG_ENABLE_PINS       (GPIO_PIN_6 | GPIO_PIN_7 | \
                               GPIO_PIN_8 | GPIO_PIN_9)

/* Seven-segment a-g: PB0-PB6 */
#define SEGMENT_PINS          (GPIO_PIN_0 | GPIO_PIN_1 | \
                               GPIO_PIN_2 | GPIO_PIN_3 | \
                               GPIO_PIN_4 | GPIO_PIN_5 | \
                               GPIO_PIN_6)

/*
 * Matrix controls match the reference schematic:
 * ENM0 = PA2
 * ENM1 = PA3
 * ENM2 = PA10
 * ENM3 = PA11
 * ENM4 = PA12
 * ENM5 = PA13
 * ENM6 = PA14
 * ENM7 = PA15
 */
#define MATRIX_ENABLE_PINS    (GPIO_PIN_2  | GPIO_PIN_3  | \
                               GPIO_PIN_10 | GPIO_PIN_11 | \
                               GPIO_PIN_12 | GPIO_PIN_13 | \
                               GPIO_PIN_14 | GPIO_PIN_15)

/* ROW0-ROW7: PB8-PB15 */
#define MATRIX_ROW_PINS       (GPIO_PIN_8  | GPIO_PIN_9  | \
                               GPIO_PIN_10 | GPIO_PIN_11 | \
                               GPIO_PIN_12 | GPIO_PIN_13 | \
                               GPIO_PIN_14 | GPIO_PIN_15)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */

/* Shared between main and TIM2 interrupt */
static volatile uint32_t timer_counter[TIMER_COUNT] = {0};
static volatile uint8_t timer_flag[TIMER_COUNT] = {0};

/* Bit 0 = a, ..., bit 6 = g */
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

static const uint16_t enablePin[MAX_LED] =
{
  GPIO_PIN_6,
  GPIO_PIN_7,
  GPIO_PIN_8,
  GPIO_PIN_9
};

int led_buffer[MAX_LED] = {1, 5, 0, 8};
int index_led = 0;

int hour = 15;
int minute = 8;
int second = 50;

/* ENM0-ENM7 in schematic order */
static const uint16_t matrixEnablePin[MAX_LED_MATRIX] =
{
  GPIO_PIN_2,
  GPIO_PIN_3,
  GPIO_PIN_10,
  GPIO_PIN_11,
  GPIO_PIN_12,
  GPIO_PIN_13,
  GPIO_PIN_14,
  GPIO_PIN_15
};

/*
 * Letter A stored by COLUMN.
 * Bit 0 = top row (ROW0).
 * COL0 = leftmost column.
 *
 * ...##...
 * ..#..#..
 * .#....#.
 * .#....#.
 * .######.
 * .#....#.
 * .#....#.
 * ........
 *
 * A set bit means the pixel should light.
 */
static const uint8_t letterA[MAX_LED_MATRIX] =
{
  0x00, 0x7C, 0x12, 0x11,
  0x11, 0x12, 0x7C, 0x00
};

uint8_t matrix_buffer[MAX_LED_MATRIX] = {0};
int index_led_matrix = 0;

#if LAB_TASK == 10
static uint8_t animation_offset = 0U;
static uint8_t animation_pending = 0U;
#endif

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);

/* USER CODE BEGIN PFP */

static void setSoftwareTimer(uint8_t id, uint32_t duration_ms);
void timer_run(void);

void display7SEG(int num);
void update7SEG(int index);
void updateClockBuffer(void);
static void advanceClockOneSecond(void);

void updateLEDMatrix(int index);
static void loadMatrixFrame(uint8_t offset);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* ---------------- Software timers ---------------- */

static void setSoftwareTimer(uint8_t id, uint32_t duration_ms)
{
  if (id >= TIMER_COUNT)
  {
    return;
  }

  uint32_t savedPrimask = __get_PRIMASK();
  __disable_irq();

  timer_counter[id] = duration_ms / TIMER_CYCLE_MS;

  /* Minimum timeout: one hardware timer tick */
  if (timer_counter[id] == 0U)
  {
    timer_counter[id] = 1U;
  }

  timer_flag[id] = 0U;

  __set_PRIMASK(savedPrimask);
}

void timer_run(void)
{
  for (uint8_t i = 0U; i < TIMER_COUNT; i++)
  {
    if (timer_counter[i] > 0U)
    {
      timer_counter[i]--;

      if (timer_counter[i] == 0U)
      {
        timer_flag[i] = 1U;
      }
    }
  }
}

/* ---------------- Seven-segment display ---------------- */

void display7SEG(int num)
{
  /* Common anode: HIGH turns segments OFF */
  HAL_GPIO_WritePin(GPIOB,
                   SEGMENT_PINS,
                   GPIO_PIN_SET);

  if (num < 0 || num > 9)
  {
    return;
  }

  /* LOW turns selected segments ON */
  HAL_GPIO_WritePin(GPIOB,
                   (uint16_t)digitPattern[num],
                   GPIO_PIN_RESET);
}

void update7SEG(int index)
{
  /* Disable all digits before changing data */
  HAL_GPIO_WritePin(GPIOA,
                   SEG_ENABLE_PINS,
                   GPIO_PIN_SET);

  if (index < 0 || index >= MAX_LED)
  {
    return;
  }

  display7SEG(led_buffer[index]);

  /* PNP control: LOW enables the selected digit */
  HAL_GPIO_WritePin(GPIOA,
                   enablePin[index],
                   GPIO_PIN_RESET);
}

void updateClockBuffer(void)
{
  /* Display processing runs entirely in main */
  led_buffer[0] = hour / 10;
  led_buffer[1] = hour % 10;
  led_buffer[2] = minute / 10;
  led_buffer[3] = minute % 10;
}

static void advanceClockOneSecond(void)
{
  second++;

  if (second >= 60)
  {
    second = 0;
    minute++;
  }

  if (minute >= 60)
  {
    minute = 0;
    hour++;
  }

  if (hour >= 24)
  {
    hour = 0;
  }

  updateClockBuffer();
}

/* ---------------- LED matrix ---------------- */

static void loadMatrixFrame(uint8_t offset)
{
  /*
   * Circular left shift:
   * columns leaving the left edge reappear on the right.
   *
   * offset 0: original centered A
   * offset 1: rotate left by one column
   * ...
   * offset 7: rotate left by seven columns
   */
  for (int column = 0; column < MAX_LED_MATRIX; column++)
  {
    int source = (column + offset) % MAX_LED_MATRIX;
    matrix_buffer[column] = letterA[source];
  }
}

void updateLEDMatrix(int index)
{
  /*
   * Matrix polarity:
   * Column HIGH + Row LOW = LED ON.
   *
   * ULN2803 column control:
   * MCU HIGH -> output LOW -> column disabled.
   * MCU LOW  -> output released -> external pull-up
   *             raises the column -> column enabled.
   */

  /* Disable every column before changing row data */
  HAL_GPIO_WritePin(GPIOA,
                   MATRIX_ENABLE_PINS,
                   GPIO_PIN_SET);

  /* All rows inactive */
  HAL_GPIO_WritePin(GPIOB,
                   MATRIX_ROW_PINS,
                   GPIO_PIN_SET);

  if (index < 0 || index >= MAX_LED_MATRIX)
  {
    return;
  }

  /* Map buffer bits 0-7 to PB8-PB15 */
  uint16_t rowData =
      (uint16_t)((uint16_t)matrix_buffer[index] << 8);

  /* Pull rows for illuminated pixels LOW */
  if (rowData != 0U)
  {
    HAL_GPIO_WritePin(GPIOB,
                     rowData,
                     GPIO_PIN_RESET);
  }

  /* Enable only this column */
  HAL_GPIO_WritePin(GPIOA,
                   matrixEnablePin[index],
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

  /*
   * Release PA13, PA14, PA15, PB3 and PB4.
   * The reference schematic uses PA13/PA14 for the matrix,
   * so both JTAG and SWD must be disabled.
   */
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_DISABLE();

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();

  /* USER CODE BEGIN 2 */

  /* Initially disable seven-segment digits and DOT */
  HAL_GPIO_WritePin(GPIOA,
                   SEG_ENABLE_PINS | DOT_PIN,
                   GPIO_PIN_SET);

  /* Turn D1 ON to indicate that initialization is complete */
  HAL_GPIO_WritePin(GPIOA,
                   STATUS_LED_PIN,
                   GPIO_PIN_RESET);

  HAL_GPIO_WritePin(GPIOB,
                   SEGMENT_PINS,
                   GPIO_PIN_SET);

  /* Initially disable matrix columns and rows */
  HAL_GPIO_WritePin(GPIOA,
                   MATRIX_ENABLE_PINS,
                   GPIO_PIN_SET);

  HAL_GPIO_WritePin(GPIOB,
                   MATRIX_ROW_PINS,
                   GPIO_PIN_SET);

  updateClockBuffer();

  /* Start with the complete A centered */
  loadMatrixFrame(0U);

  update7SEG(0);
  index_led = 1;

  updateLEDMatrix(0);
  index_led_matrix = 1;

  setSoftwareTimer(TIMER_CLOCK, CLOCK_PERIOD_MS);
  setSoftwareTimer(TIMER_7SEG, SEG_SCAN_PERIOD_MS);
  setSoftwareTimer(TIMER_MATRIX, MATRIX_SCAN_MS);

#if LAB_TASK == 10
  setSoftwareTimer(TIMER_ANIMATION, ANIMATION_PERIOD_MS);
#endif

  /* Clear any pending update event before starting TIM2 */
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

    /* Clock and DOT event */
    if (timer_flag[TIMER_CLOCK] == 1U)
    {
      setSoftwareTimer(TIMER_CLOCK, CLOCK_PERIOD_MS);

      advanceClockOneSecond();
      HAL_GPIO_TogglePin(GPIOA, DOT_PIN);
    }

    /* Seven-segment scanning event */
    if (timer_flag[TIMER_7SEG] == 1U)
    {
      setSoftwareTimer(TIMER_7SEG, SEG_SCAN_PERIOD_MS);

      update7SEG(index_led);
      index_led = (index_led + 1) % MAX_LED;
    }

#if LAB_TASK == 10
    /* Request the next circular scrolling frame */
    if (timer_flag[TIMER_ANIMATION] == 1U)
    {
      setSoftwareTimer(TIMER_ANIMATION, ANIMATION_PERIOD_MS);
      animation_pending = 1U;
    }
#endif

    /* Matrix scanning event */
    if (timer_flag[TIMER_MATRIX] == 1U)
    {
      setSoftwareTimer(TIMER_MATRIX, MATRIX_SCAN_MS);

#if LAB_TASK == 10
      /*
       * Apply a new frame before scanning column 0.
       * All eight columns in a scan then use the same frame.
       */
      if (index_led_matrix == 0 && animation_pending == 1U)
      {
        animation_pending = 0U;

        animation_offset =
            (animation_offset + 1U) % MAX_LED_MATRIX;

        loadMatrixFrame(animation_offset);
      }
#endif

      updateLEDMatrix(index_led_matrix);

      index_led_matrix =
          (index_led_matrix + 1) % MAX_LED_MATRIX;
    }
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

  /* 8 MHz / (7999 + 1) / (9 + 1) = 100 Hz */
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

  /*
   * Initial OFF levels:
   * Seven-segment enables, DOT and LED: HIGH.
   * Matrix controls: HIGH -> ULN2803 pulls columns LOW.
   * Seven-segment cathodes and matrix rows: HIGH.
   */
  HAL_GPIO_WritePin(GPIOA,
                   SEG_ENABLE_PINS |
                   DOT_PIN |
                   STATUS_LED_PIN |
                   MATRIX_ENABLE_PINS,
                   GPIO_PIN_SET);

  HAL_GPIO_WritePin(GPIOB,
                   SEGMENT_PINS | MATRIX_ROW_PINS,
                   GPIO_PIN_SET);

  /*
   * GPIOA:
   * PA2, PA3, PA10-PA15: matrix controls.
   * PA4: DOT.
   * PA5: separate LED.
   * PA6-PA9: seven-segment enables.
   */
  GPIO_InitStruct.Pin =
      SEG_ENABLE_PINS |
      DOT_PIN |
      STATUS_LED_PIN |
      MATRIX_ENABLE_PINS;

  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*
   * GPIOB:
   * PB0-PB6: seven-segment data.
   * PB8-PB15: matrix rows.
   */
  GPIO_InitStruct.Pin = SEGMENT_PINS | MATRIX_ROW_PINS;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    /* The interrupt only maintains software timers */
    timer_run();
  }
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
