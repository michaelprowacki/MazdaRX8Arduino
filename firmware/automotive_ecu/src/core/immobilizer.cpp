/**
 * @file immobilizer.cpp
 * @brief Immobilizer bypass implementation
 *
 * Two-part handshake protocol:
 * 1. KCM sends: [?, 127, 2, ...] on 0x047 → ECU responds with ResponseA on 0x041
 * 2. KCM sends: [?, 92, 244, ...] on 0x047 → ECU responds with ResponseB on 0x041
 *
 * After both handshakes complete, vehicle is unlocked.
 *
 * @author Ported from legacy RX8_CANBUS.ino
 * @date 2025-11-16
 */

#include "immobilizer.h"
#include "can_controller.h"
#include "../shared/RX8_CAN_Protocol/can_messages.h"
#include <Arduino.h>
#include "../../config/vehicle_config.h"

namespace Immobilizer {

// State tracking
static bool unlocked = false;
static uint8_t handshake_count = 0;
static uint32_t last_request_time = 0;

// Response buffers
static uint8_t response_a[8] = {7, 12, 48, 242, 23, 0, 0, 0};
static uint8_t response_b[8] = {129, 127, 0, 0, 0, 0, 0, 0};

void init() {
    unlocked = false;
    handshake_count = 0;

    #if ENABLE_SERIAL_DEBUG
        Serial.println("[IMMOB] Immobilizer bypass initialized");
        Serial.println("[IMMOB] Waiting for KCM handshake on CAN ID 0x047");
    #endif
}

void processRequest(const uint8_t* data) {
    // Check for first handshake: buf[1] == 127 && buf[2] == 2
    if (data[1] == 127 && data[2] == 2) {
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[IMMOB] Handshake 1/2 received, sending ResponseA");
        #endif

        // Send ResponseA: {7, 12, 48, 242, 23, 0, 0, 0}
        CANController::transmit(0x041, response_a, 8);
        handshake_count = 1;
        last_request_time = millis();
    }
    // Check for second handshake: buf[1] == 92 && buf[2] == 244
    else if (data[1] == 92 && data[2] == 244) {
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[IMMOB] Handshake 2/2 received, sending ResponseB");
        #endif

        // Send ResponseB: {129, 127, 0, 0, 0, 0, 0, 0}
        CANController::transmit(0x041, response_b, 8);

        handshake_count = 2;
        unlocked = true;
        last_request_time = millis();

        #if ENABLE_SERIAL_DEBUG
            Serial.println("[IMMOB] *** VEHICLE UNLOCKED ***");
        #endif
    }
}

void update() {
    // Check for timeout (if no request in 5 seconds, lock vehicle)
    #if ENABLE_SAFETY_MONITOR
        if (unlocked && (millis() - last_request_time > 5000)) {
            #if ENABLE_SERIAL_DEBUG
                Serial.println("[IMMOB] WARNING: No KCM communication for 5s, relocking");
            #endif
            unlocked = false;
            handshake_count = 0;
        }
    #endif
}

bool isUnlocked() {
    return unlocked;
}

uint8_t getHandshakeCount() {
    return handshake_count;
}

} // namespace Immobilizer
