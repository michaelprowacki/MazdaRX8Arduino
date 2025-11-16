/**
 * @file launch_control.cpp
 * @brief Launch control implementation
 *
 * @author Created for Phase 5+ advanced features
 * @date 2025-11-16
 */

#include "launch_control.h"
#include <Arduino.h>

namespace LaunchControl {

// ============================================================================
// PRIVATE STATE
// ============================================================================

static LaunchConfig config = {
    .launch_rpm = 4000,         // 4000 RPM launch limit
    .main_rpm_limit = 9000,     // 9000 RPM main limiter (13B-MSP redline)
    .ignition_cut_mode = 1,     // Hard cut (most aggressive)
    .launch_retard = 15,        // 15° retard during launch
    .boost_build = true,        // Build boost at standstill
    .speed_threshold = 100,     // 10 km/h - deactivate above this
    .require_clutch = true      // Require clutch for safety
};

static LaunchStatus status = {
    .armed = false,
    .active = false,
    .current_rpm_limit = 9000,
    .ignition_cuts = 0,
    .clutch_detected = false
};

static bool enabled = false;
static uint32_t launch_count = 0;
static uint32_t last_launch_time = 0;

// ============================================================================
// INITIALIZATION
// ============================================================================

void init() {
    Serial.println("[LAUNCH] Launch control initialized");
    Serial.printf("[LAUNCH] Launch RPM: %d\n", config.launch_rpm);
    Serial.printf("[LAUNCH] Main limiter: %d RPM\n", config.main_rpm_limit);
    Serial.printf("[LAUNCH] Mode: %s\n",
                 config.ignition_cut_mode == 0 ? "Soft Cut" :
                 config.ignition_cut_mode == 1 ? "Hard Cut" : "Retard");
    Serial.printf("[LAUNCH] Status: %s\n", enabled ? "ENABLED" : "DISABLED");
}

// ============================================================================
// MAIN UPDATE
// ============================================================================

bool update(uint16_t rpm, uint8_t throttle, uint16_t speed, bool clutch_pressed) {
    if (!enabled) {
        status.armed = false;
        status.active = false;
        return false;
    }

    status.clutch_detected = clutch_pressed;

    // Check if should activate
    if (shouldActivate(rpm, throttle, speed, clutch_pressed)) {
        if (!status.active) {
            Serial.println("[LAUNCH] LAUNCH CONTROL ACTIVE!");
            last_launch_time = millis();
            launch_count++;
        }
        status.active = true;
        status.armed = false;
    } else {
        // Check if should arm
        if (throttle > 90 && speed < 10 && (clutch_pressed || !config.require_clutch)) {
            if (!status.armed) {
                Serial.println("[LAUNCH] Launch control ARMED");
            }
            status.armed = true;
            status.active = false;
        } else {
            status.armed = false;
            status.active = false;
        }
    }

    // Determine current RPM limit
    if (status.active) {
        status.current_rpm_limit = config.launch_rpm;
    } else {
        status.current_rpm_limit = config.main_rpm_limit;
    }

    // Apply RPM limiting if active
    if (status.active) {
        uint8_t limit_action = applyRPMLimiting(rpm);
        return (limit_action > 0);
    }

    return false;
}

// ============================================================================
// ACTIVATION LOGIC
// ============================================================================

void arm() {
    if (enabled) {
        status.armed = true;
        Serial.println("[LAUNCH] Launch control ARMED manually");
    }
}

void disarm() {
    status.armed = false;
    status.active = false;
    Serial.println("[LAUNCH] Launch control disarmed");
}

bool shouldActivate(uint16_t rpm, uint8_t throttle, uint16_t speed, bool clutch_pressed) {
    // Activation conditions:
    // 1. Launch control enabled and armed
    // 2. Throttle wide open (>90%)
    // 3. Vehicle speed low (<10 km/h)
    // 4. Clutch released (if required) - triggers launch
    // 5. RPM approaching launch limit

    if (!enabled || !status.armed) {
        return false;
    }

    if (throttle < 90) {
        return false;  // Not full throttle
    }

    if (speed > config.speed_threshold) {
        // Moving too fast - deactivate
        status.armed = false;
        status.active = false;
        return false;
    }

    // If clutch required, wait for clutch release
    if (config.require_clutch) {
        // Launch when clutch is released
        static bool clutch_was_pressed = false;

        if (clutch_pressed) {
            clutch_was_pressed = true;
            return status.active;  // Keep active if already active
        } else {
            // Clutch just released - launch!
            if (clutch_was_pressed) {
                clutch_was_pressed = false;
                return true;
            }
        }
    } else {
        // No clutch required - activate when conditions met
        if (rpm > (config.launch_rpm - 500)) {
            return true;
        }
    }

    return status.active;  // Keep current state
}

// ============================================================================
// RPM LIMITING
// ============================================================================

uint8_t applyRPMLimiting(uint16_t rpm) {
    // No limiting needed if below limit
    if (rpm < status.current_rpm_limit) {
        return 0;  // Normal ignition
    }

    // RPM at or above limit - apply limiting
    status.ignition_cuts++;

    switch (config.ignition_cut_mode) {
        case 0:  // Soft cut - cut every other ignition event
            return (status.ignition_cuts % 2 == 0) ? 1 : 0;

        case 1:  // Hard cut - cut all ignition events at limit
            return 1;

        case 2:  // Retard - severely retard timing instead of cutting
            // This mode requires integration with ignition system
            // Return special code that ignition system interprets
            return 2;

        default:
            return 1;  // Default to hard cut
    }
}

// ============================================================================
// CONFIGURATION
// ============================================================================

const LaunchStatus* getStatus() {
    return &status;
}

void configure(const LaunchConfig& new_config) {
    config = new_config;
    Serial.println("[LAUNCH] Launch control configuration updated");
    init();
}

void setLaunchRPM(uint16_t rpm) {
    config.launch_rpm = rpm;
    Serial.printf("[LAUNCH] Launch RPM set to %d\n", rpm);
}

void setEnabled(bool enable) {
    enabled = enable;
    if (!enabled) {
        status.armed = false;
        status.active = false;
    }
    Serial.printf("[LAUNCH] Launch control %s\n", enabled ? "ENABLED" : "DISABLED");
}

uint32_t getLaunchCount() {
    return launch_count;
}

} // namespace LaunchControl
