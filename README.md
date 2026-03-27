# ⚙️ STM32 PID Controlled DC Motor System with Encoder Feedback

This project implements a high-performance **Closed-Loop Speed Control System** for a DC motor using an **STM32** microcontroller. It leverages a PID algorithm to maintain precise RPM regulation despite external load variations.

---

## 🎯 Project Objective

The goal is to achieve stable and accurate motor speed control by processing high-frequency feedback from an incremental encoder. This system is a fundamental building block for robotics, CNC machines, and aerospace deployment mechanisms.

### ✨ Key Features
* **Quadrature Encoder Processing:** Utilizes STM32 Timer hardware in **Encoder Mode** (X4 encoding) for high-resolution feedback.
* **Real-Time PID Regulation:** Implements a discrete-time PID controller to minimize error and overshoot.
* **PWM Speed Control:** Generates high-frequency PWM signals to drive the motor via a bridge driver.
* **Live Tuning Interface:** Ability to update PID constants ($K_p$, $K_i$, $K_d$) and setpoints via Serial (UART).

---

## 🛠 Hardware & Peripherals

| Peripheral | Function |
| :--- | :--- |
| **TIM (Encoder Mode)** | Reads A/B phases of the encoder for position/speed tracking. |
| **TIM (PWM Mode)** | Generates the control signal for the motor driver. |
| **SysTick / Timer** | Provides a fixed sampling time ($T_s$) for the PID calculation. |
| **UART** | Telemetry and command interface for monitoring RPM and tuning. |

---

## 📈 Control Theory Implementation

The controller calculates the output based on the discrete PID formula:

$$u(t) = K_p e(t) + K_i \int e(t)dt + K_d \frac{de(t)}{dt}$$

* **Sampling Rate:** 10ms (100Hz) constant loop.
* **Anti-Windup:** Integral clamping to prevent saturation issues.
* **Filtering:** Low-pass filtering on the derivative term to reduce high-frequency noise from the encoder.

---


## ⚙️ How to Tune

To optimize the system response, adjust the following parameters in the code:

```c
// Example PID Tuning Constants
#define PID_KP  1.25f
#define PID_KI  0.50f
#define PID_KD  0.01f

// Target Speed
float setpoint_rpm = 100.0f;
