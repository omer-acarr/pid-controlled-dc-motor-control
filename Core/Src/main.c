/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */


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
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

char rx_buffer[20];    // لتجميع الحروف
int rx_index = 0;      // مؤشر موقع الحرف الحالي
char rx_char;          // لتخزين آخر حرف مستلم
float total_energy_ws = 0.0f;
float total_ampere_seconds = 0.0f;

float setpoint = 18.0f;        // القيمة المطلوبة للـ RPM (Target_RPM)
float kp = 10.0f;              // المعامل التناسبي
float ki = 0.5f;               // المعامل التكاملي
float kd = 0.0f;               // المعامل التفاضلي

float integral = 0.0f;
float previous_error = 0.0f;


int motor_direction = 1; // 1 = forward, -1 = reverse
int elapsed_seconds = 0;

//int TARGET_REVS = 10;
float target_pulses = 0.0f;

float TARGET_REVS = 5.0f;
int PWM_DUTY = 400 ;


#define PULSES_PER_REV 5500
//#define TARGET_REVS 10 //rrrrr
//#define PWM_DUTY 200         // (0-1000)

int total_pulses = 0;
int last_count = 0;
int rpm = 0;
//int current_count = 0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void StartMotor(void) {

    // تصفير العدادات والمتغيرات
    total_pulses = 0;
    total_energy_ws = 0.0f;
    total_ampere_seconds = 0.0f;
    //تصفير ال PID
    integral = 0.0f;
    previous_error = 0.0f;


    last_count = 0;
    elapsed_seconds = 0;
    __HAL_TIM_SET_COUNTER(&htim2, 0);

    target_pulses = TARGET_REVS * PULSES_PER_REV;


    if (motor_direction == 1) {
        // Forward
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);   // IN1 = 1
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET); // IN2 = 0
    } else {
        // Reverse
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET); // IN1 = 0
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);   // IN2 = 1
    }


    // تشغيل PWM
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_DUTY);

    // تشغيل الـ Encoder
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);

    // تشغيل مؤقت TIM3 لمقاطعة كل ثانية
    HAL_TIM_Base_Start_IT(&htim3);
}

