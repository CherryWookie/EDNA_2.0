#pragma once

// =============================================================================
// kalman.h
// 1-D Kalman filter for fusing gyroscope integration with accelerometer angle.
// One instance per axis (pitch, roll).
//
// State vector: [angle, gyro_bias]
// Measurement:  accelerometer-derived angle
//
// Tune Q_angle and Q_bias to adjust filter responsiveness:
//   Higher Q_angle → trust gyro more, faster response, more noise
//   Higher R_measure → trust gyro more, smoother but slower to correct
// =============================================================================

typedef struct {
    float angle;        // Current estimated angle (degrees)
    float bias;         // Estimated gyro bias (degrees/second)

    // Error covariance matrix P[2][2]
    float P[2][2];

    // Tuning parameters
    float Q_angle;      // Process noise for angle   (default 0.001)
    float Q_bias;       // Process noise for bias    (default 0.003)
    float R_measure;    // Measurement noise         (default 0.03)
} kalman_state_t;

// Initialize with default tuning constants. Call before first update.
void kalman_init(kalman_state_t *k);

// Update filter with new gyro rate and accelerometer angle measurement.
// dt: time since last call in seconds.
// Returns the filtered angle estimate in degrees.
float kalman_update(kalman_state_t *k, float accel_angle, float gyro_rate, float dt);

// Override default tuning constants (optional — call after kalman_init)
void kalman_set_params(kalman_state_t *k, float Q_angle, float Q_bias, float R_measure);
