// // =============================================================================
// // controller.c
// // Bluepad32 gamepad integration.
// //
// // Bluepad32 uses callbacks registered at init time. When a gamepad sends data
// // the onData callback fires, normalizes the axes, and writes the results into
// // the shared globals declared in main.c (protected by setpoint_mutex).
// //
// // NOTE: Bluepad32's uni_platform_unijoysticle or uni_platform_airlift may
// // need to be selected in sdkconfig depending on your Bluepad32 version.
// // This file targets the uni_bt (Bluetooth classic + BLE) platform.
// // =============================================================================

// #include "controller.h"
// #include "config.h"
// #include "esp_log.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/semphr.h"
// #include <math.h>

// // Bluepad32 public header — provided by Libraries/bluepad32
// #include "uni.h"
// #include "uni_init.h"
// #include "platform/uni_platform.h"
// #include "platform/uni_platform_custom.h"
// #include "controller/uni_gamepad.h"
// #include "uni_hid_device.h"


// static const char *TAG = "CTRL";
// static int s_connected = 0;

// // Shared globals declared in main.c
// extern volatile int   g_throttle;
// extern volatile float g_pitch_sp;
// extern volatile float g_roll_sp;
// extern volatile float g_yaw_sp;
// extern SemaphoreHandle_t setpoint_mutex;

// // ---------------------------------------------------------------------------
// // Axis normalization helpers
// // ---------------------------------------------------------------------------
// static float apply_deadband(int raw) {
//     if (raw > -STICK_DEADBAND && raw < STICK_DEADBAND) return 0.0f;
//     return (float)raw;
// }

// // Map throttle axis (left stick Y, typically inverted: up = negative)
// // from raw [-512..511] to PWM [ESC_PWM_MIN_US..ESC_PWM_MAX_US]
// static int normalize_throttle(int raw) {
//     // Left stick Y is usually -512 at full up; negate so up = higher throttle
//     float val = apply_deadband(-raw);
//     if (val < 0.0f) val = 0.0f;
//     // Scale 0..512 → 1000..2000
//     float scaled = (val / (float)STICK_MAX) * (float)THROTTLE_SCALE + ESC_PWM_MIN_US;
//     if (scaled < ESC_PWM_MIN_US) scaled = ESC_PWM_MIN_US;
//     if (scaled > ESC_PWM_MAX_US) scaled = ESC_PWM_MAX_US;
//     return (int)scaled;
// }

// // Map angle axis (right stick) from raw [-512..511] to ±ANGLE_SETPOINT_MAX
// static float normalize_angle(int raw) {
//     float val = apply_deadband(raw);
//     return (val / (float)STICK_MAX) * ANGLE_SETPOINT_MAX;
// }

// // Map yaw axis (left stick X) to ±YAW_RATE_MAX deg/s
// static float normalize_yaw(int raw) {
//     float val = apply_deadband(raw);
//     return (val / (float)STICK_MAX) * YAW_RATE_MAX;
// }

// // ---------------------------------------------------------------------------
// // Bluepad32 callbacks
// // ---------------------------------------------------------------------------
// static void on_connected(uni_hid_device_t *d) {
//     s_connected = 1;
//     ESP_LOGI(TAG, "Gamepad connected: %s", d->name);
// }

// static void on_disconnected(uni_hid_device_t *d) {
//     s_connected = 0;
//     ESP_LOGW(TAG, "Gamepad disconnected");
//     // Safe the drone — zero setpoints and kill throttle
//     if (xSemaphoreTake(setpoint_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
//         g_throttle = ESC_PWM_ARM_US;
//         g_pitch_sp = 0.0f;
//         g_roll_sp  = 0.0f;
//         g_yaw_sp   = 0.0f;
//         xSemaphoreGive(setpoint_mutex);
//     }
// }

// // static void on_gamepad_data(uni_hid_device_t *d, uni_gamepad_t *gp) {

// static void on_gamepad_data(uni_hid_device_t *d, uni_controller_t *ctl) {
//     uni_gamepad_t *gp = &ctl->gamepad;   
//     // Axis mapping:
//     //   axis[0] = left  stick X  → yaw
//     //   axis[1] = left  stick Y  → throttle (inverted)
//     //   axis[2] = right stick X  → roll
//     //   axis[3] = right stick Y  → pitch (inverted: up = nose up = positive)
//     int thr   = normalize_throttle(gp->axis_y);
//     float pit = normalize_angle(-gp->axis_ry);  // negate: stick up = nose up
//     float rol = normalize_angle(gp->axis_rx);
//     float yaw = normalize_yaw(gp->axis_x);

