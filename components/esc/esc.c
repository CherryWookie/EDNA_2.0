// =============================================================================
// esc.c
// LEDC-based ESC PWM driver. Maps microsecond pulse widths to LEDC duty cycles.
//
// Duty calculation:
//   Period at 50 Hz = 20,000 µs
//   16-bit resolution = 65535 steps
//   duty = (pulse_us / 20000.0) * 65535
// =============================================================================

#include "esc.h"
#include "config.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "ESC";

// Lookup tables so we can iterate over motors without a switch
static const int motor_gpios[MOTOR_COUNT] = {
    ESC_M1_GPIO, ESC_M2_GPIO, ESC_M3_GPIO, ESC_M4_GPIO
};
static const ledc_channel_t motor_channels[MOTOR_COUNT] = {
    ESC_M1_CHANNEL, ESC_M2_CHANNEL, ESC_M3_CHANNEL, ESC_M4_CHANNEL
};

#define PERIOD_US       20000UL     // 1 / 50 Hz = 20 ms
#define MAX_DUTY        65535UL     // 2^16 - 1

static uint32_t us_to_duty(uint32_t pulse_us) {
    if (pulse_us < ESC_PWM_MIN_US) pulse_us = ESC_PWM_MIN_US;
    if (pulse_us > ESC_PWM_MAX_US) pulse_us = ESC_PWM_MAX_US;
    return (uint32_t)(((uint64_t)pulse_us * MAX_DUTY) / PERIOD_US);
}

esp_err_t esc_init(void) {
    // Configure shared timer
    ledc_timer_config_t timer = {
        .speed_mode      = ESC_LEDC_MODE,
        .timer_num       = ESC_LEDC_TIMER,
        .duty_resolution = ESC_LEDC_RESOLUTION,
        .freq_hz         = ESC_PWM_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    // Configure one channel per motor
    for (int i = 0; i < MOTOR_COUNT; i++) {
        ledc_channel_config_t ch = {
            .speed_mode = ESC_LEDC_MODE,
            .channel    = motor_channels[i],
            .timer_sel  = ESC_LEDC_TIMER,
            .gpio_num   = motor_gpios[i],
            .duty       = us_to_duty(ESC_PWM_ARM_US),
            .hpoint     = 0,
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ch));
    }

    ESP_LOGI(TAG, "ESC PWM initialized on GPIOs %d %d %d %d",
             ESC_M1_GPIO, ESC_M2_GPIO, ESC_M3_GPIO, ESC_M4_GPIO);
    return ESP_OK;
}

void esc_arm(void) {
    esc_set_all_us(ESC_PWM_ARM_US);
    ESP_LOGI(TAG, "ESCs armed at %d µs", ESC_PWM_ARM_US);
}

void esc_set_us(int motor, uint32_t pulse_us) {
    if (motor < 0 || motor >= MOTOR_COUNT) return;
    uint32_t duty = us_to_duty(pulse_us);
    ledc_set_duty(ESC_LEDC_MODE, motor_channels[motor], duty);
    ledc_update_duty(ESC_LEDC_MODE, motor_channels[motor]);
}

void esc_set_motors_us(uint32_t m1, uint32_t m2, uint32_t m3, uint32_t m4) {
    esc_set_us(MOTOR_FL, m1);
    esc_set_us(MOTOR_FR, m2);
    esc_set_us(MOTOR_RL, m3);
    esc_set_us(MOTOR_RR, m4);
}

void esc_set_all_us(uint32_t pulse_us) {
    for (int i = 0; i < MOTOR_COUNT; i++) {
        esc_set_us(i, pulse_us);
    }
}
