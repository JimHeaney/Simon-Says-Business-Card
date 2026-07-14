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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "simon_game.h"
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
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim14;

/* USER CODE BEGIN PV */
SimonGame_t gameInstance; // Define the game state structure

// Individual brightness calibration for each game LED (Range: 0 to 65535)
// Adjust these values relative to each other  until all 4 look equally bright!
volatile uint32_t brightness_led1 = 3000;  // Adjust for LED 1 (Blue)
volatile uint32_t brightness_led2 = 6000;  // Adjust for LED 2 (Green)
volatile uint32_t brightness_led3 = 6000;  // Adjust for LED 3 (Yellow)
volatile uint32_t brightness_led4 = 3000;  // Adjust for LED 4 (Red)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM14_Init(void);
/* USER CODE BEGIN PFP */
// Function declarations for hardware integration
void my_SetLed(uint8_t index, bool state);
bool my_GetButton(uint8_t index);
void my_UpdateScore(uint8_t score);
void my_SaveHighScore(uint8_t score);
uint8_t my_LoadHighScore(void);
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
  MX_I2C1_Init();
  MX_TIM3_Init();
  MX_TIM14_Init();

  /* USER CODE BEGIN 2 */
    const uint16_t AW9523_ADDR = 0x5B << 1;

    // 1. Set Global Control Register (0x11)
    // Bits [1:0] set the Max current:
    // 0b00 = 37mA, 0b01 = 27.75mA, 0b10 = 18.5mA, 0b11 = 9.25mA
    // Let's configure it to 9.25mA (0x03) for safe, power-efficient, resistor-less operation
    uint8_t gcr_val = 0x03;
    HAL_I2C_Mem_Write(&hi2c1, AW9523_ADDR, 0x11, I2C_MEMADD_SIZE_8BIT, &gcr_val, 1, 100);

    // 2. Configure Pin Modes (0x12 for Port 0, 0x13 for Port 1)
    // A bit value of '0' enables Constant-Current LED Mode. '1' is standard GPIO.
    // We want Port 0 pins 0-7 (all 8) in LED mode -> 0x00
    // We want Port 1 pins 0-5 in LED mode, pins 6-7 as standard GPIO -> 0xC0 (1100 0000)
    uint8_t port0_mode = 0x00;
    uint8_t port1_mode = 0xC0;
    HAL_I2C_Mem_Write(&hi2c1, AW9523_ADDR, 0x12, I2C_MEMADD_SIZE_8BIT, &port0_mode, 1, 100);
    HAL_I2C_Mem_Write(&hi2c1, AW9523_ADDR, 0x13, I2C_MEMADD_SIZE_8BIT, &port1_mode, 1, 100);

    // 3. Clear/turn off all LED current registers initially (so they don't glow at startup)
    // In AW9523, Port 0 dimming registers are at 0x24 to 0x2B
    // Port 1 (pins 0-3) dimming registers are 0x20 to 0x23, (pins 4-7) are 0x2C to 0x2F
    uint8_t zero_brightness = 0x00;
    for (uint8_t reg = 0x20; reg <= 0x2F; reg++) {
        HAL_I2C_Mem_Write(&hi2c1, AW9523_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &zero_brightness, 1, 10);
    }

    // --- START PWM CHANNELS ---
      HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);  // LED 1
      HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);  // LED 2
      HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);  // LED 3
      HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1); // LED 4

    // 4. Load high score and initialize game
    uint8_t storedHighScore = my_LoadHighScore();
    SimonGame_Init(&gameInstance, storedHighScore, my_SetLed, my_GetButton, my_UpdateScore, my_SaveHighScore);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  SimonGame_Update(&gameInstance);
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00503D58;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM14 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM14_Init(void)
{

  /* USER CODE BEGIN TIM14_Init 0 */

  /* USER CODE END TIM14_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM14_Init 1 */

  /* USER CODE END TIM14_Init 1 */
  htim14.Instance = TIM14;
  htim14.Init.Prescaler = 0;
  htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim14.Init.Period = 65535;
  htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim14) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim14) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim14, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM14_Init 2 */

  /* USER CODE END TIM14_Init 2 */
  HAL_TIM_MspPostInit(&htim14);

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
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pins : Button2_Pin Button1_Pin Button3_Pin Button4_Pin */
  GPIO_InitStruct.Pin = Button2_Pin|Button1_Pin|Button3_Pin|Button4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void my_SetLed(uint8_t index, bool state) {
    switch (index) {
        case 0: // Game Index 0 (Button 1) -> Physical LED 1 (TIM3_CH1)
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, state ? brightness_led1 : 0);
            break;

        case 1: // Game Index 1 (Button 2) -> Physical LED 4 (TIM14_CH1)
            __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, state ? brightness_led4 : 0);
            break;

        case 2: // Game Index 2 (Button 3) -> Physical LED 2 (TIM3_CH2)
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, state ? brightness_led2 : 0);
            break;

        case 3: // Game Index 3 (Button 4) -> Physical LED 3 (TIM3_CH3)
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, state ? brightness_led3 : 0);
            break;

        default:
            break;
    }
}

