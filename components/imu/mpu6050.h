#pragma once

// =============================================================================
// mpu6050.h
// I2C driver for the MPU-6050 6-axis IMU.
// Handles: init, wake-from-sleep, full-scale config, raw register reads,
// axis remapping for non-standard mounting orientation, and accel angle calc.
//
// Physical mounting on this drone:
//   Sensor +X → drone DOWN   (remapped to -Z in aviation frame)
//   Sensor +Y → drone BACK   (remapped to -Y)
//   Sensor +Z → drone LEFT   (remapped to -X)
// After remapping: X=forward, Y=right, Z=up (standard aviation NED-ish frame)
// =============================================================================

#include "esp_err.h"
#include <stdint.h>

// Processed IMU data — axis remapping and unit conversion already applied
typedef struct {
    // Accelerometer (g)
    float accel_x_g;
    float accel_y_g;
    float accel_z_g;

    // Gyroscope (degrees/second)
    float gyro_x_dps;    // roll rate  (after remap)
    float gyro_y_dps;    // pitch rate (after remap)
    float gyro_z_dps;    // yaw rate   (after remap)

    // Accel-derived angle estimates (degrees) — used as Kalman measurement input
    float accel_angle_pitch;
    float accel_angle_roll;
} mpu6050_data_t;

// Initialize I2C bus and MPU-6050. Call once from app_main before tasks start.
esp_err_t mpu6050_init(void);

// Read one sample. Applies axis remapping and unit conversion.
// Returns ESP_OK on success, ESP_FAIL on I2C error.
esp_err_t mpu6050_read(mpu6050_data_t *out);

// Read raw temperature (degrees C) — useful for monitoring ESC/board heat
esp_err_t mpu6050_read_temp(float *temp_c);
