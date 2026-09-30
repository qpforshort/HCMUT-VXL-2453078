/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Lab 01 - Exercises 6 to 10
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

/* Chon bai: 6, 7, 8, 9 hoac 10 */
#define LAB_TASK 10

/* PA4 den PA15 */
#define CLOCK_PINS ((uint16_t)0xFFF0)

/* LED noi ve GND: muc 1 sang, muc 0 tat */
#define CLOCK_LED_ON  GPIO_PIN_SET
#define CLOCK_LED_OFF GPIO_PIN_RESET

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

void clearAllClock(void);
void setNumberOnClock(int num);
void clearNumberOnClock(int num);

static void displayClock(uint8_t hours,
                         uint8_t minutes,
                         uint8_t seconds);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*
 * Anh xa vi tri:
 * d0  -> PA4  -> 12 gio
 * d1  -> PA5  -> 1 gio
 * ...
 * d11 -> PA15 -> 11 gio
 */

/* Bai 7: tat tat ca 12 LED */
void clearAllClock(void)
{
  HAL_GPIO_WritePin(GPIOA, CLOCK_PINS, CLOCK_LED_OFF);
}

/* Bai 8: bat LED num, giu nguyen cac LED khac */
void setNumberOnClock(int num)
{
  if (num < 0 || num > 11)
  {
    return;
  }

  uint16_t pin = (uint16_t)(GPIO_PIN_4 << num);

  HAL_GPIO_WritePin(GPIOA, pin, CLOCK_LED_ON);
}

/* Bai 9: tat LED num, giu nguyen cac LED khac */
void clearNumberOnClock(int num)
{
  if (num < 0 || num > 11)
  {
    return;
  }

  uint16_t pin = (uint16_t)(GPIO_PIN_4 << num);

  HAL_GPIO_WritePin(GPIOA, pin, CLOCK_LED_OFF);
}

/*
 * Bai 10: hien thi gio, phut, giay tren 12 vi tri.
 *
 * Gio:  hours % 12
 * Phut: minutes / 5
 * Giay: seconds / 5
 *
 * Cac kim trung vi tri se dung chung mot LED.
 */
static void displayClock(uint8_t hours,
                         uint8_t minutes,
                         uint8_t seconds)
{
  clearAllClock();

  setNumberOnClock(hours % 12);
  setNumberOnClock(minutes / 5);
  setNumberOnClock(seconds / 5);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

#if LAB_TASK == 6 || LAB_TASK == 8 || LAB_TASK == 9
  int position = 0;
#elif LAB_TASK == 10
  uint8_t hours = 10;
  uint8_t minutes = 10;
  uint8_t seconds = 30;
#endif

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset peripherals, initialize Flash interface and SysTick */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /*
   * Tat JTAG va SWD de giai phong PA13, PA14, PA15.
   * Cac chan nay duoc dung de dieu khien LED.
   */
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_DISABLE();

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();

  /* USER CODE BEGIN 2 */

  clearAllClock();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

#if LAB_TASK == 6

    /*
     * Bai 6:
     * Kiem tra day noi bang cach bat tung LED.
     * Moi thoi diem chi co mot LED sang.
     */
    clearAllClock();
    setNumberOnClock(position);
    HAL_Delay(500);

    position = (position + 1) % 12;

#elif LAB_TASK == 7

    /*
     * Bai 7:
     * Bat tat ca LED, sau do goi clearAllClock().
     */
    HAL_GPIO_WritePin(GPIOA, CLOCK_PINS, CLOCK_LED_ON);
    HAL_Delay(1000);

    clearAllClock();
    HAL_Delay(1000);

#elif LAB_TASK == 8

    /*
     * Bai 8:
     * Bat tich luy tung LED: 1 -> 2 -> ... -> 12 LED.
     * Sang vong moi thi tat tat ca va bat lai tu d0.
     */
    if (position == 0)
    {
      clearAllClock();
    }

    setNumberOnClock(position);
    HAL_Delay(500);

    position = (position + 1) % 12;

#elif LAB_TASK == 9

    /*
     * Bai 9:
     * Bat tat ca, sau do tat lan luot d0 den d11.
     */
    if (position == 0)
    {
      HAL_GPIO_WritePin(GPIOA, CLOCK_PINS, CLOCK_LED_ON);
      HAL_Delay(1000);
    }

    clearNumberOnClock(position);
    HAL_Delay(500);

    position = (position + 1) % 12;

#elif LAB_TASK == 10

    /*
     * Bai 10:
     * Hien thi thoi gian hien tai trong mot giay.
     */
    displayClock(hours, minutes, seconds);
    HAL_Delay(1000);

    /* Tang thoi gian them mot giay */
    seconds++;

    if (seconds >= 60)
    {
      seconds = 0;
      minutes++;
    }

    if (minutes >= 60)
    {
      minutes = 0;
      hours = (hours + 1) % 12;
    }

#else
#error "LAB_TASK must be 6, 7, 8, 9, or 10"
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

  /* Initialize the internal HSI oscillator */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure CPU, AHB and APB clocks */
  RCC_ClkInitStruct.ClockType =
      RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
      RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;

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
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*
   * Dat muc tat truoc khi chuyen cac chan thanh Output.
   */
  HAL_GPIO_WritePin(GPIOA, CLOCK_PINS, CLOCK_LED_OFF);

  /* Configure PA4 to PA15 as GPIO outputs */
  GPIO_InitStruct.Pin = CLOCK_PINS;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
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
  * @brief Reports the source file and line of an assert error.
  * @param file: source file name
  * @param line: source line number
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
