#pragma once
// =============================================================================
// telemetry.h
// UDP telemetry: sends live flight data to the QTC console at 50 Hz,
// and receives PID gain update packets from the QTC.
//
// Outgoing packet (telemetry_packet_t) — JSON string for easy QTC parsing.
// Incoming packet — JSON string: {"axis":"pitch","kp":1.2,"ki":0.01,"kd":0.08}
//
// Both directions use the same UDP socket managed by wifi_server.c.
// The client address is learned from the first inbound packet.
// =============================================================================
#include "esp_err.h"
#include <stdint.h>

// Call once after wifi_server_init()
void telemetry_init(void);

// Build and send one telemetry JSON packet to the QTC client.
// Reads current IMU, PID, and motor state from shared globals.
void telemetry_send(void);

// Check for an inbound PID gain update packet (non-blocking).
// If a valid packet is received, hot-updates the named PID instance.
void telemetry_receive_pid_updates(void);

// Called by flight_controller after each update to cache latest values.
// These are read by telemetry_send() on the next 50 Hz tick.
void telemetry_set_motor_values(uint32_t fl, uint32_t fr, uint32_t rl, uint32_t rr);
void telemetry_set_pid_outputs(float pitch, float roll, float yaw);