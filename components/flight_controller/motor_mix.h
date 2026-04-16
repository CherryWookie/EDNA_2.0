#pragma once

// =============================================================================
// motor_mix.h
// X-configuration quadcopter motor mixing.
//
// Takes throttle (µs base) plus PID outputs for pitch, roll, yaw and produces
// four clamped PWM values ready to send to the ESC driver.
//
// Motor layout (X-config, top-down view):
//
//      FRONT
//   FL (CW)   FR (CCW)

//   RL (CCW)  RR (CW)
//
// Mixing table:
//   FL = throttle + pitch + roll - yaw   (nose up → FL up; right roll → FL up; CCW yaw → FL up)
//   FR = throttle + pitch - roll + yaw
//   RL = throttle - pitch + roll + yaw
//   RR = throttle - pitch - roll - yaw
//
// Signs assume: pitch+ = nose up, roll+ = right side down, yaw+ = nose left (CCW).
// Verify motor rotation directions match this assumption on the bench.
// =============================================================================

#include <stdint.h>

typedef struct {
    uint32_t fl;    // Front-left  (µs)
    uint32_t fr;    // Front-right (µs)
    uint32_t rl;    // Rear-left   (µs)
    uint32_t rr;    // Rear-right  (µs)
} motor_outputs_t;

// Compute mixed motor outputs.
// throttle_us: base PWM in microseconds (1000–2000)
// pid_pitch, pid_roll, pid_yaw: PID controller outputs (float, ±PID_OUTPUT_LIMIT)
motor_outputs_t motor_mix(int throttle_us,
                           float pid_pitch,
                           float pid_roll,
                           float pid_yaw);
