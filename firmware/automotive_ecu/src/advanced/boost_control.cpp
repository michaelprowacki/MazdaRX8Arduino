/**
 * @file boost_control.cpp
 * @brief Turbo boost control implementation
 *
 * @author Created for Phase 5+ advanced features
 * @date 2025-11-16
 */

#include "boost_control.h"
#include <Arduino.h>

namespace BoostControl {

// ============================================================================
// PRIVATE STATE
// ============================================================================

static BoostConfig config = {
    .target_boost_psi = {50, 70, 90, 100, 100, 90},  // 5/7/9/10/10/9 PSI
    .max_boost_psi = 120,           // 12 PSI max (safe for stock 13B-MSP)
    .boost_ramp_rate = 50,          // Medium ramp rate
    .antilag_enabled = false,       // Disabled by default (hard on engine)
    .wastegate_preload = 30,        // 30% preload (typical)
    .spool_rpm_threshold = 2500     // Start boost control at 2500 RPM
};

static BoostStatus status = {
    .current_boost_psi = 0,
    .target_boost_psi = 0,
    .wastegate_duty = 0,
    .overboost_detected = false,
    .antilag_active = false,
    .current_gear = 0
};

// PID controller state for closed-loop boost control
static float pid_integral = 0.0f;
static float pid_previous_error = 0.0f;
static const float PID_KP = 0.5f;   // Proportional gain
static const float PID_KI = 0.1f;   // Integral gain
static const float PID_KD = 0.05f;  // Derivative gain

// Anti-lag state
static uint32_t antilag_last_trigger = 0;
static bool throttle_was_closed = false;

// ============================================================================
// INITIALIZATION
// ============================================================================

void init() {
    Serial.println("[BOOST] Boost control initialized");
    Serial.printf("[BOOST] Max boost: %.1f PSI\n", config.max_boost_psi / 10.0f);
    Serial.printf("[BOOST] Anti-lag: %s\n", config.antilag_enabled ? "ENABLED" : "DISABLED");
    Serial.println("[BOOST] Boost-by-gear targets:");
    for (int i = 0; i < 6; i++) {
        Serial.printf("[BOOST]   Gear %d: %.1f PSI\n", i + 1, config.target_boost_psi[i] / 10.0f);
    }
}

// ============================================================================
// MAIN UPDATE
// ============================================================================

uint8_t update(uint16_t rpm, uint8_t throttle, uint16_t current_boost, uint8_t gear) {
    status.current_boost_psi = current_boost;
    status.current_gear = gear;

    // Determine target boost based on gear
    if (gear >= 1 && gear <= 6) {
        status.target_boost_psi = config.target_boost_psi[gear - 1];
    } else {
        status.target_boost_psi = 0;  // Neutral/invalid gear = no boost
    }

    // Check for overboost
    status.overboost_detected = checkOverboost(current_boost);
    if (status.overboost_detected) {
        // Cut boost immediately
        status.wastegate_duty = 100;  // Fully open wastegate
        Serial.println("[BOOST] OVERBOOST! Wastegate fully open");
        return status.wastegate_duty;
    }

    // Only control boost above spool threshold
    if (rpm < config.spool_rpm_threshold) {
        status.wastegate_duty = config.wastegate_preload;
        pid_integral = 0.0f;  // Reset integrator
        return status.wastegate_duty;
    }

    // Calculate wastegate duty using closed-loop control
    status.wastegate_duty = calculateWastegateDuty(
        status.target_boost_psi,
        current_boost,
        rpm
    );

    // Anti-lag control (if enabled and conditions met)
    if (config.antilag_enabled) {
        status.antilag_active = controlAntiLag(rpm, throttle, current_boost);
    }

    return status.wastegate_duty;
}

// ============================================================================
// WASTEGATE CONTROL (PID)
// ============================================================================

uint8_t calculateWastegateDuty(uint16_t target_boost, uint16_t current_boost, uint16_t rpm) {
    // Error in PSI * 10
    float error = (float)(target_boost - current_boost) / 10.0f;

    // PID terms
    float p_term = PID_KP * error;

    pid_integral += error;
    pid_integral = constrain(pid_integral, -50.0f, 50.0f);  // Anti-windup
    float i_term = PID_KI * pid_integral;

    float d_term = PID_KD * (error - pid_previous_error);
    pid_previous_error = error;

    // Total PID output (-100 to +100)
    float pid_output = p_term + i_term + d_term;

    // Convert to wastegate duty (0-100%)
    // Lower duty = more boost (wastegate closed)
    // Higher duty = less boost (wastegate open)
    int16_t duty = config.wastegate_preload - (int16_t)pid_output;

    // Apply boost ramping (prevent boost spikes)
    static uint8_t previous_duty = config.wastegate_preload;
    int16_t max_change = (100 - config.boost_ramp_rate) / 10 + 1;  // Faster ramp = larger changes

    if (duty > previous_duty + max_change) {
        duty = previous_duty + max_change;
    } else if (duty < previous_duty - max_change) {
        duty = previous_duty - max_change;
    }

    previous_duty = duty;

    // Clamp to 0-100%
    duty = constrain(duty, 0, 100);

    return (uint8_t)duty;
}

// ============================================================================
// ANTI-LAG SYSTEM
// ============================================================================

bool controlAntiLag(uint16_t rpm, uint8_t throttle, uint16_t boost) {
    // Anti-lag activates when:
    // 1. Throttle closes quickly (lift-off)
    // 2. RPM is in boost range (3000-8000)
    // 3. We had boost pressure

    const uint16_t MIN_RPM = 3000;
    const uint16_t MAX_RPM = 8000;
    const uint8_t THROTTLE_THRESHOLD = 20;  // Less than 20% = closed
    const uint16_t MIN_BOOST = 30;  // 3 PSI minimum to trigger

    bool should_activate = false;

    // Detect throttle closing
    if (throttle < THROTTLE_THRESHOLD && !throttle_was_closed) {
        // Throttle just closed
        if (rpm >= MIN_RPM && rpm <= MAX_RPM && boost >= MIN_BOOST) {
            should_activate = true;
            antilag_last_trigger = millis();
            Serial.println("[BOOST] Anti-lag activated!");
        }
    }

    throttle_was_closed = (throttle < THROTTLE_THRESHOLD);

    // Keep anti-lag active for short duration after trigger
    if ((millis() - antilag_last_trigger) < 1000) {  // 1 second max
        should_activate = true;
    }

    // Anti-lag actions (when active):
    // - Retard ignition timing severely (20-30°)
    // - Keep injectors firing
    // - May cut spark on some cycles
    // - Keeps exhaust gas energy high → spools turbo
    //
    // Implementation note: This requires integration with ignition
    // and fuel systems (not fully implemented here)

    return should_activate;
}

// ============================================================================
// SAFETY CHECKS
// ============================================================================

bool checkOverboost(uint16_t boost) {
    if (boost > config.max_boost_psi) {
        // Sustained overboost
        static uint32_t overboost_start = 0;
        static bool was_overboost = false;

        if (!was_overboost) {
            overboost_start = millis();
            was_overboost = true;
        }

        // Allow brief spikes (100ms) but cut if sustained
        if ((millis() - overboost_start) > 100) {
            Serial.printf("[BOOST] OVERBOOST: %.1f PSI (max: %.1f PSI)\n",
                         boost / 10.0f, config.max_boost_psi / 10.0f);
            return true;
        }
    } else {
        static bool was_overboost = false;
        was_overboost = false;
    }

    return false;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

const BoostStatus* getStatus() {
    return &status;
}

void configure(const BoostConfig& new_config) {
    config = new_config;
    Serial.println("[BOOST] Boost configuration updated");
    init();  // Re-display configuration
}

void setGearBoost(uint8_t gear, uint16_t boost_psi) {
    if (gear >= 1 && gear <= 6) {
        config.target_boost_psi[gear - 1] = boost_psi;
        Serial.printf("[BOOST] Gear %d boost set to %.1f PSI\n", gear, boost_psi / 10.0f);
    }
}

void setAntiLagEnabled(bool enabled) {
    config.antilag_enabled = enabled;
    Serial.printf("[BOOST] Anti-lag %s\n", enabled ? "ENABLED" : "DISABLED");

    if (enabled) {
        Serial.println("[BOOST] WARNING: Anti-lag increases exhaust temps!");
        Serial.println("[BOOST] WARNING: May damage catalytic converter");
        Serial.println("[BOOST] WARNING: Use only on track/race applications");
    }
}

} // namespace BoostControl
