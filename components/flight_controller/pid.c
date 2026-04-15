// =============================================================================
// pid.c
// =============================================================================

#include "pid.h"
#include "config.h"

void pid_init(pid_state_t *p, float kp, float ki, float kd) {
    p->kp             = kp;
    p->ki             = ki;
    p->kd             = kd;
    p->integral       = 0.0f;
    p->prev_error     = 0.0f;
    p->output_limit   = PID_OUTPUT_LIMIT;
    p->integral_limit = PID_INTEGRAL_LIMIT;
}

float pid_update(pid_state_t *p, float setpoint, float measurement, float dt) {
    float error = setpoint - measurement;

    // Proportional term
    float p_term = p->kp * error;

    // Integral term with anti-windup clamp
    p->integral += error * dt;
    if      (p->integral >  p->integral_limit) p->integral =  p->integral_limit;
    else if (p->integral < -p->integral_limit) p->integral = -p->integral_limit;
    float i_term = p->ki * p->integral;

    // Derivative term (on measurement, not error — avoids derivative kick)
    float d_term = 0.0f;
    if (dt > 0.0f) {
        d_term = p->kd * ((error - p->prev_error) / dt);
    }
    p->prev_error = error;

    // Sum and clamp output
    float output = p_term + i_term + d_term;
    if      (output >  p->output_limit) output =  p->output_limit;
    else if (output < -p->output_limit) output = -p->output_limit;

    return output;
}

void pid_reset(pid_state_t *p) {
    p->integral   = 0.0f;
    p->prev_error = 0.0f;
}

void pid_set_gains(pid_state_t *p, float kp, float ki, float kd) {
    p->kp = kp;
    p->ki = ki;
    p->kd = kd;
}
