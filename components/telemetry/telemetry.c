// =============================================================================
// telemetry.c
// Sends JSON telemetry to QTC console. Receives PID gain updates.
//
// Outgoing JSON example:
// {"pitch":2.34,"roll":-1.12,"yaw_rate":0.5,
//  "pitch_sp":0.0,"roll_sp":0.0,"yaw_sp":0.0,
//  "pitch_out":12.3,"roll_out":-8.7,"yaw_out":0.0,
//  "m_fl":1150,"m_fr":1148,"m_rl":1145,"m_rr":1152,
//  "throttle":1200,
//  "kp_pitch":1.2,"ki_pitch":0.01,"kd_pitch":0.08,
//  "kp_roll":1.2,"ki_roll":0.01,"kd_roll":0.08}
//
// Incoming JSON example:
//   {"axis":"pitch","kp":1.5,"ki":0.01,"kd":0.1}
//   {"axis":"roll", "kp":1.5,"ki":0.01,"kd":0.1}
//   {"axis":"yaw",  "kp":2.0,"ki":0.05,"kd":0.0}
// =============================================================================

#include "telemetry.h"
#include "wifi_server.h"
#include "config.h"
#include "flight_controller.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "TELEM";

// Shared globals from main.c
extern volatile float g_pitch_deg;
extern volatile float g_roll_deg;
extern volatile float g_yaw_rate;
extern volatile int   g_throttle;
extern volatile float g_pitch_sp;
extern volatile float g_roll_sp;
extern volatile float g_yaw_sp;
extern SemaphoreHandle_t imu_mutex;
extern SemaphoreHandle_t setpoint_mutex;

// Cached last motor values so telemetry can report them
// (set by a tiny shim — see telemetry_set_motors below)
static uint32_t s_m_fl = 1000, s_m_fr = 1000, s_m_rl = 1000, s_m_rr = 1000;
static float    s_pitch_out = 0, s_roll_out = 0, s_yaw_out = 0;

// Allow the flight controller to report its last outputs for telemetry
void telemetry_set_motor_values(uint32_t fl, uint32_t fr, uint32_t rl, uint32_t rr) {
    s_m_fl = fl; s_m_fr = fr; s_m_rl = rl; s_m_rr = rr;
}
void telemetry_set_pid_outputs(float pitch, float roll, float yaw) {
    s_pitch_out = pitch; s_roll_out = roll; s_yaw_out = yaw;
}

static struct sockaddr_in s_client;
static bool s_client_set = false;

void telemetry_init(void) {
    ESP_LOGI(TAG, "Telemetry ready. Waiting for QTC to connect on port %d", TELEMETRY_PORT);
}

void telemetry_send(void) {
    int sock = wifi_server_get_sock();
    if (sock < 0 || !s_client_set) return;

    // Snapshot shared state
    float pitch, roll, yaw_rate;
    int   throttle;
    float pitch_sp, roll_sp, yaw_sp;

    if (xSemaphoreTake(imu_mutex, pdMS_TO_TICKS(2)) == pdTRUE) {
        pitch    = g_pitch_deg;
        roll     = g_roll_deg;
        yaw_rate = g_yaw_rate;
        xSemaphoreGive(imu_mutex);
    } else return;

    if (xSemaphoreTake(setpoint_mutex, pdMS_TO_TICKS(2)) == pdTRUE) {
        throttle = g_throttle;
        pitch_sp = g_pitch_sp;
        roll_sp  = g_roll_sp;
        yaw_sp   = g_yaw_sp;
        xSemaphoreGive(setpoint_mutex);
    } else return;

    pid_state_t *pp = flight_controller_get_pid_pitch();
    pid_state_t *pr = flight_controller_get_pid_roll();
    pid_state_t *py = flight_controller_get_pid_yaw();

    char buf[512];
    int len = snprintf(buf, sizeof(buf),
        "{\"pitch\":%.2f,\"roll\":%.2f,\"yaw_rate\":%.2f,"
        "\"pitch_sp\":%.2f,\"roll_sp\":%.2f,\"yaw_sp\":%.2f,"
        "\"pitch_out\":%.2f,\"roll_out\":%.2f,\"yaw_out\":%.2f,"
        "\"m_fl\":%lu,\"m_fr\":%lu,\"m_rl\":%lu,\"m_rr\":%lu,"
        "\"throttle\":%d,"
        "\"kp_pitch\":%.4f,\"ki_pitch\":%.4f,\"kd_pitch\":%.4f,"
        "\"kp_roll\":%.4f,\"ki_roll\":%.4f,\"kd_roll\":%.4f,"
        "\"kp_yaw\":%.4f,\"ki_yaw\":%.4f,\"kd_yaw\":%.4f}\n",
        pitch, roll, yaw_rate,
        pitch_sp, roll_sp, yaw_sp,
        s_pitch_out, s_roll_out, s_yaw_out,
        s_m_fl, s_m_fr, s_m_rl, s_m_rr,
        throttle,
        pp->kp, pp->ki, pp->kd,
        pr->kp, pr->ki, pr->kd,
        py->kp, py->ki, py->kd);

    sendto(sock, buf, len, 0,
           (struct sockaddr *)&s_client, sizeof(s_client));
}

