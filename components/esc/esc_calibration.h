#pragma once

// =============================================================================
// esc_calibration.h
// One-shot ESC calibration routine. Call ONCE before normal flight operation
// if ESCs have not been calibrated, or after replacing an ESC.
//
// SAFETY: Remove ALL propellers before running this routine.
//
// Procedure (automated):
//   1. Send 2000 µs (max throttle) — ESP32 powered, battery UNPLUGGED
//   2. Plug in battery — ESC beeps (entering calibration mode)
//   3. Wait for first beep sequence to end
//   4. Send 1000 µs (min throttle)
//   5. Second beep series confirms calibration complete
//   6. Unplug battery
//
// This function blocks for the full duration (~10 seconds).
// It prints instructions to serial so you know when to plug/unplug.
// =============================================================================

void esc_calibrate(void);
