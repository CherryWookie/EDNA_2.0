#pragma once

// =============================================================================
// pid.h
// Generic discrete PID controller.
// One instance per axis. Thread-safe if you protect the instance externally
// (the flight controller task does this via its own 100 Hz single-task access).
//
// Usage:
//   pid_state_t p;
//   pid_init(&p, 1.2f, 0.01f, 0.08f);
//   float out = pid_update(&p, setpoint, measurement, dt);
// =============================================================================

typedef struct {
    float kp;
    float ki;
    float kd;

    float integral;
    float prev_error;

    float output_limit;     // Symmetric clamp on final output
    float integral_limit;   // Anti-windup: clamps integral term
} pid_state_t;

// Initialize PID with gains and default limits from config.h
void pid_init(pid_state_t *p, float kp, float ki, float kd);

// Compute one PID step.
// setpoint:    desired value (degrees for pitch/roll, deg/s for yaw)
// measurement: current value from sensor
// dt:          time since last call (seconds)
// Returns: controller output (added to throttle in motor mix)
float pid_update(pid_state_t *p, float setpoint, float measurement, float dt);

// Reset integrator and derivative history (call on disarm or angle-limit trip)
void pid_reset(pid_state_t *p);

// Hot-update gains without resetting state (used by telemetry tuning)
void pid_set_gains(pid_state_t *p, float kp, float ki, float kd);
