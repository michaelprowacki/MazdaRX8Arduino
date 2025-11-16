/**
 * @file abs_dsc.cpp
 * @brief ABS/DSC system emulation for RX8
 *
 * Emulates ABS and DSC systems to prevent warning lights when
 * factory modules are removed or bypassed for engine swaps.
 *
 * @author Ported from legacy RX8_CANBUS.ino
 * @date 2025-11-16
 */

#include "abs_dsc.h"
#include "can_controller.h"
#include "../../config/vehicle_config.h"
#include <Arduino.h>

namespace ABSDSC {

// ABS/DSC state variables
static bool abs_warning = false;      // ABS warning light
static bool dsc_off = false;          // DSC system off
static bool brake_fail = false;       // Brake failure warning
static bool etc_active = false;       // Electronic throttle control active
static bool etc_disabled = false;     // ETC disabled

// CAN message buffers
static uint8_t msg_620[7];  // ABS system data
static uint8_t msg_630[8];  // ABS configuration
static uint8_t msg_650[1];  // ABS supplement
static uint8_t msg_212[7];  // DSC status (optional)

void init() {
    // Initialize state
    abs_warning = false;
    dsc_off = false;
    brake_fail = false;
    etc_active = false;
    etc_disabled = false;

    // Initialize CAN message 0x620 - ABS system data
    // NOTE: Byte 6 varies by vehicle (2, 3, or 4)
    // If ABS light stays on, try different values
    msg_620[0] = 0;
    msg_620[1] = 0;
    msg_620[2] = 0;
    msg_620[3] = 0;
    #if VEHICLE_TYPE == VEHICLE_TYPE_EV
        msg_620[4] = 0;   // EV: byte 4 = 0
    #else
        msg_620[4] = 16;  // ICE: byte 4 = 16
    #endif
    msg_620[5] = 0;
    msg_620[6] = ABS_VARIANT;  // From vehicle_config.h (2, 3, or 4)

    // Initialize CAN message 0x630 - ABS configuration
    // Encodes transmission type and wheel size
    msg_630[0] = TRANSMISSION_CONFIG_BYTE;  // From vehicle_config.h
    msg_630[1] = 0;
    msg_630[2] = 0;
    msg_630[3] = 0;
    msg_630[4] = 0;
    msg_630[5] = 0;
    msg_630[6] = WHEEL_SIZE_PARAM;  // From vehicle_config.h (typically 106)
    msg_630[7] = WHEEL_SIZE_PARAM;  // From vehicle_config.h (typically 106)

    // Initialize CAN message 0x650 - ABS supplement
    msg_650[0] = 0;

    // Initialize CAN message 0x212 - DSC status (optional)
    msg_212[0] = 0;
    msg_212[1] = 0;
    msg_212[2] = 0;
    msg_212[3] = 0;
    msg_212[4] = 0;
    msg_212[5] = 0;
    msg_212[6] = 0;

    #if ENABLE_SERIAL_DEBUG
        Serial.println("[ABS/DSC] ABS/DSC emulation initialized");
        Serial.printf("[ABS/DSC] ABS variant: %d (byte 6 of 0x620)\n", ABS_VARIANT);
        Serial.printf("[ABS/DSC] Transmission: %s\n",
                     (TRANSMISSION_TYPE == TRANSMISSION_MANUAL) ? "Manual" : "Auto");
        Serial.printf("[ABS/DSC] Wheel size parameter: %d\n", WHEEL_SIZE_PARAM);
    #endif
}

void update(uint16_t vehicle_speed) {
    // Update DSC status message (0x212) based on current state
    // This message is optional and usually commented out in production

    // Byte layout for 0x212:
    // Byte 5, bit 1: DSC Off indicator
    // Byte 5, bit 2: ABS warning light
    // Byte 5, bit 3: Brake failure warning
    // Byte 5, bit 6: ETC active backlight
    // Byte 6, bit 3: ETC disabled

    msg_212[5] = 0;
    msg_212[6] = 0;

    if (dsc_off) {
        msg_212[5] |= 0b00000010;  // Bit 1: DSC off
    }

    if (abs_warning) {
        msg_212[5] |= 0b00000100;  // Bit 2: ABS warning
    }

    if (brake_fail) {
        msg_212[5] |= 0b00001000;  // Bit 3: Brake failure
    }

    if (etc_active) {
        msg_212[5] |= 0b01000000;  // Bit 6: ETC active
    }

    if (etc_disabled) {
        msg_212[6] |= 0b00001000;  // Bit 3: ETC disabled
    }

    // Future: Add actual DSC/traction control logic based on:
    // - Wheel speed differences
    // - Lateral acceleration
    // - Yaw rate
    // - Steering angle
}

bool getABSWarning() {
    return abs_warning;
}

void setABSWarning(bool state) {
    abs_warning = state;
}

bool getDSCOff() {
    return dsc_off;
}

void setDSCOff(bool state) {
    dsc_off = state;
}

bool getBrakeFailWarning() {
    return brake_fail;
}

void setBrakeFailWarning(bool state) {
    brake_fail = state;
}

bool getETCActive() {
    return etc_active;
}

void setETCActive(bool state) {
    etc_active = state;
}

bool getETCDisabled() {
    return etc_disabled;
}

void setETCDisabled(bool state) {
    etc_disabled = state;
}

void transmitCANMessages() {
    // Transmit ABS system messages (always)
    CANController::transmit(0x620, msg_620, 7);
    CANController::transmit(0x630, msg_630, 8);
    CANController::transmit(0x650, msg_650, 1);

    // Transmit DSC status (optional, controlled by config)
    #if ENABLE_DSC_CONTROL
        update(0);  // Update DSC message before transmitting
        CANController::transmit(0x212, msg_212, 7);
    #endif

    #if ENABLE_SERIAL_DEBUG >= 2  // Verbose logging
        Serial.println("[ABS/DSC] Transmitted ABS/DSC CAN messages");
    #endif
}

} // namespace ABSDSC
