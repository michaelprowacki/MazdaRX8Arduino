/**
 * @file wheel_speed.cpp
 * @brief Wheel speed sensor processing for RX8
 *
 * Reads wheel speeds from CAN bus (ID 0x4B1) and calculates vehicle speed.
 * Implements safety check for wheel speed mismatch (spin detection).
 *
 * @author Ported from legacy RX8_CANBUS.ino
 * @date 2025-11-16
 */

#include "wheel_speed.h"
#include "../core/can_controller.h"
#include <Arduino.h>
#include "../../config/vehicle_config.h"

namespace WheelSpeed {

// Wheel speed state (km/h * 100)
static uint16_t fl = 0;  // Front left
static uint16_t fr = 0;  // Front right
static uint16_t rl = 0;  // Rear left
static uint16_t rr = 0;  // Rear right

// Last update time
static uint32_t last_update = 0;

void init() {
    fl = fr = rl = rr = 0;
    last_update = millis();

    #if ENABLE_SERIAL_DEBUG
        Serial.println("[WHEEL] Wheel speed sensor initialized");
        Serial.println("[WHEEL] Listening for CAN ID 0x4B1");
    #endif
}

void processCANMessage(const uint8_t* data) {
    // CAN ID 0x4B1 (1201 decimal) contains wheel speeds
    // Format: [FL_H, FL_L, FR_H, FR_L, RL_H, RL_L, RR_H, RR_L]
    // Each speed is encoded as: (speed_kmh * 100) + 10000

    fl = (data[0] << 8) | data[1];  // Front left
    fr = (data[2] << 8) | data[3];  // Front right
    rl = (data[4] << 8) | data[5];  // Rear left
    rr = (data[6] << 8) | data[7];  // Rear right

    // Remove 10000 offset
    fl = (fl > 10000) ? (fl - 10000) : 0;
    fr = (fr > 10000) ? (fr - 10000) : 0;
    rl = (rl > 10000) ? (rl - 10000) : 0;
    rr = (rr > 10000) ? (rr - 10000) : 0;

    last_update = millis();
}

void read(uint16_t* fl_out, uint16_t* fr_out, uint16_t* rl_out, uint16_t* rr_out) {
    if (fl_out) *fl_out = fl;
    if (fr_out) *fr_out = fr;
    if (rl_out) *rl_out = rl;
    if (rr_out) *rr_out = rr;
}

uint16_t getVehicleSpeed(bool* mismatch_detected) {
    // Calculate average of front wheels
    uint16_t avg_front = (fl + fr) / 2;

    // Safety check: detect wheel speed mismatch (wheelspin/skid)
    // Threshold: 500 = 5 km/h difference
    int16_t diff = abs((int16_t)fl - (int16_t)fr);

    if (diff > 500) {
        #if ENABLE_SERIAL_DEBUG
            Serial.printf("[WHEEL] WARNING: Wheel speed mismatch! FL=%d, FR=%d, diff=%d\n",
                         fl, fr, diff);
        #endif

        if (mismatch_detected) {
            *mismatch_detected = true;
        }

        // Return 0 for safety
        return 0;
    }

    if (mismatch_detected) {
        *mismatch_detected = false;
    }

    // Convert from (km/h * 100) to (km/h * 10) for vehicle state
    return avg_front / 10;
}

bool isDataFresh(uint32_t timeout_ms) {
    return (millis() - last_update) < timeout_ms;
}

} // namespace WheelSpeed
