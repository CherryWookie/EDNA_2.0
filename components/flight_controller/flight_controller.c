// =============================================================================
// flight_controller.c
// Runs pitch, roll, and yaw PID loops then writes mixed PWM to ESCs.
//
// Serial Plotter output (when DEBUG_PID=1):
//   Format understood by both Arduino IDE Serial Plotter and the Python QTC.
//   Each line:  "pitch_sp:X,pitch:X,pitch_out:X,roll_sp:X,roll:X,roll_out:X,..."
//   Open Arduino Serial Monitor / Serial Plotter at 115200 baud to view.
// =============================================================================

#include "flight_controller.h"
#include "motor_mix.h"
#include "esc.h"
#include "config.h"
#include "log.h"
#include "esp_timer.h"
#include <stdio.h>

static const char *TAG = "FC";

static pid_state_t pid_pitch;
static pid_state_t pid_roll;
static pid_state_t pid_yaw;

static uint32_t last_update_us = 0;

void flight_controller_init(void) {
    pid_init(&pid_pitch, PID_PITCH_KP, PID_PITCH_KI, PID_PITCH_KD);
    pid_init(&pid_roll,  PID_ROLL_KP,  PID_ROLL_KI,  PID_ROLL_KD);
    pid_init(&pid_yaw,   PID_YAW_KP,   PID_YAW_KI,   PID_YAW_KD);
    last_update_us = esp_timer_get_time();
    ESP_LOGI(TAG, "Flight controller initialized");
    ESP_LOGI(TAG, "Pitch PID: Kp=%.3f Ki=%.3f Kd=%.3f",
             PID_PITCH_KP, PID_PITCH_KI, PID_PITCH_KD);
    ESP_LOGI(TAG, "Roll  PID: Kp=%.3f Ki=%.3f Kd=%.3f",
             PID_ROLL_KP,  PID_ROLL_KI,  PID_ROLL_KD);
    ESP_LOGI(TAG, "Yaw   PID: Kp=%.3f Ki=%.3f Kd=%.3f",
             PID_YAW_KP,   PID_YAW_KI,   PID_YAW_KD);
}

void flight_controller_update(int   throttle_us,
                               float pitch_sp, float roll_sp, float yaw_sp,
                               float pitch,    float roll,    float yaw_rate) {
    // Compute dt from real elapsed time, not assumed loop period
    uint32_t now_us = esp_timer_get_time();
    float dt = (now_us - last_update_us) / 1e6f;
    last_update_us = now_us;
    if (dt <= 0.0f || dt > 0.1f) dt = 0.01f;

    // If throttle is at minimum, hold all PIDs reset and kill motors.
    // This prevents integral windup while sitting on the ground.
    if (throttle_us <= ESC_PWM_MIN_US + 20) {
        flight_controller_reset_integrals();
        esc_set_all_us(ESC_PWM_ARM_US);
        return;
    }

    // Run PID loops
    float out_pitch = pid_update(&pid_pitch, pitch_sp, pitch,    dt);
    float out_roll  = pid_update(&pid_roll,  roll_sp,  roll,     dt);
    float out_yaw   = pid_update(&pid_yaw,   yaw_sp,   yaw_rate, dt);

    // Mix and write to ESCs
    motor_outputs_t motors = motor_mix(throttle_us, out_pitch, out_roll, out_yaw);
    esc_set_motors_us(motors.fl, motors.fr, motors.rl, motors.rr);

#if DEBUG_PID
    // Arduino Serial Plotter / QTC compatible format
    // Labels must not contain spaces; separate channels with commas.
    printf("pitch_sp:%.2f,pitch:%.2f,pitch_out:%.2f,"
           "roll_sp:%.2f,roll:%.2f,roll_out:%.2f,"
           "yaw_sp:%.2f,yaw_rate:%.2f,yaw_out:%.2f,"
           "m_fl:%lu,m_fr:%lu,m_rl:%lu,m_rr:%lu\n",
           pitch_sp, pitch, out_pitch,
           roll_sp,  roll,  out_roll,
           yaw_sp,   yaw_rate, out_yaw,
           motors.fl, motors.fr, motors.rl, motors.rr);
#endif
}

void flight_controller_reset_integrals(void) {
    pid_reset(&pid_pitch);
    pid_reset(&pid_roll);
    pid_reset(&pid_yaw);
}

pid_state_t *flight_controller_get_pid_pitch(void) { return &pid_pitch; }
pid_state_t *flight_controller_get_pid_roll(void)  { return &pid_roll;  }
pid_state_t *flight_controller_get_pid_yaw(void)   { return &pid_yaw;   }
