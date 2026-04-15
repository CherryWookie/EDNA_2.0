// =============================================================================
// motor_mix.c
// =============================================================================

#include "motor_mix.h"
#include "config.h"

static uint32_t clamp_us(float val) {
    if (val < ESC_PWM_MIN_US) return ESC_PWM_MIN_US;
    if (val > ESC_PWM_MAX_US) return ESC_PWM_MAX_US;
    return (uint32_t)val;
}

motor_outputs_t motor_mix(int throttle_us, float pid_pitch, float pid_roll, float pid_yaw) {
    motor_outputs_t out;
    float t = (float)throttle_us;

    out.fl = clamp_us(t + pid_pitch + pid_roll - pid_yaw);
    out.fr = clamp_us(t + pid_pitch - pid_roll + pid_yaw);
    out.rl = clamp_us(t - pid_pitch + pid_roll + pid_yaw);
    out.rr = clamp_us(t - pid_pitch - pid_roll - pid_yaw);

    return out;
}
