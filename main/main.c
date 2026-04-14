// =============================================================================
// main.c
// Application entry point. Initializes all subsystems in dependency order,
// then starts FreeRTOS tasks. Nothing flight-critical lives here — this file
// is purely orchestration.
//
// Task map:
//   imu_task         — reads MPU-6050, runs Kalman filter @ 100 Hz (core 1)
//   flight_ctrl_task — runs PID loops, writes motor PWM @ 100 Hz (core 1)
//   controller_task  — polls Bluepad32 gamepad state @ 50 Hz (core 0)
//   telemetry_task   — sends UDP packets, receives PID updates @ 50 Hz (core 0)
// =============================================================================

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "config.h"
#include "../components/imu/mpu6050.h"
#include "../components/imu/kalman.h"
#include "../components/esc/esc.h"
#include "../components/flight_controller/flight_controller.h"
#include "../components/controller/controller.h"
#include "../components/telemetry/wifi_server.h"
#include "../components/telemetry/telemetry.h"

static const char *TAG = "MAIN";

// Shared state — written by IMU task, read by flight controller task.
// Protected by imu_mutex so reads are never torn.
SemaphoreHandle_t imu_mutex;
volatile float g_pitch_deg = 0.0f;
volatile float g_roll_deg  = 0.0f;
volatile float g_yaw_rate  = 0.0f;   // deg/s from gyro Z (remapped)

// Shared setpoints — written by controller task, read by flight controller.
SemaphoreHandle_t setpoint_mutex;
volatile int   g_throttle  = ESC_PWM_ARM_US;
volatile float g_pitch_sp  = 0.0f;
volatile float g_roll_sp   = 0.0f;
volatile float g_yaw_sp    = 0.0f;

// IMU + Kalman filter task (high priority, pinned to core 1)
static void imu_task(void *arg) {
    mpu6050_data_t raw;
    kalman_state_t kf_pitch, kf_roll;
    kalman_init(&kf_pitch);
    kalman_init(&kf_roll);

    TickType_t last_wake = xTaskGetTickCount();
    uint32_t   last_us   = esp_timer_get_time();

    while (1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(FC_LOOP_PERIOD_MS));

        uint32_t now_us = esp_timer_get_time();
        float dt = (now_us - last_us) / 1e6f;
        last_us  = now_us;
        if (dt <= 0.0f || dt > 0.1f) dt = 0.01f;   // guard against bad dt

        if (mpu6050_read(&raw) != ESP_OK) {
            ESP_LOGW(TAG, "IMU read failed");
            continue;
        }

        // Compute accel angles (degrees) — uses remapped axes
        float accel_pitch = raw.accel_angle_pitch;
        float accel_roll  = raw.accel_angle_roll;

        float pitch = kalman_update(&kf_pitch, accel_pitch, raw.gyro_x_dps, dt);
        float roll  = kalman_update(&kf_roll,  accel_roll,  raw.gyro_y_dps, dt);

        if (xSemaphoreTake(imu_mutex, 0) == pdTRUE) {
            g_pitch_deg = pitch;
            g_roll_deg  = roll;
            g_yaw_rate  = raw.gyro_z_dps;
            xSemaphoreGive(imu_mutex);
        }

#if DEBUG_IMU
        ESP_LOGI(TAG, "pitch:%.2f roll:%.2f yaw_rate:%.2f", pitch, roll, raw.gyro_z_dps);
#endif
    }
}

// Flight controller task (high priority, pinned to core 1)
static void flight_ctrl_task(void *arg) {
    flight_controller_init();
    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(FC_LOOP_PERIOD_MS));

        float pitch, roll, yaw_rate;
        int   throttle;
        float pitch_sp, roll_sp, yaw_sp;

        // Snapshot shared state
        if (xSemaphoreTake(imu_mutex, pdMS_TO_TICKS(2)) == pdTRUE) {
            pitch    = g_pitch_deg;
            roll     = g_roll_deg;
            yaw_rate = g_yaw_rate;
            xSemaphoreGive(imu_mutex);
        } else continue;

        if (xSemaphoreTake(setpoint_mutex, pdMS_TO_TICKS(2)) == pdTRUE) {
            throttle = g_throttle;
            pitch_sp = g_pitch_sp;
            roll_sp  = g_roll_sp;
            yaw_sp   = g_yaw_sp;
            xSemaphoreGive(setpoint_mutex);
        } else continue;

        // Safety cutoff — kill motors if attitude exceeds limit
        if (pitch > ANGLE_LIMIT_DEG || pitch < -ANGLE_LIMIT_DEG ||
            roll  > ANGLE_LIMIT_DEG || roll  < -ANGLE_LIMIT_DEG) {
            esc_set_all_us(ESC_PWM_ARM_US);
            flight_controller_reset_integrals();
            ESP_LOGW(TAG, "ANGLE LIMIT EXCEEDED — motors killed");
            continue;
        }

        flight_controller_update(throttle, pitch_sp, roll_sp, yaw_sp,
                                 pitch, roll, yaw_rate);
    }
}

// Controller (gamepad) task — pinned to core 0
static void controller_task(void *arg) {
    while (1) {
        controller_poll();
        vTaskDelay(pdMS_TO_TICKS(20));   // 50 Hz
    }
}

// Telemetry task — pinned to core 0
static void telemetry_task(void *arg) {
    while (1) {
        telemetry_send();
        telemetry_receive_pid_updates();
        vTaskDelay(pdMS_TO_TICKS(20));   // 50 Hz
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "=== ESP32 Quadcopter booting ===");

    // NVS required by WiFi driver
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // Create mutexes before any task can run
    imu_mutex      = xSemaphoreCreateMutex();
    setpoint_mutex = xSemaphoreCreateMutex();

    // Initialize hardware in dependency order
    ESP_ERROR_CHECK(mpu6050_init());
    ESP_LOGI(TAG, "MPU-6050 OK");

    ESP_ERROR_CHECK(esc_init());
    esc_arm();
    ESP_LOGI(TAG, "ESCs armed");

    controller_init();
    ESP_LOGI(TAG, "Bluepad32 controller init OK");

    wifi_server_init();
    telemetry_init();
    ESP_LOGI(TAG, "WiFi + telemetry OK");

    // Start tasks
    xTaskCreatePinnedToCore(imu_task,         "imu",        4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(flight_ctrl_task, "fc",         4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(controller_task,  "ctrl",       3072, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(telemetry_task,   "telemetry",  4096, NULL, 2, NULL, 0);

    ESP_LOGI(TAG, "All tasks started");
}
