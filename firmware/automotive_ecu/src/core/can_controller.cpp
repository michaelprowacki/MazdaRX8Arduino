/**
 * @file can_controller.cpp
 * @brief CAN bus controller implementation
 *
 * Platform-agnostic CAN controller that uses HAL for actual hardware access.
 *
 * @author Ported from legacy RX8_CANBUS.ino
 * @date 2025-11-16
 */

#include "can_controller.h"
#include "../hal/hal_interface.h"
#include <Arduino.h>

namespace CANController {

static uint32_t last_rx_time = 0;

bool init(uint32_t speed) {
    #if ENABLE_SERIAL_DEBUG
        Serial.println("[CAN] Initializing CAN controller...");
    #endif

    bool success = HAL_CAN_Init(speed);

    if (success) {
        #if ENABLE_SERIAL_DEBUG
            Serial.printf("[CAN] Initialized @ %d bps\n", speed);
        #endif
        last_rx_time = millis();
    } else {
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[CAN] ERROR: Failed to initialize!");
        #endif
    }

    return success;
}

void process() {
    // Called from main loop - process any incoming messages
    // Platform-specific implementation handles actual message reception
    if (messageAvailable()) {
        last_rx_time = millis();
    }
}

bool transmit(uint32_t id, const uint8_t* data, uint8_t len) {
    return HAL_CAN_Transmit(id, data, len);
}

bool messageAvailable() {
    return HAL_CAN_MessageAvailable();
}

bool readMessage(uint32_t* id, uint8_t* len, uint8_t* data) {
    bool success = HAL_CAN_ReadMessage(id, len, data);
    if (success) {
        last_rx_time = millis();
    }
    return success;
}

uint32_t getLastRxTime() {
    return last_rx_time;
}

} // namespace CANController
