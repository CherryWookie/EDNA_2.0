#pragma once

// =============================================================================
// wifi_server.h
// Configures the ESP32 as a WiFi Access Point and opens a UDP socket.
// SSID and password are set in config.h (WIFI_SSID / WIFI_PASSWORD).
//
// After init, the drone broadcasts SSID "esp32drone". Connect your laptop
// to that network, then launch MBL_QTC.py to see live telemetry and tune PID.
//
// The UDP server binds to TELEMETRY_PORT (default 4444).
// Outgoing packets: telemetry structs at 50 Hz  (telemetry.c)
// Incoming packets: PID gain updates from QTC   (telemetry.c)
// =============================================================================

#include "esp_err.h"

// Initialize WiFi AP and create UDP socket. Returns socket fd or -1 on error.
// Stores the socket fd internally — telemetry.c retrieves it via wifi_server_get_sock().
esp_err_t wifi_server_init(void);

// Return the open UDP socket file descriptor.
int wifi_server_get_sock(void);

// Return the most recent client address (populated on first received packet).
// telemetry.c uses this to send reply packets back to the QTC console.
struct sockaddr_in *wifi_server_get_client_addr(void);
