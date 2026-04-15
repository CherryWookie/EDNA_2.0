#pragma once

// =============================================================================
// esc.h
// PWM driver for four ESCs using the ESP32 LEDC peripheral.
// All four channels share one LEDC timer running at 50 Hz.
// Pulse width is specified in microseconds (1000 = min, 2000 = max).
// =============================================================================

#include "esp_err.h"
#include <stdint.h>

// Motor index constants — use these everywhere instead of raw numbers
#define MOTOR_FL    0   // Front-left  (clockwise)
#define MOTOR_FR    1   // Front-right (counter-clockwise)
#define MOTOR_RL    2   // Rear-left   (counter-clockwise)
#define MOTOR_RR    3   // Rear-right  (clockwise)
#define MOTOR_COUNT 4

// Initialize LEDC timer and all four channels. Sets all motors to ESC_PWM_ARM_US.
esp_err_t esc_init(void);

// Send arming signal (ESC_PWM_ARM_US) to all motors. Call after esc_init.
void esc_arm(void);

// Set one motor by index (MOTOR_FL .. MOTOR_RR), value in microseconds.
// Clamps to [ESC_PWM_MIN_US, ESC_PWM_MAX_US].
void esc_set_us(int motor, uint32_t pulse_us);

// Set all four motors at once. Order: [FL, FR, RL, RR].
void esc_set_motors_us(uint32_t m1, uint32_t m2, uint32_t m3, uint32_t m4);

// Set all four motors to the same value (e.g. kill / arm).
void esc_set_all_us(uint32_t pulse_us);