//     if (xSemaphoreTake(setpoint_mutex, 0) == pdTRUE) {
//         g_throttle = thr;
//         g_pitch_sp = pit;
//         g_roll_sp  = rol;
//         g_yaw_sp   = yaw;
//         xSemaphoreGive(setpoint_mutex);
//     }

// #if DEBUG_CONTROLLER
//     ESP_LOGI(TAG, "thr:%d pitch_sp:%.1f roll_sp:%.1f yaw_sp:%.1f",
//              thr, pit, rol, yaw);
// #endif
// }

// // BEGIN - Added to make sure bluepad32 doesn't keep rebooting
// static void platform_init(int argc, const char** argv) {
//     (void)argc;
//     (void)argv;
// }

// static void platform_on_init_complete(void) {
//     uni_bt_enable_new_connections_unsafe(true);
// }

// static uni_error_t platform_on_device_ready(uni_hid_device_t* d) {
//     (void)d;
//     return UNI_ERROR_SUCCESS;
// }

// static void platform_on_oob_event(uni_platform_oob_event_t event, void* data) {
//     (void)event;
//     (void)data;
// }
// // END

// // static struct uni_platform s_platform = {
// //     .name            = "ESP32Drone",
// //     .on_init_complete = NULL,
// //     .on_device_connected    = on_connected,
// //     .on_device_disconnected = on_disconnected,
// //     .on_gamepad_data        = on_gamepad_data,
// // };

// static struct uni_platform s_platform = {
//     .name                   = "ESP32Drone",
//     .init                   = platform_init,
//     .on_init_complete       = platform_on_init_complete,
//     .on_device_discovered   = NULL,
//     .on_device_connected    = on_connected,
//     .on_device_disconnected = on_disconnected,
//     .on_device_ready        = platform_on_device_ready,
//     .on_controller_data     = on_gamepad_data,
//     .on_gamepad_data        = NULL,
//     .get_property           = NULL,
//     .on_oob_event           = platform_on_oob_event,
//     .device_dump            = NULL,
//     .register_console_cmds  = NULL,
// };
// // ---------------------------------------------------------------------------
// // Public API
// // ---------------------------------------------------------------------------
// void controller_init(void) {
//     uni_platform_set_custom(&s_platform);
//     uni_init(0, NULL);
//     ESP_LOGI(TAG, "Bluepad32 initialized — waiting for gamepad connection");
// }

// void controller_poll(void) {
//     // Bluepad32 is event-driven via callbacks; polling the task loop
//     // gives FreeRTOS time to process the BT stack events.
//     // No explicit action needed here — the callbacks fire automatically.
//     vTaskDelay(pdMS_TO_TICKS(1));
// }

// int controller_is_connected(void) {
//     return s_connected;
// }


#include "controller.h"
#include "config.h"
#include "esc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <math.h>

#include "uni.h"
#include "uni_init.h"
#include "platform/uni_platform.h"
#include "platform/uni_platform_custom.h"
#include "controller/uni_gamepad.h"
#include "uni_hid_device.h"

static const char *TAG = "CTRL";
static int s_connected = 0;

// Persistent throttle state — accumulates over time
static int s_throttle_us = ESC_PWM_SPIN_US;

// Shared globals declared in main.c
extern volatile int   g_throttle;
extern volatile float g_pitch_sp;
extern volatile float g_roll_sp;
extern volatile float g_yaw_sp;
extern volatile int   g_motors_killed;   // ADD this extern
extern SemaphoreHandle_t setpoint_mutex;

// ---------------------------------------------------------------------------
// Axis normalization helpers
// ---------------------------------------------------------------------------
static float apply_deadband(int raw) {
    if (raw > -STICK_DEADBAND && raw < STICK_DEADBAND) return 0.0f;
    return (float)raw;
}

