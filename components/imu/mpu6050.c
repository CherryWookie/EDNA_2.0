// =============================================================================
// mpu6050.c
// MPU-6050 I2C driver for ESP-IDF.
// =============================================================================

#include "mpu6050.h"
#include "../../main/config.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "MPU6050";

// ---------------------------------------------------------------------------
// MPU-6050 register map (subset used here)
// ---------------------------------------------------------------------------
#define MPU_REG_PWR_MGMT_1   0x6B
#define MPU_REG_SMPLRT_DIV   0x19
#define MPU_REG_CONFIG       0x1A
#define MPU_REG_GYRO_CONFIG  0x1B
#define MPU_REG_ACCEL_CONFIG 0x1C
#define MPU_REG_ACCEL_XOUT_H 0x3B   // First of 14 consecutive data bytes
#define MPU_REG_TEMP_OUT_H   0x41
#define MPU_REG_GYRO_XOUT_H  0x43
#define MPU_REG_WHO_AM_I     0x75

// Full-scale ranges and their LSB sensitivities
#define GYRO_FS_250_DPS      0x00   // ±250 °/s
#define ACCEL_FS_2G          0x00   // ±2 g
#define GYRO_SENSITIVITY     131.0f  // LSB / (°/s) for ±250
#define ACCEL_SENSITIVITY    16384.0f // LSB / g for ±2g

#define I2C_TIMEOUT_MS       50

// ---------------------------------------------------------------------------
// Low-level I2C helpers
// ---------------------------------------------------------------------------
static esp_err_t i2c_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return i2c_master_write_to_device(IMU_I2C_PORT, IMU_I2C_ADDR,
                                      buf, 2,
                                      pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}

static esp_err_t i2c_read_regs(uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_write_read_device(IMU_I2C_PORT, IMU_I2C_ADDR,
                                        &reg, 1,
                                        data, len,
                                        pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
esp_err_t mpu6050_init(void) {
    // Configure I2C master
    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = IMU_SDA_GPIO,
        .scl_io_num       = IMU_SCL_GPIO,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = IMU_I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(IMU_I2C_PORT, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(IMU_I2C_PORT, I2C_MODE_MASTER, 0, 0, 0));

    // Verify chip identity
    uint8_t who_am_i;
    esp_err_t err = i2c_read_regs(MPU_REG_WHO_AM_I, &who_am_i, 1);
    if (err != ESP_OK || who_am_i != 0x68) {
        ESP_LOGE(TAG, "WHO_AM_I mismatch: got 0x%02X (expected 0x68)", who_am_i);
        return ESP_FAIL;
    }

    // Wake from sleep (POR leaves SLEEP bit set), use internal 8 MHz oscillator
    ESP_ERROR_CHECK(i2c_write_reg(MPU_REG_PWR_MGMT_1, 0x00));
    vTaskDelay(pdMS_TO_TICKS(100));

    // Sample rate = 8kHz / (1 + SMPLRT_DIV) — set to 1kHz (div=7)
    ESP_ERROR_CHECK(i2c_write_reg(MPU_REG_SMPLRT_DIV, 0x07));

    // DLPF config — 44 Hz bandwidth (good noise rejection, ~4.9 ms delay)
    ESP_ERROR_CHECK(i2c_write_reg(MPU_REG_CONFIG, 0x03));

    // Gyro ±250 °/s
    ESP_ERROR_CHECK(i2c_write_reg(MPU_REG_GYRO_CONFIG, GYRO_FS_250_DPS));

    // Accel ±2 g
    ESP_ERROR_CHECK(i2c_write_reg(MPU_REG_ACCEL_CONFIG, ACCEL_FS_2G));

    ESP_LOGI(TAG, "MPU-6050 initialized");
    return ESP_OK;
}

esp_err_t mpu6050_read(mpu6050_data_t *out) {
    uint8_t raw[14];
    // Burst-read all 14 bytes starting at ACCEL_XOUT_H
    esp_err_t err = i2c_read_regs(MPU_REG_ACCEL_XOUT_H, raw, 14);
    if (err != ESP_OK) return err;

    // Parse raw big-endian int16 values
    int16_t ax_raw = (int16_t)((raw[0]  << 8) | raw[1]);
    int16_t ay_raw = (int16_t)((raw[2]  << 8) | raw[3]);
    int16_t az_raw = (int16_t)((raw[4]  << 8) | raw[5]);
    // raw[6..7] = temperature (parsed separately)
    int16_t gx_raw = (int16_t)((raw[8]  << 8) | raw[9]);
    int16_t gy_raw = (int16_t)((raw[10] << 8) | raw[11]);
    int16_t gz_raw = (int16_t)((raw[12] << 8) | raw[13]);

    // Convert to physical units (sensor frame)
    float ax_sf = ax_raw / ACCEL_SENSITIVITY;
    float ay_sf = ay_raw / ACCEL_SENSITIVITY;
    float az_sf = az_raw / ACCEL_SENSITIVITY;
    float gx_sf = gx_raw / GYRO_SENSITIVITY;
    float gy_sf = gy_raw / GYRO_SENSITIVITY;
    float gz_sf = gz_raw / GYRO_SENSITIVITY;

    // -------------------------------------------------------------------------
    // Axis remapping for physical drone mounting:
    //   Sensor +X → drone DOWN  → aviation -Z  (we don't use Z accel for angles)
    //   Sensor +Y → drone BACK  → aviation -Y
    //   Sensor +Z → drone LEFT  → aviation -X
    //
    // For angle estimation we need:
    //   aviation X (forward) = -sensor_Z
    //   aviation Y (right)   = -sensor_Y
    //   aviation Z (up)      = -sensor_X
    //
    // For gyro rates (roll=X axis, pitch=Y axis, yaw=Z axis):
    //   roll rate  = -gz_sf
    //   pitch rate = -gy_sf
    //   yaw rate   = -gx_sf    (sign may need flip — verify on bench)
    // -------------------------------------------------------------------------
    out->accel_x_g = -az_sf;   // aviation forward
    out->accel_y_g = -ay_sf;   // aviation right
    out->accel_z_g = -ax_sf;   // aviation up

    out->gyro_x_dps = -gz_sf;  // roll rate
    out->gyro_y_dps = -gy_sf;  // pitch rate
    out->gyro_z_dps = -gx_sf;  // yaw rate

    // Accel-derived angles (degrees) — atan2 is more stable than asin
    out->accel_angle_pitch = atan2f(-out->accel_x_g,
                                     sqrtf(out->accel_y_g * out->accel_y_g +
                                           out->accel_z_g * out->accel_z_g))
                             * (180.0f / M_PI);

    out->accel_angle_roll  = atan2f(out->accel_y_g, out->accel_z_g)
                             * (180.0f / M_PI);

    return ESP_OK;
}

esp_err_t mpu6050_read_temp(float *temp_c) {
    uint8_t raw[2];
    esp_err_t err = i2c_read_regs(MPU_REG_TEMP_OUT_H, raw, 2);
    if (err != ESP_OK) return err;
    int16_t t_raw = (int16_t)((raw[0] << 8) | raw[1]);
    *temp_c = (t_raw / 340.0f) + 36.53f;
    return ESP_OK;
}
