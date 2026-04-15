// =============================================================================
// esc_calibration.c
// Blocking ESC calibration routine. Read the header for the full procedure.
// PROPS OFF before calling this.
// =============================================================================

#include "esc_calibration.h"
#include "esc.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "ESC_CAL";

void esc_calibrate(void) {
    ESP_LOGW(TAG, "==============================================");
    ESP_LOGW(TAG, "ESC CALIBRATION — PROPS MUST BE REMOVED");
    ESP_LOGW(TAG, "==============================================");
    ESP_LOGI(TAG, "Step 1: Sending MAX throttle (2000 µs)...");
    ESP_LOGI(TAG, "NOW PLUG IN THE BATTERY. Wait for beep sequence.");

    esc_set_all_us(ESC_PWM_MAX_US);
    vTaskDelay(pdMS_TO_TICKS(7000));    // Wait for battery plug-in + ESC beeps

    ESP_LOGI(TAG, "Step 2: Sending MIN throttle (1000 µs)...");
    esc_set_all_us(ESC_PWM_MIN_US);
    vTaskDelay(pdMS_TO_TICKS(3000));    // Wait for confirmation beeps

    ESP_LOGI(TAG, "Step 3: UNPLUG BATTERY now.");
    ESP_LOGI(TAG, "Calibration complete. On next power-up you should hear");
    ESP_LOGI(TAG, "a single synchronized 3-beep series.");

    // Return to arm position
    esc_set_all_us(ESC_PWM_ARM_US);
}