// Accumulative throttle — stick deflection controls rate of change
// raw: left stick Y (Bluepad32: up = negative, so we negate)
static int accumulate_throttle(int raw) {
    // Negate: up = positive deflection
    float val = apply_deadband(-raw);

    if (val == 0.0f) {
        // Stick centered — hold current value
        return s_throttle_us;
    }

    // Scale deflection (0..512) to a rate (MIN..MAX µs per callback)
    float fraction = fabsf(val) / (float)STICK_MAX;  // 0.0 to 1.0
    float rate = THROTTLE_ACCEL_MIN_US_PER_CALL +
                 fraction * (THROTTLE_ACCEL_MAX_US_PER_CALL - THROTTLE_ACCEL_MIN_US_PER_CALL);

    if (val > 0.0f) {
        s_throttle_us += (int)rate;
    } else {
        s_throttle_us -= (int)rate;
    }

    // Clamp
    if (s_throttle_us < ESC_PWM_MIN_US) s_throttle_us = ESC_PWM_MIN_US;
    if (s_throttle_us > ESC_PWM_FLIGHT_MAX_US) s_throttle_us = ESC_PWM_FLIGHT_MAX_US;

    return s_throttle_us;
}

static float normalize_angle(int raw) {
    float val = apply_deadband(raw);
    return (val / (float)STICK_MAX) * ANGLE_SETPOINT_MAX;
}

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
    // Safe the drone on disconnect
    s_throttle_us = ESC_PWM_ARM_US;
    if (xSemaphoreTake(setpoint_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        g_throttle      = ESC_PWM_ARM_US;
        g_pitch_sp      = 0.0f;
        g_roll_sp       = 0.0f;
        g_yaw_sp        = 0.0f;
        g_motors_killed = 1;
        xSemaphoreGive(setpoint_mutex);
    }
}

static void on_gamepad_data(uni_hid_device_t *d, uni_controller_t *ctl) {
    uni_gamepad_t *gp = &ctl->gamepad;

    // X button (BUTTON_X on Xbox = 0x0008) — failsafe kill
    // Reset throttle accumulator too so it doesn't resume mid-value
    if (gp->buttons & BUTTON_X) {
        s_throttle_us = ESC_PWM_ARM_US;
        if (xSemaphoreTake(setpoint_mutex, 0) == pdTRUE) {
            g_motors_killed = 1;
            g_throttle      = ESC_PWM_ARM_US;
            g_pitch_sp      = 0.0f;
            g_roll_sp       = 0.0f;
            g_yaw_sp        = 0.0f;
            xSemaphoreGive(setpoint_mutex);
        }
        ESP_LOGW(TAG, "FAILSAFE — X button pressed, motors killed");
        return;  // skip normal update this frame
    }

    int   thr = accumulate_throttle(gp->axis_y);
    float pit = normalize_angle(-gp->axis_ry);
    float rol = normalize_angle(gp->axis_rx);
    float yaw = normalize_yaw(gp->axis_x);

    if (xSemaphoreTake(setpoint_mutex, 0) == pdTRUE) {
        g_throttle      = thr;
        g_pitch_sp      = pit;
        g_roll_sp       = rol;
        g_yaw_sp        = yaw;
        g_motors_killed = 0;   // clear kill flag on normal input
        xSemaphoreGive(setpoint_mutex);
    }

#if DEBUG_CONTROLLER
    ESP_LOGI(TAG, "thr:%d pitch_sp:%.1f roll_sp:%.1f yaw_sp:%.1f", thr, pit, rol, yaw);
#endif
}

// ---------------------------------------------------------------------------
// Bluepad32 platform callbacks
// ---------------------------------------------------------------------------
static void platform_init(int argc, const char** argv) {
    (void)argc;
    (void)argv;
}

static void platform_on_init_complete(void) {
    uni_bt_enable_new_connections_unsafe(true);
}

static uni_error_t platform_on_device_ready(uni_hid_device_t* d) {
    (void)d;
    return UNI_ERROR_SUCCESS;
}

static void platform_on_oob_event(uni_platform_oob_event_t event, void* data) {
    (void)event;
    (void)data;
}

static struct uni_platform s_platform = {
    .name                   = "ESP32Drone",
    .init                   = platform_init,
    .on_init_complete       = platform_on_init_complete,
    .on_device_discovered   = NULL,
    .on_device_connected    = on_connected,
    .on_device_disconnected = on_disconnected,
    .on_device_ready        = platform_on_device_ready,
    .on_controller_data     = on_gamepad_data,
    .on_gamepad_data        = NULL,
    .get_property           = NULL,
    .on_oob_event           = platform_on_oob_event,
    .device_dump            = NULL,
    .register_console_cmds  = NULL,
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void controller_init(void) {
    uni_platform_set_custom(&s_platform);
    uni_init(0, NULL);
    ESP_LOGI(TAG, "Bluepad32 initialized — waiting for gamepad connection");
}

void controller_poll(void) {
    vTaskDelay(pdMS_TO_TICKS(1));
}

int controller_is_connected(void) {
    return s_connected;
}