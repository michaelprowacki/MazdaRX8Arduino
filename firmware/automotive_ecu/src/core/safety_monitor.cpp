/**
 * @file safety_monitor.cpp
 * @brief Safety monitoring and watchdog for automotive ECU
 *
 * Implements critical safety features including hardware watchdog,
 * CAN timeout detection, and failsafe mode.
 *
 * @author Created for unified automotive ECU
 * @date 2025-11-16
 */

#include "safety_monitor.h"
#include "../hal/hal_interface.h"
#include "../../config/vehicle_config.h"
#include <Arduino.h>

namespace SafetyMonitor {

// Safety state
static SafetyState current_state = SafetyState::NORMAL;
static const char* failsafe_reason = nullptr;

// CAN activity tracking
static uint32_t last_can_rx_time = 0;
static bool can_timeout_detected = false;

// Temperature limits (°C * 10)
static const int16_t TEMP_MIN = -400;   // -40°C
static const int16_t TEMP_MAX = 1250;   // 125°C
static const int16_t TEMP_WARNING = 1100; // 110°C

// Voltage limits (V * 100)
static const uint16_t VOLTAGE_MIN = 1000;  // 10.0V
static const uint16_t VOLTAGE_MAX = 1600;  // 16.0V
static const uint16_t VOLTAGE_WARNING_LOW = 1100;  // 11.0V
static const uint16_t VOLTAGE_WARNING_HIGH = 1500; // 15.0V

// Watchdog kick counter (for diagnostics)
static uint32_t watchdog_kick_count = 0;

void init() {
    current_state = SafetyState::NORMAL;
    failsafe_reason = nullptr;
    last_can_rx_time = millis();
    can_timeout_detected = false;
    watchdog_kick_count = 0;

    // Initialize hardware watchdog via HAL
    // Timeout varies by platform:
    // - STM32: 4 seconds (typical)
    // - C2000: Configurable
    // - S32K: Configurable
    HAL_Watchdog_Init();

    #if ENABLE_SERIAL_DEBUG
        Serial.println("[SAFETY] Safety monitor initialized");
        Serial.println("[SAFETY] Hardware watchdog enabled");
        Serial.printf("[SAFETY] Temperature limits: %.1f°C to %.1f°C\n",
                     TEMP_MIN / 10.0f, TEMP_MAX / 10.0f);
        Serial.printf("[SAFETY] Voltage limits: %.2fV to %.2fV\n",
                     VOLTAGE_MIN / 100.0f, VOLTAGE_MAX / 100.0f);
    #endif
}

void kick() {
    // Kick the hardware watchdog
    // This MUST be called every loop iteration (< timeout period)
    HAL_Watchdog_Kick();
    watchdog_kick_count++;

    #if ENABLE_SERIAL_DEBUG >= 3  // Very verbose logging
        if (watchdog_kick_count % 1000 == 0) {
            Serial.printf("[SAFETY] Watchdog kicked %lu times\n", watchdog_kick_count);
        }
    #endif
}

void update(bool can_rx_active) {
    // Update CAN activity timestamp if active
    if (can_rx_active) {
        last_can_rx_time = millis();
        can_timeout_detected = false;
    }

    // Check for CAN timeout
    if (isCANTimeout(CAN_RX_TIMEOUT_MS)) {
        if (!can_timeout_detected) {
            can_timeout_detected = true;

            #if ENABLE_SERIAL_DEBUG
                Serial.println("[SAFETY] WARNING: CAN timeout detected!");
            #endif

            // Enter failsafe if enabled
            #if ENABLE_CAN_TIMEOUT_FAILSAFE
                enterFailsafe("CAN RX timeout");
            #else
                // Just warn, don't enter failsafe
                if (current_state == SafetyState::NORMAL) {
                    current_state = SafetyState::WARNING;
                }
            #endif
        }
    }

    // Additional safety checks can be added here:
    // - Temperature monitoring
    // - Voltage monitoring
    // - Sensor plausibility checks
}

SafetyState getState() {
    return current_state;
}

void enterFailsafe(const char* reason) {
    if (current_state != SafetyState::FAILSAFE) {
        current_state = SafetyState::FAILSAFE;
        failsafe_reason = reason;

        #if ENABLE_SERIAL_DEBUG
            Serial.println("╔════════════════════════════════════════╗");
            Serial.println("║     FAILSAFE MODE ACTIVATED            ║");
            Serial.println("╚════════════════════════════════════════╝");
            Serial.printf("[SAFETY] Reason: %s\n", reason);
            Serial.println("[SAFETY] Actions:");
            Serial.println("[SAFETY]   - Throttle → 0%");
            Serial.println("[SAFETY]   - RPM → 0 (or idle)");
            Serial.println("[SAFETY]   - All warning lights ON");
            Serial.println("[SAFETY]   - Motor control disabled");
        #endif

        // Failsafe actions are handled by main loop checking getState()
        // Main loop should:
        // - Set throttle to 0%
        // - Set RPM to 0 or idle
        // - Activate all warning lights
        // - Disable motor control (EV)
    }
}

void exitFailsafe() {
    if (current_state == SafetyState::FAILSAFE) {
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[SAFETY] Exiting failsafe mode");
            Serial.printf("[SAFETY] Previous reason: %s\n",
                         failsafe_reason ? failsafe_reason : "unknown");
        #endif

        current_state = SafetyState::NORMAL;
        failsafe_reason = nullptr;
    }
}

bool isCANTimeout(uint32_t timeout_ms) {
    return (millis() - last_can_rx_time) > timeout_ms;
}

void updateCANActivity() {
    last_can_rx_time = millis();
    can_timeout_detected = false;
}

uint32_t getTimeSinceCANActivity() {
    return millis() - last_can_rx_time;
}

bool isTemperatureSafe(int16_t temp) {
    if (temp < TEMP_MIN || temp > TEMP_MAX) {
        #if ENABLE_SERIAL_DEBUG
            Serial.printf("[SAFETY] Temperature out of range: %.1f°C\n", temp / 10.0f);
        #endif
        return false;
    }

    if (temp > TEMP_WARNING) {
        #if ENABLE_SERIAL_DEBUG >= 2
            Serial.printf("[SAFETY] Temperature warning: %.1f°C\n", temp / 10.0f);
        #endif
    }

    return true;
}

bool isVoltageSafe(uint16_t voltage) {
    if (voltage < VOLTAGE_MIN || voltage > VOLTAGE_MAX) {
        #if ENABLE_SERIAL_DEBUG
            Serial.printf("[SAFETY] Voltage out of range: %.2fV\n", voltage / 100.0f);
        #endif
        return false;
    }

    if (voltage < VOLTAGE_WARNING_LOW || voltage > VOLTAGE_WARNING_HIGH) {
        #if ENABLE_SERIAL_DEBUG >= 2
            Serial.printf("[SAFETY] Voltage warning: %.2fV\n", voltage / 100.0f);
        #endif
    }

    return true;
}

const char* getFailsafeReason() {
    return failsafe_reason ? failsafe_reason : "none";
}

} // namespace SafetyMonitor
