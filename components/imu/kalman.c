// =============================================================================
// kalman.c
// 1-D Kalman filter — angle + gyro bias estimation.
// Based on Lauszus/KalmanFilter, adapted for ESP-IDF.
// =============================================================================

#include "kalman.h"

void kalman_init(kalman_state_t *k) {
    k->angle   = 0.0f;
    k->bias    = 0.0f;
    k->P[0][0] = 0.0f;
    k->P[0][1] = 0.0f;
    k->P[1][0] = 0.0f;
    k->P[1][1] = 0.0f;
    k->Q_angle   = 0.001f;
    k->Q_bias    = 0.003f;
    k->R_measure = 0.03f;
}

void kalman_set_params(kalman_state_t *k, float Q_angle, float Q_bias, float R_measure) {
    k->Q_angle   = Q_angle;
    k->Q_bias    = Q_bias;
    k->R_measure = R_measure;
}

float kalman_update(kalman_state_t *k, float accel_angle, float gyro_rate, float dt) {
    // ---- Predict ----
    // Project state forward using gyro rate minus current bias estimate
    k->angle += dt * (gyro_rate - k->bias);

    // Project error covariance forward
    k->P[0][0] += dt * (dt * k->P[1][1] - k->P[0][1] - k->P[1][0] + k->Q_angle);
    k->P[0][1] -= dt * k->P[1][1];
    k->P[1][0] -= dt * k->P[1][1];
    k->P[1][1] += k->Q_bias * dt;

    // ---- Update ----
    // Compute innovation (difference between measurement and prediction)
    float innovation = accel_angle - k->angle;

    // Innovation covariance
    float S = k->P[0][0] + k->R_measure;

    // Kalman gain
    float K[2];
    K[0] = k->P[0][0] / S;
    K[1] = k->P[1][0] / S;

    // Update state estimate
    k->angle += K[0] * innovation;
    k->bias  += K[1] * innovation;

    // Update error covariance
    float P00_tmp = k->P[0][0];
    float P01_tmp = k->P[0][1];

    k->P[0][0] -= K[0] * P00_tmp;
    k->P[0][1] -= K[0] * P01_tmp;
    k->P[1][0] -= K[1] * P00_tmp;
    k->P[1][1] -= K[1] * P01_tmp;

    return k->angle;
}
