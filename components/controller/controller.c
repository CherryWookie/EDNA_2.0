// =============================================================================
// controller.c
// Bluepad32 gamepad integration.
//
// Bluepad32 uses callbacks registered at init time. When a gamepad sends data
// the onData callback fires, normalizes the axes, and writes the results into
// the shared globals declared in main.c (protected by setpoint_mutex).
//
// NOTE: Bluepad32's uni_platform_unijoysticle or uni_platform_airlift may
// need to be selected in sdkconfig depending on your Bluepad32 version.
// This file targets the uni_bt (Bluetooth classic + BLE) platform.
// =============================================================================

#include "controller.h"
#include "../../main/config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <math.h>

// Bluepad32 public header — provided by Libraries/bluepad32
#include "bluepad32.h"

static const char *TAG = "CTRL";
static int s_connected = 0;

// Shared globals declared in main.c
extern volatile int   g_throttle;
extern volatile float g_pitch_sp;
extern volatile float g_roll_sp;
extern volatile float g_yaw_sp;
extern SemaphoreHandle_t setpoint_mutex;

// ---------------------------------------------------------------------------
// Axis normalization helpers
// ---------------------------------------------------------------------------
static float apply_deadband(int raw) {
    if (raw > -STICK_DEADBAND && raw < STICK_DEADBAND) return 0.0f;
    return (float)raw;
}

// Map throttle axis (left stick Y, typically inverted: up = negative)
// from raw [-512..511] to PWM [ESC_PWM_MIN_US..ESC_PWM_MAX_US]
static int normalize_throttle(int raw) {
    // Left stick Y is usually -512 at full up; negate so up = higher throttle
    float val = apply_deadband(-raw);
    if (val < 0.0f) val = 0.0f;
    // Scale 0..512 → 1000..2000
    float scaled = (val / (float)STICK_MAX) * (float)THROTTLE_SCALE + ESC_PWM_MIN_US;
    if (scaled < ESC_PWM_MIN_US) scaled = ESC_PWM_MIN_US;
    if (scaled > ESC_PWM_MAX_US) scaled = ESC_PWM_MAX_US;
    return (int)scaled;
}

// Map angle axis (right stick) from raw [-512..511] to ±ANGLE_SETPOINT_MAX
static float normalize_angle(int raw) {
    float val = apply_deadband(raw);
    return (val / (float)STICK_MAX) * ANGLE_SETPOINT_MAX;
}

// Map yaw axis (left stick X) to ±YAW_RATE_MAX deg/s
static float normalize_yaw(int raw) {
    float val = apply_deadband(raw);
    return (val / (float)STICK_MAX) * YAW_RATE_MAX;
}

// ---------------------------------------------------------------------------
// Bluepad32 callbacks
// ---------------------------------------------------------------------------
static void on_connected(uni_hid_device_t *d) {
    s_connected = 1;
    ESP_LOGI(TAG, "Gamepad connected: %s", d->name);
}

static void on_disconnected(uni_hid_device_t *d) {
    s_connected = 0;
    ESP_LOGW(TAG, "Gamepad disconnected");
    // Safe the drone — zero setpoints and kill throttle
    if (xSemaphoreTake(setpoint_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        g_throttle = ESC_PWM_ARM_US;
        g_pitch_sp = 0.0f;
        g_roll_sp  = 0.0f;
        g_yaw_sp   = 0.0f;
        xSemaphoreGive(setpoint_mutex);
    }
}

static void on_gamepad_data(uni_hid_device_t *d, uni_gamepad_t *gp) {
    // Axis mapping:
    //   axis[0] = left  stick X  → yaw
    //   axis[1] = left  stick Y  → throttle (inverted)
    //   axis[2] = right stick X  → roll
    //   axis[3] = right stick Y  → pitch (inverted: up = nose up = positive)
    int thr   = normalize_throttle(gp->axis[1]);
    float pit = normalize_angle(-gp->axis[3]);  // negate: stick up = nose up
    float rol = normalize_angle(gp->axis[2]);
    float yaw = normalize_yaw(gp->axis[0]);

    if (xSemaphoreTake(setpoint_mutex, 0) == pdTRUE) {
        g_throttle = thr;
        g_pitch_sp = pit;
        g_roll_sp  = rol;
        g_yaw_sp   = yaw;
        xSemaphoreGive(setpoint_mutex);
    }

#if DEBUG_CONTROLLER
    ESP_LOGI(TAG, "thr:%d pitch_sp:%.1f roll_sp:%.1f yaw_sp:%.1f",
             thr, pit, rol, yaw);
#endif
}

static uni_platform_t s_platform = {
    .name            = "ESP32Drone",
    .on_init_complete = NULL,
    .on_device_connected    = on_connected,
    .on_device_disconnected = on_disconnected,
    .on_gamepad_data        = on_gamepad_data,
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void controller_init(void) {
    uni_platform_set(&s_platform);
    uni_init(0, NULL);
    ESP_LOGI(TAG, "Bluepad32 initialized — waiting for gamepad connection");
}

void controller_poll(void) {
    // Bluepad32 is event-driven via callbacks; polling the task loop
    // gives FreeRTOS time to process the BT stack events.
    // No explicit action needed here — the callbacks fire automatically.
    vTaskDelay(pdMS_TO_TICKS(1));
}

int controller_is_connected(void) {
    return s_connected;
}
