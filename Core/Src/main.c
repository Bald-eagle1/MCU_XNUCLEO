/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "dht20.h"
#include "lcd.h"
#include <stdio.h>
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

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
DHT20_HandleTypeDef hdht20;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
typedef enum {
    LIGHT_RED = 0,
    LIGHT_AMBER,
    LIGHT_GREEN,
    LIGHT_OFF
} TrafficLightColor_t;

/**
 * @brief Control Traffic Light 1 via 74S139 decoder (TL1_A: PB3, TL1_B: PB5)
 * Truth table (Active-Low decoder outputs):
 *   B=0, A=0 -> Y0: RED_1
 *   B=0, A=1 -> Y1: YELLOW_1 / AMBER
 *   B=1, A=0 -> Y2: GREEN_1
 *   B=1, A=1 -> Y3: OFF (All lights off)
 */
static void Set_TrafficLight1(TrafficLightColor_t color)
{
    switch (color)
    {
        case LIGHT_RED:
            HAL_GPIO_WritePin(TL1_B_GPIO_Port, TL1_B_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(TL1_A_GPIO_Port, TL1_A_Pin, GPIO_PIN_RESET);
            break;
        case LIGHT_AMBER:
            HAL_GPIO_WritePin(TL1_B_GPIO_Port, TL1_B_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(TL1_A_GPIO_Port, TL1_A_Pin, GPIO_PIN_SET);
            break;
        case LIGHT_GREEN:
            HAL_GPIO_WritePin(TL1_B_GPIO_Port, TL1_B_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(TL1_A_GPIO_Port, TL1_A_Pin, GPIO_PIN_RESET);
            break;
        default: /* LIGHT_OFF */
            HAL_GPIO_WritePin(TL1_B_GPIO_Port, TL1_B_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(TL1_A_GPIO_Port, TL1_A_Pin, GPIO_PIN_SET);
            break;
    }
}

/**
 * @brief Control Traffic Light 2 via 74S139 decoder (TL2_A: PB4, TL2_B: PB10)
 * Truth table (Active-Low decoder outputs):
 *   B=0, A=0 -> Y0: RED_2
 *   B=0, A=1 -> Y1: YELLOW_2 / AMBER
 *   B=1, A=0 -> Y2: GREEN_2
 *   B=1, A=1 -> Y3: OFF (All lights off)
 */
static void Set_TrafficLight2(TrafficLightColor_t color)
{
    switch (color)
    {
        case LIGHT_RED:
            HAL_GPIO_WritePin(TL2_B_GPIO_Port, TL2_B_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(TL2_A_GPIO_Port, TL2_A_Pin, GPIO_PIN_RESET);
            break;
        case LIGHT_AMBER:
            HAL_GPIO_WritePin(TL2_B_GPIO_Port, TL2_B_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(TL2_A_GPIO_Port, TL2_A_Pin, GPIO_PIN_SET);
            break;
        case LIGHT_GREEN:
            HAL_GPIO_WritePin(TL2_B_GPIO_Port, TL2_B_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(TL2_A_GPIO_Port, TL2_A_Pin, GPIO_PIN_RESET);
            break;
        default: /* LIGHT_OFF */
            HAL_GPIO_WritePin(TL2_B_GPIO_Port, TL2_B_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(TL2_A_GPIO_Port, TL2_A_Pin, GPIO_PIN_SET);
            break;
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

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  /* Try initializing LCD at Proteus address 0x20, standard 0x27, or fallback 0x3F */
  const uint8_t lcd_addrs[] = {LCD_ADDR_PROTEUS, LCD_ADDR_PCF8574, LCD_ADDR_PCF8574A};
  for (int i = 0; i < 3; i++)
  {
    if (LCD_Init(&hi2c1, lcd_addrs[i]) == HAL_OK)
    {
      break;
    }
  }

  LCD_Clear();
  LCD_SetCursor(0, 0);
  LCD_SendString("Button & TL Test");
  LCD_SetCursor(0, 1);
  LCD_SendString("Press any button");
  HAL_Delay(1000);

  /* Set initial safe traffic light state */
  Set_TrafficLight1(LIGHT_RED);
  Set_TrafficLight2(LIGHT_GREEN);

  /* State tracking variables */
  TrafficLightColor_t tl1_color = LIGHT_RED;
  TrafficLightColor_t tl2_color = LIGHT_GREEN;
  char line_buf[17];
  const char *color_names[] = {"RED", "AMB", "GRN", "OFF"};
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* Read all 4 buttons and user button (Active LOW: pressed = 0) */
    uint8_t btn1_a = (HAL_GPIO_ReadPin(BU1_A_GPIO_Port, BU1_A_Pin) == GPIO_PIN_RESET);
    uint8_t btn1_b = (HAL_GPIO_ReadPin(BU1_B_GPIO_Port, BU1_B_Pin) == GPIO_PIN_RESET);
    uint8_t btn2_a = (HAL_GPIO_ReadPin(BU2_A_GPIO_Port, BU2_A_Pin) == GPIO_PIN_RESET);
    uint8_t btn2_b = (HAL_GPIO_ReadPin(BU2_B_GPIO_Port, BU2_B_Pin) == GPIO_PIN_RESET);
    uint8_t btn_usr = (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET);

    uint8_t any_pressed = 1;
    const char *current_btn_str = "None       ";

    if (btn1_a)
    {
      /* Button 1 (D7 / PA8): Road 1 = RED, Road 2 = GREEN */
      tl1_color = LIGHT_RED;
      tl2_color = LIGHT_GREEN;
      current_btn_str = "BU1_A (D7) ";
    }
    else if (btn1_b)
    {
      /* Button 2 (D8 / PA9): Road 1 = RED, Road 2 = AMBER */
      tl1_color = LIGHT_RED;
      tl2_color = LIGHT_AMBER;
      current_btn_str = "BU1_B (D8) ";
    }
    else if (btn2_a)
    {
      /* Button 3 (D9 / PC7): Road 1 = GREEN, Road 2 = RED */
      tl1_color = LIGHT_GREEN;
      tl2_color = LIGHT_RED;
      current_btn_str = "BU2_A (D9) ";
    }
    else if (btn2_b)
    {
      /* Button 4 (D10 / PB6): Road 1 = AMBER, Road 2 = RED */
      tl1_color = LIGHT_AMBER;
      tl2_color = LIGHT_RED;
      current_btn_str = "BU2_B (D10)";
    }
    else if (btn_usr)
    {
      /* Blue User Button (PC13): Both Roads OFF */
      tl1_color = LIGHT_OFF;
      tl2_color = LIGHT_OFF;
      current_btn_str = "USER (B1)  ";
    }
    else
    {
      any_pressed = 0;
      current_btn_str = "None       ";
    }

    /* Output to traffic lights */
    Set_TrafficLight1(tl1_color);
    Set_TrafficLight2(tl2_color);

    /* Turn on-board LED LD2 (PA5) ON when any button is pressed */
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, any_pressed ? GPIO_PIN_SET : GPIO_PIN_RESET);

    /* Update LCD Line 1: current traffic light states */
    snprintf(line_buf, sizeof(line_buf), "TL1:%-3s  TL2:%-3s",
             color_names[tl1_color], color_names[tl2_color]);
    LCD_SetCursor(0, 0);
    LCD_SendString(line_buf);

    /* Update LCD Line 2: active button */
    snprintf(line_buf, sizeof(line_buf), "Btn: %-11s", current_btn_str);
    LCD_SetCursor(0, 1);
    LCD_SendString(line_buf);

    HAL_Delay(50); /* 50ms polling delay for smooth button responsiveness */
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
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, TL2_B_Pin|TL1_A_Pin|TL2_A_Pin|TL1_B_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : TL2_B_Pin TL1_A_Pin TL2_A_Pin TL1_B_Pin */
  GPIO_InitStruct.Pin = TL2_B_Pin|TL1_A_Pin|TL2_A_Pin|TL1_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : BU2_A_Pin */
  GPIO_InitStruct.Pin = BU2_A_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BU2_A_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PC9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : BU1_A_Pin BU1_B_Pin */
  GPIO_InitStruct.Pin = BU1_A_Pin|BU1_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : BU2_B_Pin */
  GPIO_InitStruct.Pin = BU2_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BU2_B_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

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
  /* User can add his own implementation to report the HAL error return state */
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