bool my_GetButton(uint8_t index) {
    if (index == 0) return HAL_GPIO_ReadPin(Button1_GPIO_Port, Button1_Pin) == GPIO_PIN_SET;
    if (index == 1) return HAL_GPIO_ReadPin(Button2_GPIO_Port, Button2_Pin) == GPIO_PIN_SET;
    if (index == 2) return HAL_GPIO_ReadPin(Button3_GPIO_Port, Button3_Pin) == GPIO_PIN_SET;
    if (index == 3) return HAL_GPIO_ReadPin(Button4_GPIO_Port, Button4_Pin) == GPIO_PIN_SET;
    return false;
}

void my_UpdateScore(uint8_t score) {
    const uint16_t AW9523_ADDR = 0x5B << 1;

    // 10% brightness limit (Range: 0 - 255)
    const uint8_t MAX_BRIGHTNESS = 5;

    // Standard 7-segment lookup table for numbers 0-9
    // Bit mapping: [MSB] . G F E D C B A [LSB]
    const uint8_t seg_lookup[10] = {
        0x3F, // 0 -> A, B, C, D, E, F
        0x06, // 1 -> B, C
        0x5B, // 2 -> A, B, D, E, G
        0x4F, // 3 -> A, B, C, D, G
        0x66, // 4 -> B, C, F, G
        0x6D, // 5 -> A, C, D, F, G
        0x7D, // 6 -> A, C, D, E, F, G
        0x07, // 7 -> A, B, C
        0x7F, // 8 -> A, B, C, D, E, F, G
        0x6F  // 9 -> A, B, C, D, F, G
    };

    // Your custom hardware register arrays mapped to A, B, C, D, E, F, G
    const uint8_t digit1_regs[7] = {0x27, 0x28, 0x2A, 0x2C, 0x2D, 0x2B, 0x29}; // Tens (LED14, 18, 16, 12, 15, 13, 17)
    const uint8_t digit2_regs[7] = {0x20, 0x21, 0x22, 0x24, 0x25, 0x26, 0x23}; // Ones (LED5, 6, 9, 11, 8, 7, 10)

    // Cap the score display at 99
    if (score > 99) {
        score = 99;
    }

    // Split score into digits
    uint8_t tens = score / 10;
    uint8_t ones = score % 10;

    // Get segment patterns
    uint8_t tens_pattern = seg_lookup[tens];
    uint8_t ones_pattern = seg_lookup[ones];

    // Blank leading zero if score is less than 10
    if (score < 10) {
        tens_pattern = 0x00;
    }

    // Write to Digit 1 (Tens)
    for (uint8_t i = 0; i < 7; i++) {
        uint8_t brightness = (tens_pattern & (1 << i)) ? MAX_BRIGHTNESS : 0;
        HAL_I2C_Mem_Write(&hi2c1, AW9523_ADDR, digit1_regs[i], I2C_MEMADD_SIZE_8BIT, &brightness, 1, 10);
    }

    // Write to Digit 2 (Ones)
    for (uint8_t i = 0; i < 7; i++) {
        uint8_t brightness = (ones_pattern & (1 << i)) ? MAX_BRIGHTNESS : 0;
        HAL_I2C_Mem_Write(&hi2c1, AW9523_ADDR, digit2_regs[i], I2C_MEMADD_SIZE_8BIT, &brightness, 1, 10);
    }
}

void my_SaveHighScore(uint8_t score) {
    // 1. Unlock the Flash
    HAL_FLASH_Unlock();

    // 2. Setup the Erase configuration (Page 31 is the last page of G030 64KB Flash)
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Page = 31;
    EraseInitStruct.NbPages = 1;

    // 3. Erase the page (required before writing new data)
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) == HAL_OK) {
        // STM32G0 requires writing in 64-bit double-words.
        uint64_t dataToWrite = score;
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, 0x0800F800, dataToWrite);
    }

    // 4. Lock the Flash again
    HAL_FLASH_Lock();
}

uint8_t my_LoadHighScore(void) {
    // Read directly from Page 31 base address
    uint8_t storedScore = (*(__IO uint64_t*)0x0800F800) & 0xFF;
    return (storedScore > 14) ? 0 : storedScore;
}
/* USER CODE END 4 */
/* USER CODE END 4 */

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
