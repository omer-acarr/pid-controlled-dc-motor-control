/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - Motor Control with PID and Encoder
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

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
// UART Communication Buffers
char rx_buffer[20];    
int rx_index = 0;      
char rx_char;          

// Energy and Power Tracking
float total_energy_ws = 0.0f;
float total_ampere_seconds = 0.0f;

// PID Controller Variables
float setpoint = 18.0f;        // Target RPM
float kp = 10.0f;              // Proportional Gain
float ki = 0.5f;               // Integral Gain
float kd = 0.0f;               // Derivative Gain
float integral = 0.0f;
float previous_error = 0.0f;

// Motor State and Constraints
int motor_direction = 1;       // 1 = forward, -1 = reverse
int elapsed_seconds = 0;
float TARGET_REVS = 5.0f;      // Target revolutions
float target_pulses = 0.0f;
int PWM_DUTY = 400;            // Base PWM (0-1000)

// Encoder and RPM
#define PULSES_PER_REV 5500
int total_pulses = 0;
int last_count = 0;
int rpm = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
void StartMotor(void);
void ParseCommand(char* cmd);

/* USER CODE BEGIN 0 */
void StartMotor(void) {
    // Reset counters and energy metrics
    total_pulses = 0;
    total_energy_ws = 0.0f;
    total_ampere_seconds = 0.0f;
    
    // Reset PID state
    integral = 0.0f;
    previous_error = 0.0f;

    last_count = 0;
    elapsed_seconds = 0;
    __HAL_TIM_SET_COUNTER(&htim2, 0);

    target_pulses = TARGET_REVS * PULSES_PER_REV;

    // Set Motor Direction
    if (motor_direction == 1) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);    // IN1 = 1
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET); // IN2 = 0
    } else {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET); // IN1 = 0
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);   // IN2 = 1
    }

    // Start PWM and Encoder
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_DUTY);
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);

    // Start Timer 3 (1-second interrupt)
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
    } else if (strncmp(cmd, "dir:", 4) == 0) {
        motor_direction = (atoi(&cmd[4]) > 0) ? 1 : -1;
    } else if (strncmp(cmd, "target:", 7) == 0) {
        setpoint = atof(&cmd[7]);
    }
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();

  // Start UART interrupt reception
  HAL_UART_Receive_IT(&huart1, (uint8_t*)&rx_char, 1);

  while (1)
  {
      // Main loop is empty; tasks are handled via Interrupts (TIM3 and UART)
  }
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM3) {
        // Calculate Delta Pulses
        int current_count = __HAL_TIM_GET_COUNTER(&htim2);
        int delta = (int16_t)(current_count - last_count);
        last_count = current_count;

        total_pulses += abs(delta);
        rpm = (abs(delta) * 60) / PULSES_PER_REV;

        // PID Calculation
        float error = setpoint - rpm;
        integral += error;
        float derivative = error - previous_error;
        float correction = (kp * error) + (ki * integral) + (kd * derivative);
        previous_error = error;

        // Apply Correction to PWM
        int pwm_output = PWM_DUTY + (int)correction;
        if (pwm_output > 1000) pwm_output = 1000;
        if (pwm_output < 0) pwm_output = 0;

        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pwm_output);
        elapsed_seconds++;

        // Power and Energy Calculations (Estimated)
        float max_current = 2.0f; // Amps at 100% duty cycle
        float current = max_current * ((float)pwm_output / 1000.0f);
        float voltage = 12.0f;
        float power = voltage * current;

        total_energy_ws += power; // 1s interval
        total_ampere_seconds += current;
        float consumed_mAh = (total_ampere_seconds / 3600.0f) * 1000.0f;

        // Telemetry via UART
        char msg1[60], msg2[60];
        snprintf(msg1, sizeof(msg1), "Time=%d sec | Current=%.2f mAh\r\n", elapsed_seconds, consumed_mAh);
        HAL_UART_Transmit(&huart1, (uint8_t*)msg1, strlen(msg1), 10);

        snprintf(msg2, sizeof(msg2), "RPM=%d | Power=%.2f W\r\n", rpm, power);
        HAL_UART_Transmit(&huart1, (uint8_t*)msg2, strlen(msg2), 10);

        // Check if target revolutions reached
        if (total_pulses >= target_pulses) {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
            HAL_TIM_Base_Stop_IT(&htim3);
            const char* done_msg = "Target Reached: Stopping Motor\r\n";
            HAL_UART_Transmit(&huart1, (uint8_t*)done_msg, strlen(done_msg), 10);
        }
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        if (rx_char == '\n' || rx_char == '\r')
        {
            rx_buffer[rx_index] = '\0'; // Null-terminate string
            ParseCommand(rx_buffer);
            rx_index = 0;
        }
        else if (rx_index < sizeof(rx_buffer) - 1)
        {
            rx_buffer[rx_index++] = rx_char;
        }

        // Restart interrupt reception
        HAL_UART_Receive_IT(&huart1, (uint8_t*)&rx_char, 1);
    }
}
/* USER CODE END 4 */
