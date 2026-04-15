#pragma once

// =============================================================================
// flight_controller.h
// Top-level flight controller. Owns the three PID instances (pitch, roll, yaw)
// and calls the motor mixer. This is the only file that touches both the PID
// layer and the ESC layer simultaneously.
//
// Called from the flight_ctrl_task in main.c at FC_LOOP_RATE_HZ (100 Hz).
// =============================================================================

#include "pid.h"

// Initialize PID instances with defaults from config.h
void flight_controller_init(void);

// Run one control loop iteration.
// throttle_us: raw throttle from gamepad (1000–2000 µs)
// *_sp:        setpoints from gamepad (degrees for pitch/roll, deg/s for yaw)
// pitch/roll:  measured angles from Kalman filter (degrees)
// yaw_rate:    measured yaw rate from gyro (deg/s)
void flight_controller_update(int    throttle_us,
                               float  pitch_sp,
                               float  roll_sp,
                               float  yaw_sp,
                               float  pitch,
                               float  roll,
                               float  yaw_rate);

// Reset all three PID integrators (call on disarm or angle-limit trip)
void flight_controller_reset_integrals(void);

// Expose PID pointers so telemetry can hot-update gains without a lock.
// The caller must not store these pointers beyond the current task scope.
pid_state_t *flight_controller_get_pid_pitch(void);
pid_state_t *flight_controller_get_pid_roll(void);
pid_state_t *flight_controller_get_pid_yaw(void);
