/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Lab 2 - Exercises 2 to 8
  *
  * DOT: PA4
  * Separate active-LOW LED: PA5
  * EN0-EN3: PA6-PA9, active LOW through PNP
  * Segments a-g: PB0-PB6, common anode
  * TIM2 interrupt: 10 ms
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

/* Select exercise: 2, 3, 4, 5, 6, 7 or 8 */
#define LAB_TASK 8

#if LAB_TASK < 2 || LAB_TASK > 8
#error "LAB_TASK must be between 2 and 8"
#endif

#define DOT_PIN          GPIO_PIN_4
#define STATUS_LED_PIN   GPIO_PIN_5

#define EN0_PIN          GPIO_PIN_6
#define EN1_PIN          GPIO_PIN_7
#define EN2_PIN          GPIO_PIN_8
#define EN3_PIN          GPIO_PIN_9

#define ENABLE_PINS      (EN0_PIN | EN1_PIN | EN2_PIN | EN3_PIN)

#define SEGMENT_PINS     (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | \
                          GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | \
                          GPIO_PIN_6)

#define MAX_LED          4
#define TIMER_CYCLE      10U

#if LAB_TASK <= 3
#define SCAN_PERIOD_MS   500U
#else
#define SCAN_PERIOD_MS   250U
#endif

/*
 * Exercise 6 test settings:
 *
 * Normal example:
 *   EX6_START_TIMER = 1, EX6_FIRST_MS = 1000U
 *
 * Report 1: omit initial timer:
 *   EX6_START_TIMER = 0
 *
 * Report 2:
 *   EX6_START_TIMER = 1, EX6_FIRST_MS = 1U
 *
 * Report 3:
 *   EX6_START_TIMER = 1, EX6_FIRST_MS = 10U
 */
#define EX6_START_TIMER  1
#define EX6_FIRST_MS     1000U

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

static const uint16_t enablePin[MAX_LED] =
{
  EN0_PIN, EN1_PIN, EN2_PIN, EN3_PIN
};

#if LAB_TASK == 2
volatile int led_buffer[MAX_LED] = {1, 2, 3, 0};
#else
volatile int led_buffer[MAX_LED] = {1, 2, 3, 4};
#endif

int index_led = 0;

int hour = 15;
int minute = 8;
int second = 50;

#if LAB_TASK <= 7
static uint16_t scanCounter = SCAN_PERIOD_MS / TIMER_CYCLE;
#endif

#if LAB_TASK <= 6
static uint16_t dotCounter = 1000U / TIMER_CYCLE;
#endif

volatile uint32_t timer0_counter = 0U;
volatile uint8_t timer0_flag = 0U;

volatile uint32_t timer1_counter = 0U;
volatile uint8_t timer1_flag = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

void display7SEG(int num);
void update7SEG(int index);
void updateClockBuffer(void);

void setTimer0(uint32_t duration);
void setTimer1(uint32_t duration);
void timer_run(void);

