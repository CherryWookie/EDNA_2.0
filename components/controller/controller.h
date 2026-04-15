#pragma once

// =============================================================================
// controller.h
// Thin wrapper around Bluepad32 that normalizes raw gamepad axis values into
// flight setpoints and writes them to the shared globals in main.c.
//
// Bluepad32 axis range: -512 to +511
// Left  stick Y (axis 1): throttle  → mapped to 1000–2000 µs
// Right stick Y (axis 3): pitch     → mapped to ±ANGLE_SETPOINT_MAX degrees
// Right stick X (axis 2): roll      → mapped to ±ANGLE_SETPOINT_MAX degrees
// Left  stick X (axis 0): yaw rate  → mapped to ±YAW_RATE_MAX deg/s
//
// Deadband of STICK_DEADBAND counts is applied at center before scaling.
// =============================================================================

// Initialize Bluepad32 and register connect/disconnect/data callbacks.
// Must be called from app_main before tasks start.
void controller_init(void);

// Poll Bluepad32 event queue. Call at 50 Hz from controller_task.
void controller_poll(void);

// Returns 1 if a gamepad is currently connected, 0 otherwise.
int controller_is_connected(void);
