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
#define LAB_TASK 5   /* Chọn 3, 4 hoặc 5 */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/* USER CODE BEGIN PFP */
static void setTrafficPhase(GPIO_PinState redV,
                            GPIO_PinState yellowV,
                            GPIO_PinState greenV,
                            GPIO_PinState redH,
                            GPIO_PinState yellowH,
                            GPIO_PinState greenH);
void display7SEG(int num);
static void showCountdown(int seconds);
static void runTrafficCycle(uint8_t withCountdown);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*
 * PA1–PA3: đèn trái–phải (V).
 * PA8–PA10: đèn trên–dưới (H).
 * Các LED bài 3 nối về GND: GPIO_PIN_SET = sáng.
 */
static void setTrafficPhase(GPIO_PinState redV,
                            GPIO_PinState yellowV,
                            GPIO_PinState greenV,
                            GPIO_PinState redH,
                            GPIO_PinState yellowH,
                            GPIO_PinState greenH)
{
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, redV);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, yellowV);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, greenV);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, redH);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, yellowH);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, greenH);
}

/*
 * PB0–PB6 nối lần lượt với a–g.
 * 7SEG-COM-ANODE: GPIO_PIN_RESET = đoạn sáng.
 */
void display7SEG(int num)
{
  static const uint8_t digits[10] = {
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

  if (num < 0 || num > 9)
  {
    return;
  }

  for (uint8_t i = 0; i < 7; i++)
  {
    uint16_t pin = (uint16_t)(1U << i);

    GPIO_PinState state =
        (digits[num] & (1U << i))
            ? GPIO_PIN_RESET
            : GPIO_PIN_SET;

    HAL_GPIO_WritePin(GPIOB, pin, state);
  }
}

static void showCountdown(int seconds)
{
  for (int remaining = seconds; remaining >= 1; remaining--)
  {
    display7SEG(remaining);
    HAL_Delay(1000);
  }
}

/*
 * Chu kỳ: H xanh 3 s, H vàng 2 s,
 *         V xanh 3 s, V vàng 2 s.
 * Khi withCountdown = 1, LED 7 đoạn đếm thời gian pha hiện tại.
 */
static void runTrafficCycle(uint8_t withCountdown)
{
  /* Trên–dưới xanh; trái–phải đỏ */
  setTrafficPhase(GPIO_PIN_SET,   GPIO_PIN_RESET, GPIO_PIN_RESET,
                  GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_SET);

  if (withCountdown) showCountdown(3);
  else HAL_Delay(3000);

  /* Trên–dưới vàng; trái–phải đỏ */
  setTrafficPhase(GPIO_PIN_SET,   GPIO_PIN_RESET, GPIO_PIN_RESET,
                  GPIO_PIN_RESET, GPIO_PIN_SET,   GPIO_PIN_RESET);

  if (withCountdown) showCountdown(2);
  else HAL_Delay(2000);

  /* Trái–phải xanh; trên–dưới đỏ */
  setTrafficPhase(GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_SET,
                  GPIO_PIN_SET,   GPIO_PIN_RESET, GPIO_PIN_RESET);

  if (withCountdown) showCountdown(3);
  else HAL_Delay(3000);

  /* Trái–phải vàng; trên–dưới đỏ */
  setTrafficPhase(GPIO_PIN_RESET, GPIO_PIN_SET,   GPIO_PIN_RESET,
                  GPIO_PIN_SET,   GPIO_PIN_RESET, GPIO_PIN_RESET);

  if (withCountdown) showCountdown(2);
  else HAL_Delay(2000);
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

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();

  /* USER CODE BEGIN 2 */

  /*
   * Ba LED cũ của bài 2 ở PA5–PA7 nối về +3.3 V:
   * SET để chúng tắt trong lúc chạy bài 3–5.
   */
  HAL_GPIO_WritePin(GPIOA,
                    LED_RED_Pin | LED_YELLOW_Pin | LED_GREEN_Pin,
                    GPIO_PIN_SET);

  /* 7 đoạn common-anode: SET để tắt toàn bộ các đoạn ban đầu. */
  HAL_GPIO_WritePin(GPIOB,
                    GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 |
                    GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 |
                    GPIO_PIN_6,
                    GPIO_PIN_SET);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
#if LAB_TASK == 4
  int counter = 0;
#endif

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

#if LAB_TASK == 3
    runTrafficCycle(0);

#elif LAB_TASK == 4
    display7SEG(counter);
    counter = (counter + 1) % 10;
    HAL_Delay(1000);

#elif LAB_TASK == 5
    runTrafficCycle(1);

#else
#error "LAB_TASK must be 3, 4, or 5"
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
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
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

  /* Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA,
                    GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                    LED_RED_Pin | LED_YELLOW_Pin | LED_GREEN_Pin |
                    GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10,
                    GPIO_PIN_RESET);

  HAL_GPIO_WritePin(GPIOB,
                    GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 |
                    GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 |
                    GPIO_PIN_6,
                    GPIO_PIN_SET);

  /* Configure GPIOA output pins */
  GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                        LED_RED_Pin | LED_YELLOW_Pin | LED_GREEN_Pin |
                        GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* Configure PB0–PB6 as outputs */
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 |
                        GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 |
                        GPIO_PIN_6;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

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
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