static void disableAllDigits(void);
static void advanceClockOneSecond(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* Disable all PNP-controlled displays */
static void disableAllDigits(void)
{
  HAL_GPIO_WritePin(GPIOA, ENABLE_PINS, GPIO_PIN_SET);
}

/* Output one decimal digit to segment pins */
void display7SEG(int num)
{
  HAL_GPIO_WritePin(GPIOB, SEGMENT_PINS, GPIO_PIN_SET);

  if (num < 0 || num > 9)
  {
    return;
  }

  /* Common anode: pull required segments LOW */
  HAL_GPIO_WritePin(GPIOB,
                   (uint16_t)digitPattern[num],
                   GPIO_PIN_RESET);
}

/* Show the buffer value on one selected display */
void update7SEG(int index)
{
  disableAllDigits();

  if (index < 0 || index >= MAX_LED)
  {
    return;
  }

  display7SEG(led_buffer[index]);

  HAL_GPIO_WritePin(GPIOA,
                   enablePin[index],
                   GPIO_PIN_RESET);
}

/* Convert hour and minute to four decimal digits */
void updateClockBuffer(void)
{
  /*
   * Protect the buffer from being read halfway through an update
   * by the scanning interrupt in exercises 5-7.
   */
  uint32_t savedPrimask = __get_PRIMASK();
  __disable_irq();

  led_buffer[0] = hour / 10;
  led_buffer[1] = hour % 10;
  led_buffer[2] = minute / 10;
  led_buffer[3] = minute % 10;

  __set_PRIMASK(savedPrimask);
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

/*
 * Software timer duration is in milliseconds.
 * Integer division is retained to match exercise 6.
 */
void setTimer0(uint32_t duration)
{
  uint32_t savedPrimask = __get_PRIMASK();
  __disable_irq();

  timer0_counter = duration / TIMER_CYCLE;
  timer0_flag = 0U;

  __set_PRIMASK(savedPrimask);
}

void setTimer1(uint32_t duration)
{
  uint32_t savedPrimask = __get_PRIMASK();
  __disable_irq();

  timer1_counter = duration / TIMER_CYCLE;
  timer1_flag = 0U;

  __set_PRIMASK(savedPrimask);
}

/* Run every 10 ms inside the TIM2 callback */
void timer_run(void)
{
  if (timer0_counter > 0U)
  {
    timer0_counter--;

    if (timer0_counter == 0U)
    {
      timer0_flag = 1U;
    }
  }

  if (timer1_counter > 0U)
  {
    timer1_counter--;

    if (timer1_counter == 0U)
    {
      timer1_flag = 1U;
    }
  }
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

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* Release PB3/PB4 from JTAG, retain SWD */
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_NOJTAG();

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

#if LAB_TASK >= 5
  updateClockBuffer();
#endif

  /* Display the first digit immediately */
  update7SEG(0);
  index_led = 1;

  /* DOT starts OFF */
  HAL_GPIO_WritePin(GPIOA, DOT_PIN, GPIO_PIN_SET);

  #if LAB_TASK == 6
    /* Exercise 6: D1 starts OFF for software timer tests */
    HAL_GPIO_WritePin(GPIOA, STATUS_LED_PIN, GPIO_PIN_SET);
  #else
    /* Other exercises: D1 stays ON as an initialization indicator */
    HAL_GPIO_WritePin(GPIOA, STATUS_LED_PIN, GPIO_PIN_RESET);
  #endif

#if LAB_TASK == 6

#if EX6_START_TIMER
  setTimer0(EX6_FIRST_MS);
#endif

#elif LAB_TASK == 7

  setTimer0(1000U);

#elif LAB_TASK == 8

  setTimer0(1000U);
  setTimer1(SCAN_PERIOD_MS);

#endif

  /* Start TIM2 with no pending update event */
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

#if LAB_TASK == 2 || LAB_TASK == 3 || LAB_TASK == 4

    /* Scanning and DOT are handled by the timer interrupt. */

#elif LAB_TASK == 5

    /* Exercise 5: blocking delay in main */
    HAL_Delay(1000);
    advanceClockOneSecond();

#elif LAB_TASK == 6

    /*
     * Software timer demonstration on PA5.
     * Clock functions from exercise 5 are retained.
     * The displayed time remains fixed during this test.
     */
    if (timer0_flag == 1U)
    {
      setTimer0(2000U);
      HAL_GPIO_TogglePin(GPIOA, STATUS_LED_PIN);
    }

#elif LAB_TASK == 7

    /* Clock and DOT move to main; scanning stays in interrupt */
    if (timer0_flag == 1U)
    {
      setTimer0(1000U);

      advanceClockOneSecond();
      HAL_GPIO_TogglePin(GPIOA, DOT_PIN);
    }

#elif LAB_TASK == 8

    /* Clock and DOT event every second */
    if (timer0_flag == 1U)
    {
      setTimer0(1000U);

      advanceClockOneSecond();
      HAL_GPIO_TogglePin(GPIOA, DOT_PIN);
    }

    /* Scanning event every 250 ms */
    if (timer1_flag == 1U)
    {
      setTimer1(SCAN_PERIOD_MS);

      update7SEG(index_led);
      index_led = (index_led + 1) % MAX_LED;
    }

#endif

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
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
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

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, DOT_Pin|LED_RED_Pin|EN0_Pin|EN1_Pin
                          |EN3_Pin|EN4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, SEG_A_Pin|SEG_B_Pin|SEG_C_Pin|SEG_D_Pin
                          |SEG_E_Pin|SEG_F_Pin|SEG_G_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : DOT_Pin LED_RED_Pin EN0_Pin EN1_Pin
                           EN3_Pin EN4_Pin */
  GPIO_InitStruct.Pin = DOT_Pin|LED_RED_Pin|EN0_Pin|EN1_Pin
                          |EN3_Pin|EN4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : SEG_A_Pin SEG_B_Pin SEG_C_Pin SEG_D_Pin
                           SEG_E_Pin SEG_F_Pin SEG_G_Pin */
  GPIO_InitStruct.Pin = SEG_A_Pin|SEG_B_Pin|SEG_C_Pin|SEG_D_Pin
                          |SEG_E_Pin|SEG_F_Pin|SEG_G_Pin;
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

#if LAB_TASK >= 6
  timer_run();
#endif

#if LAB_TASK <= 7
  /* Exercises 2-7: scan displays inside the interrupt */
  if (scanCounter > 0U)
  {
    scanCounter--;
  }

  if (scanCounter == 0U)
  {
    scanCounter = SCAN_PERIOD_MS / TIMER_CYCLE;

    update7SEG(index_led);
    index_led = (index_led + 1) % MAX_LED;
  }
#endif

#if LAB_TASK <= 6
  /* Exercises 2-6: control DOT inside the interrupt */
  if (dotCounter > 0U)
  {
    dotCounter--;
  }

  if (dotCounter == 0U)
  {
    dotCounter = 1000U / TIMER_CYCLE;
    HAL_GPIO_TogglePin(GPIOA, DOT_PIN);
  }
#endif
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

#ifdef  USE_FULL_ASSERT
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

  (void)file;
  (void)line;

  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
