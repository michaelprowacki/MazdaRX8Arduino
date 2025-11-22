/**
 * @file wipers.cpp
 * @brief Speed-sensitive wiper control implementation
 *
 * Ported from legacy Wipers_Module to unified ESP32 firmware.
 *
 * @author Ported for Phase 5 unified architecture
 * @date 2025-11-16
 */

#include "wipers.h"
#include "../../config/features.h"
#include <Arduino.h>

#if ENABLE_WIPERS

namespace Wipers {

// Pin configuration
#define WIPER_RELAY_PIN     23  // GPIO for wiper relay control

// Timing constants (milliseconds)
static const uint32_t INTERMITTENT_MIN_DELAY = 1000;   // 1 second (fastest)
static const uint32_t INTERMITTENT_MAX_DELAY = 5000;   // 5 seconds (slowest)
static const uint32_t WIPE_DURATION = 500;             // 500ms wipe cycle
static const uint16_t SPEED_THRESHOLD_LOW = 30;        // 30 km/h
static const uint16_t SPEED_THRESHOLD_HIGH = 80;       // 80 km/h

// State variables
static Mode current_mode = MODE_OFF;
static uint8_t sensitivity = 50;                       // Default 50%
static bool speed_sensitive = true;
static bool wiper_active = false;
static uint32_t last_wipe_time = 0;
static uint32_t next_wipe_time = 0;
static uint16_t current_speed = 0;

// Forward declarations
static uint32_t calculateIntermittentDelay();
static void activateWiper();
static void deactivateWiper();

// ============================================================================
// INITIALIZATION
// ============================================================================

void init() {
    // Configure wiper relay pin
    pinMode(WIPER_RELAY_PIN, OUTPUT);
    digitalWrite(WIPER_RELAY_PIN, LOW);  // Off by default

    // Initialize state
    current_mode = MODE_OFF;
    sensitivity = 50;
    speed_sensitive = true;
    wiper_active = false;
    last_wipe_time = 0;
    next_wipe_time = 0;

    Serial.println("[WIPERS] Speed-sensitive wiper control initialized");
}

// ============================================================================
// UPDATE (CALLED FROM FREERTOS TASK)
// ============================================================================

void update(uint16_t vehicle_speed) {
    current_speed = vehicle_speed;
    uint32_t now = millis();

    switch (current_mode) {
        case MODE_OFF:
            // Ensure wipers are off
            if (wiper_active) {
                deactivateWiper();
            }
            break;

        case MODE_INTERMITTENT:
            // Intermittent mode with speed sensitivity
            if (!wiper_active) {
                // Check if it's time for next wipe
                if (now >= next_wipe_time) {
                    activateWiper();
                    next_wipe_time = now + calculateIntermittentDelay();
                    last_wipe_time = now;
                }
            } else {
                // Check if wipe cycle is complete
                if (now - last_wipe_time >= WIPE_DURATION) {
                    deactivateWiper();
                }
            }
            break;

        case MODE_LOW:
            // Continuous low speed
            if (!wiper_active) {
                activateWiper();
                last_wipe_time = now;
            } else {
                // Cycle: 500ms on, 500ms off
                if (now - last_wipe_time >= WIPE_DURATION) {
                    deactivateWiper();
                    last_wipe_time = now;
                }
                if (now - last_wipe_time >= WIPE_DURATION && !wiper_active) {
                    activateWiper();
                    last_wipe_time = now;
                }
            }
            break;

        case MODE_HIGH:
            // Continuous high speed (always on)
            if (!wiper_active) {
                activateWiper();
            }
            break;

        case MODE_AUTO:
            // Automatic mode (rain sensor)
            // For now, fall back to intermittent
            // TODO: Implement rain sensor integration
            if (now >= next_wipe_time) {
                activateWiper();
                next_wipe_time = now + calculateIntermittentDelay();
                last_wipe_time = now;
            }
            if (wiper_active && (now - last_wipe_time >= WIPE_DURATION)) {
                deactivateWiper();
            }
            break;
    }
}

// ============================================================================
// MODE CONTROL
// ============================================================================

void setMode(Mode mode) {
    current_mode = mode;

    #if ENABLE_SERIAL_DEBUG
        const char* mode_names[] = {"OFF", "INTERMITTENT", "LOW", "HIGH", "AUTO"};
        Serial.printf("[WIPERS] Mode set to: %s\n", mode_names[mode]);
    #endif

    // If switching to intermittent, calculate first delay
    if (mode == MODE_INTERMITTENT) {
        next_wipe_time = millis() + calculateIntermittentDelay();
    }
}

Mode getMode() {
    return current_mode;
}

// ============================================================================
// SENSITIVITY CONTROL
// ============================================================================

void setSensitivity(uint8_t new_sensitivity) {
    sensitivity = (new_sensitivity > 100) ? 100 : new_sensitivity;

    #if ENABLE_SERIAL_DEBUG
        Serial.printf("[WIPERS] Sensitivity set to: %d%%\n", sensitivity);
    #endif
}

uint8_t getSensitivity() {
    return sensitivity;
}

void setSpeedSensitive(bool enabled) {
    speed_sensitive = enabled;
}

// ============================================================================
// MANUAL CONTROL
// ============================================================================

void singleWipe() {
    if (!wiper_active) {
        activateWiper();
        last_wipe_time = millis();

        #if ENABLE_SERIAL_DEBUG
            Serial.println("[WIPERS] Single wipe activated");
        #endif
    }
}

// ============================================================================
// STATUS
// ============================================================================

bool isActive() {
    return wiper_active;
}

uint32_t getTimeUntilNextWipe() {
    if (current_mode != MODE_INTERMITTENT) {
        return 0;
    }

    uint32_t now = millis();
    if (now >= next_wipe_time) {
        return 0;
    }

    return next_wipe_time - now;
}

// ============================================================================
// INTERNAL FUNCTIONS
// ============================================================================

static uint32_t calculateIntermittentDelay() {
    // Base delay from sensitivity (0-100%)
    // Higher sensitivity = shorter delay
    uint32_t base_delay = map(sensitivity, 0, 100,
                              INTERMITTENT_MAX_DELAY, INTERMITTENT_MIN_DELAY);

    // Apply speed sensitivity
    if (speed_sensitive) {
        if (current_speed < SPEED_THRESHOLD_LOW) {
            // Low speed: increase delay by 50%
            base_delay = base_delay * 3 / 2;
        } else if (current_speed > SPEED_THRESHOLD_HIGH) {
            // High speed: decrease delay by 30%
            base_delay = base_delay * 7 / 10;
        }
        // Medium speed: use base delay as-is
    }

    return base_delay;
}

static void activateWiper() {
    digitalWrite(WIPER_RELAY_PIN, HIGH);
    wiper_active = true;

    #if ENABLE_SERIAL_DEBUG >= 2  // Verbose logging
        Serial.println("[WIPERS] Wiper activated");
    #endif
}

static void deactivateWiper() {
    digitalWrite(WIPER_RELAY_PIN, LOW);
    wiper_active = false;

    #if ENABLE_SERIAL_DEBUG >= 2  // Verbose logging
        Serial.println("[WIPERS] Wiper deactivated");
    #endif
}

} // namespace Wipers

#endif // ENABLE_WIPERS
