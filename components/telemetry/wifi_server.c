// =============================================================================
// wifi_server.c
// WiFi Access Point + UDP server initialization.
// =============================================================================

#include "wifi_server.h"
#include "config.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <string.h>
#include "esp_mac.h"

static const char *TAG = "WIFI";

static int s_sock = -1;
static struct sockaddr_in s_client_addr;
static bool s_client_known = false;

static void wifi_event_handler(void *arg, esp_event_base_t base,
                                int32_t id, void *data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *e = (wifi_event_ap_staconnected_t *)data;
        ESP_LOGI(TAG, "Client connected, MAC: %02x:%02x:%02x:%02x:%02x:%02x",
         MAC2STR(e->mac));
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *e = (wifi_event_ap_stadisconnected_t *)data;
        ESP_LOGI(TAG, "Client disconnected, MAC: %02x:%02x:%02x:%02x:%02x:%02x",
         MAC2STR(e->mac));
        s_client_known = false;
    }
}

esp_err_t wifi_server_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                               wifi_event_handler, NULL));

    wifi_config_t wifi_cfg = {
        .ap = {
            .ssid           = WIFI_SSID,
            .ssid_len       = strlen(WIFI_SSID),
            .password       = WIFI_PASSWORD,
            .channel        = WIFI_CHANNEL,
            .authmode       = WIFI_AUTH_WPA2_PSK,
            .max_connection = WIFI_MAX_CONN,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP started. SSID: %s  Password: %s", WIFI_SSID, WIFI_PASSWORD);

    // Create UDP socket
    s_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (s_sock < 0) {
        ESP_LOGE(TAG, "Failed to create UDP socket");
        return ESP_FAIL;
    }

    // Set non-blocking so telemetry_receive_pid_updates() doesn't stall the task
    int flags = fcntl(s_sock, F_GETFL, 0);
    fcntl(s_sock, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in bind_addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port        = htons(TELEMETRY_PORT),
    };
    if (bind(s_sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
        ESP_LOGE(TAG, "UDP bind failed");
        close(s_sock);
        s_sock = -1;
        return ESP_FAIL;
    }

    memset(&s_client_addr, 0, sizeof(s_client_addr));
    ESP_LOGI(TAG, "UDP server bound to port %d", TELEMETRY_PORT);
    return ESP_OK;
}

int wifi_server_get_sock(void) { return s_sock; }

struct sockaddr_in *wifi_server_get_client_addr(void) {
    return s_client_known ? &s_client_addr : NULL;
}