// Minimal JSON field extractor — avoids pulling in cJSON to save RAM.
// Finds "key":value and returns the float. Returns default_val if not found.
static float json_get_float(const char *json, const char *key, float default_val) {
    char search[32];
    snprintf(search, sizeof(search), "\"%s\":", key);
    const char *p = strstr(json, search);
    if (!p) return default_val;
    p += strlen(search);
    // Skip whitespace
    while (*p == ' ' || *p == '\t') p++;
    char *end;
    float val = strtof(p, &end);
    if (end == p) return default_val;
    return val;
}

static void json_get_str(const char *json, const char *key, char *out, int outlen) {
    char search[32];
    snprintf(search, sizeof(search), "\"%s\":\"", key);
    const char *p = strstr(json, search);
    if (!p) { out[0] = '\0'; return; }
    p += strlen(search);
    int i = 0;
    while (*p && *p != '"' && i < outlen - 1) out[i++] = *p++;
    out[i] = '\0';
}

void telemetry_receive_pid_updates(void) {
    int sock = wifi_server_get_sock();
    if (sock < 0) return;

    char buf[256];
    struct sockaddr_in src;
    socklen_t srclen = sizeof(src);

    int n = recvfrom(sock, buf, sizeof(buf) - 1, 0,
                     (struct sockaddr *)&src, &srclen);
    if (n <= 0) return;   // Non-blocking: EAGAIN means no data

    buf[n] = '\0';

    // Learn the client address so telemetry_send() knows where to reply
    if (!s_client_set) {
        s_client     = src;
        s_client_set = true;
        ESP_LOGI(TAG, "QTC client registered");
    }

    // Parse axis and new gains
    char axis[16];
    json_get_str(buf, "axis", axis, sizeof(axis));
    float kp = json_get_float(buf, "kp", -1.0f);
    float ki = json_get_float(buf, "ki", -1.0f);
    float kd = json_get_float(buf, "kd", -1.0f);

    if (kp < 0 || ki < 0 || kd < 0) {
        ESP_LOGW(TAG, "Malformed PID packet: %s", buf);
        return;
    }

    pid_state_t *target = NULL;
    if      (strcmp(axis, "pitch") == 0) target = flight_controller_get_pid_pitch();
    else if (strcmp(axis, "roll")  == 0) target = flight_controller_get_pid_roll();
    else if (strcmp(axis, "yaw")   == 0) target = flight_controller_get_pid_yaw();

    if (target) {
        pid_set_gains(target, kp, ki, kd);
        ESP_LOGI(TAG, "Updated %s PID: Kp=%.4f Ki=%.4f Kd=%.4f", axis, kp, ki, kd);
    } else {
        ESP_LOGW(TAG, "Unknown axis in PID packet: %s", axis);
    }
}