void ParseCommand(char* cmd) {
    if (strncmp(cmd, "start", 5) == 0) {
        StartMotor();
    } else if (strncmp(cmd, "rev:", 4) == 0) {
        TARGET_REVS = atof(&cmd[4]);
        target_pulses = TARGET_REVS * PULSES_PER_REV;
    } else if (strncmp(cmd, "speed:", 6) == 0) {
        PWM_DUTY = atoi(&cmd[6]);
        if (PWM_DUTY > 1000) PWM_DUTY = 1000;
    } else if (strncmp(cmd, "stop", 4) == 0) {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
        HAL_TIM_Base_Stop_IT(&htim3);
    }
    else if (strncmp(cmd, "dir:", 4) == 0) {
        motor_direction = atoi(&cmd[4]);
        if (motor_direction > 0)
            motor_direction = 1;
        else
            motor_direction = -1;

    }
    else if (strncmp(cmd, "target:", 7) == 0) {
        setpoint = atof(&cmd[7]);
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
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();




//  char rx_buffer[10];  // لاستقبال البيانات
//
//
//
//  /* USER CODE BEGIN 2 */
//  HAL_UART_Receive(&huart1, (uint8_t*)rx_buffer, sizeof(rx_buffer), HAL_MAX_DELAY);
//
//  if (strncmp(rx_buffer, "start", 5) == 0) {
//      StartMotor();
//  }
  HAL_UART_Receive_IT(&huart1, (uint8_t*)&rx_char, 1);



  /* USER CODE END 2 */
  //  StartMotor();
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // مفيش حاجة هنا خلاص، كله بيحصل بالمقاطعة
  }


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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 72;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 1000;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

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

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
//  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI1;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 0;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&htim2, &sConfig) != HAL_OK)
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
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 7199;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 9999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

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
  huart1.Init.BaudRate = 9600;
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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9|GPIO_PIN_10, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA9 PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM3) {


        int current_count = __HAL_TIM_GET_COUNTER(&htim2);
        int delta = (int16_t)(current_count - last_count);
        last_count = current_count;

        total_pulses += abs(delta);
        rpm = (abs(delta) * 60) / PULSES_PER_REV;
        // PID Correction
        float error = setpoint - rpm;
        integral += error;
        float derivative = error - previous_error;
        float correction = kp * error + ki * integral + kd * derivative;
        previous_error = error;


        // PWM = Base + Correction
        int pwm_output = PWM_DUTY + (int)correction;
        if (pwm_output > 1000) pwm_output = 1000;
        if (pwm_output < 0) pwm_output = 0;

        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pwm_output);



        elapsed_seconds++;

        float max_current = 2.0f; // Ampere at 100% PWM
        float current = max_current * ((float)PWM_DUTY / 1000.0f);

        float voltage = 12.0f;

        float power = voltage * current;

        total_energy_ws += power * 1.0f;

        total_ampere_seconds += current * 1.0f;

        float consumed_mAh = (total_ampere_seconds / 3600.0f) * 1000.0f;

        // السطر الأول: الوقت والتيار
        char msg1[60];
        snprintf(msg1, sizeof(msg1),
                 "Time=%d sec  Current=%.2f mAh\r\n",
                 elapsed_seconds, consumed_mAh);
        HAL_UART_Transmit(&huart1, (uint8_t*)msg1, strlen(msg1), HAL_MAX_DELAY);

        // السطر الثاني: RPM والPower
        char msg2[60];
        snprintf(msg2, sizeof(msg2),
                 "RPM=%d  Power=%.2f W\r\n",
                 rpm, power);
        HAL_UART_Transmit(&huart1, (uint8_t*)msg2, strlen(msg2), HAL_MAX_DELAY);




       // int revs = total_pulses / PULSES_PER_REV;
      //  if (revs >= TARGET_REVS)
        if (total_pulses >= target_pulses) {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
            HAL_TIM_Base_Stop_IT(&htim3);

            const char* done_msg = "Finished target revolutions\r\n";
            HAL_UART_Transmit(&huart1, (uint8_t*)done_msg, strlen(done_msg), HAL_MAX_DELAY);
        }
    }
}





//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
//    if (htim->Instance == TIM3) {
//        int current_count = __HAL_TIM_GET_COUNTER(&htim2);
//        int delta = (int16_t)(current_count - last_count);
//        last_count = current_count;
//
//        total_pulses += abs(delta);
//        rpm = (abs(delta) * 60) / PULSES_PER_REV;
//
//        elapsed_seconds++;
//
//        // ارسال البيانات عبر البلوتوث
//        char msg[50];
//        snprintf(msg, sizeof(msg), "RPM=%d  Time=%d sec\r\n", rpm, elapsed_seconds);
//        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
//
//        int revs = total_pulses / PULSES_PER_REV;
//
//        if (revs >= TARGET_REVS) {
//            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
//            HAL_TIM_Base_Stop_IT(&htim3);
//
//            // ارسال رسالة انتهاء
//            const char* done_msg = "Finished target revolutions\r\n";
//            HAL_UART_Transmit(&huart1, (uint8_t*)done_msg, strlen(done_msg), HAL_MAX_DELAY);
//        }
//    }
//}





void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        if (rx_char == '\n' || rx_char == '\r')
        {
            rx_buffer[rx_index] = 0; // إنهاء السلسلة
            ParseCommand(rx_buffer);
            rx_index = 0; // إعادة المؤشر للبداية
        }
        else
        {
            if (rx_index < sizeof(rx_buffer) - 1)
            {
                rx_buffer[rx_index++] = rx_char;
            }

        }


        // إعادة استقبال الحرف التالي
        HAL_UART_Receive_IT(&huart1, (uint8_t*)&rx_char, 1);
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
