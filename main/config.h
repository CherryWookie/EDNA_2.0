#pragma once

// =============================================================================
// config.h
// Global configuration: pin mappings, PWM limits, PID defaults, WiFi settings.
// All hardware-specific constants live here. Change this file, not the drivers.
// =============================================================================

// -----------------------------------------------------------------------------
// ESC / Motor GPIO pins
// Motor layout (X-config, viewed from above):
//
//   M1 (CW)  [FL] ------- [FR] M2 (CCW)
//                   [  ]
//   M3 (CCW) [RL] ------- [RR] M4 (CW)
//
// NOTE: Replace these with your actual wired GPIOs before flashing.
// -----------------------------------------------------------------------------
#define ESC_M1_GPIO     25      // Front-left
#define ESC_M2_GPIO     26      // Front-right
#define ESC_M3_GPIO     27      // Rear-left
#define ESC_M4_GPIO     32      // Rear-right

// ESC PWM pulse width limits (microseconds)
#define ESC_PWM_MIN_US  1000
#define ESC_PWM_MAX_US  2000
#define ESC_PWM_ARM_US  1000    // Value sent during arming / idle
#define ESC_PWM_FREQ_HZ 50      // Standard 50 Hz PWM for ESCs

// LEDC timer and channel assignments (one channel per motor)
#define ESC_LEDC_TIMER      LEDC_TIMER_0
#define ESC_LEDC_MODE       LEDC_LOW_SPEED_MODE
#define ESC_M1_CHANNEL      LEDC_CHANNEL_0
#define ESC_M2_CHANNEL      LEDC_CHANNEL_1
#define ESC_M3_CHANNEL      LEDC_CHANNEL_2
#define ESC_M4_CHANNEL      LEDC_CHANNEL_3
#define ESC_LEDC_RESOLUTION LEDC_TIMER_16_BIT   // 65535 steps

// -----------------------------------------------------------------------------
// MPU-6050 I2C
// -----------------------------------------------------------------------------
#define IMU_I2C_PORT    I2C_NUM_0
#define IMU_SDA_GPIO    21
#define IMU_SCL_GPIO    22
#define IMU_I2C_FREQ_HZ 400000   // 400 kHz fast mode
#define IMU_I2C_ADDR    0x68     // AD0 low; use 0x69 if AD0 pulled high

// MPU-6050 axis remapping for physical mounting:
//   +X faces DOWN  → remap to -Z (negate and swap)
//   +Y faces BACK  → remap to -Y (negate)
//   +Z faces LEFT  → remap to -X (negate and swap)
// Applied in mpu6050_read() after raw register reads.
// Result: standard aviation frame (X=forward, Y=right, Z=up).

// -----------------------------------------------------------------------------
// Flight controller loop rate
// -----------------------------------------------------------------------------
#define FC_LOOP_RATE_HZ     100                     // 100 Hz control loop
#define FC_LOOP_PERIOD_MS   (1000 / FC_LOOP_RATE_HZ)

// -----------------------------------------------------------------------------
// PID defaults  (tune via serial or WiFi QTC — do not need reflash)
// These are conservative starting values. Kp first, then Kd, then Ki.
// -----------------------------------------------------------------------------
#define PID_PITCH_KP    1.2f
#define PID_PITCH_KI    0.01f
#define PID_PITCH_KD    0.08f

#define PID_ROLL_KP     1.2f
#define PID_ROLL_KI     0.01f
#define PID_ROLL_KD     0.08f

#define PID_YAW_KP      2.0f
#define PID_YAW_KI      0.05f
#define PID_YAW_KD      0.0f

#define PID_OUTPUT_LIMIT    400.0f   // Max correction added/subtracted from throttle
#define PID_INTEGRAL_LIMIT  200.0f   // Anti-windup clamp

// -----------------------------------------------------------------------------
// Angle limits (degrees) — safety cutoff
// If the drone exceeds these angles the motors are killed.
// -----------------------------------------------------------------------------
#define ANGLE_LIMIT_DEG     45.0f

// -----------------------------------------------------------------------------
// WiFi / Telemetry
// -----------------------------------------------------------------------------
#define WIFI_SSID           "edna"
#define WIFI_PASSWORD       "quadcopter"     // Min 8 chars for WPA2
#define WIFI_CHANNEL        1
#define WIFI_MAX_CONN       1

#define TELEMETRY_PORT      4444
#define TELEMETRY_RATE_HZ   50              // UDP packet rate

// -----------------------------------------------------------------------------
// Controller (Bluepad32)
// Stick axis raw range is typically -512 to +511.
// Deadband prevents motor jitter at stick neutral.
// -----------------------------------------------------------------------------
#define STICK_MAX           512
#define STICK_DEADBAND      20
#define THROTTLE_SCALE      (ESC_PWM_MAX_US - ESC_PWM_MIN_US)  // maps 0..512 → 0..1000
#define ANGLE_SETPOINT_MAX  20.0f    // degrees of pitch/roll at full stick deflection
#define YAW_RATE_MAX        90.0f    // deg/s at full yaw stick

// -----------------------------------------------------------------------------
// Debug / logging
// Set to 1 to enable verbose serial output from each module.
// -----------------------------------------------------------------------------
#define DEBUG_IMU       0
#define DEBUG_PID       1    // Enable for Serial Plotter tuning
#define DEBUG_MOTORS    0
#define DEBUG_CONTROLLER 0
