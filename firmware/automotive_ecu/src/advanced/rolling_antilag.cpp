/**
 * @file rolling_antilag.cpp
 * @brief Rolling anti-lag system implementation
 *
 * @author Created for Phase 5++ Haltech-style features
 * @date 2025-11-16
 */

#include "rolling_antilag.h"
#include <Arduino.h>

namespace RollingAntilag {

// ============================================================================
// CONFIGURATION
// ============================================================================

static AntilagConfig config;
static AntilagStatus status;

// Fuel cut counter (for pattern generation)
static uint8_t fuel_cut_counter = 0;

// ============================================================================
// INITIALIZATION
// ============================================================================

void init(const AntilagConfig& cfg) {
    config = cfg;

    Serial.println("[RollingALS] Initializing rolling anti-lag system");
    Serial.printf("[RollingALS] Enabled: %s\n", config.enabled ? "YES" : "NO");
    Serial.printf("[RollingALS] RPM range: %d-%d\n", config.min_rpm, config.max_rpm);
    Serial.printf("[RollingALS] Speed range: %.1f-%.1f km/h\n",
                 config.min_speed / 10.0f,
                 config.max_speed > 0 ? config.max_speed / 10.0f : 999.9f);
    Serial.printf("[RollingALS] Gear range: %d-%d\n", config.min_gear, config.max_gear);
    Serial.printf("[RollingALS] Timing retard: %d°\n", config.timing_retard);
    Serial.printf("[RollingALS] Fuel cut ratio: %d%%\n", config.fuel_cut_ratio);

    // Initialize status
    status = {};
    status.active = false;
    status.conditions_met = false;
    status.wot_recently = false;
}

// ============================================================================
// MAIN UPDATE
// ============================================================================

void update(uint16_t rpm, uint8_t throttle, uint16_t speed,
            uint8_t gear, uint16_t boost, int16_t coolant_temp) {

    // Update current values
    status.current_rpm = rpm;
    status.current_tps = throttle;
    status.current_speed = speed;
    status.current_gear = gear;
    status.current_boost = boost;

    // Check if disabled
    if (!config.enabled) {
        if (status.active) {
            emergencyDeactivate();
        }
        return;
    }

    // Track WOT
    if (throttle >= 95) {  // 95% or more is WOT
        status.last_wot_time = millis();
        status.wot_recently = true;
    } else {
        // Check if WOT memory has expired
        uint32_t time_since_wot = millis() - status.last_wot_time;
        if (time_since_wot > config.wot_memory_time) {
            status.wot_recently = false;
        }
    }

    // Check if conditions met
    status.conditions_met = shouldActivate(rpm, throttle, speed, gear,
                                          boost, coolant_temp);

    // Activate/deactivate based on conditions
    if (status.conditions_met && !status.active) {
        // Activate anti-lag
        status.active = true;
        status.activation_time = millis();
        status.total_activations++;

        Serial.println("[RollingALS] *** ACTIVATED ***");
        Serial.printf("[RollingALS]   RPM: %d, Speed: %.1f km/h, Gear: %d\n",
                     rpm, speed / 10.0f, gear);
        Serial.printf("[RollingALS]   Boost: %.1f PSI\n", boost / 10.0f);
    } else if (!status.conditions_met && status.active) {
        // Deactivate anti-lag
        uint32_t duration = millis() - status.activation_time;
        status.total_time += duration;
        status.active = false;
        status.activation_time = 0;
        status.duration = 0;

        Serial.printf("[RollingALS] Deactivated (duration: %lu ms)\n", duration);
    }

    // Update duration if active
    if (status.active) {
        status.duration = millis() - status.activation_time;

        // Check maximum duration
        if (status.duration >= config.max_duration) {
            Serial.println("[RollingALS] Max duration reached, deactivating");
            emergencyDeactivate();
        }
    }

    // Increment fuel cut counter
    fuel_cut_counter++;
}

// ============================================================================
// ACTIVATION LOGIC
// ============================================================================

bool shouldActivate(uint16_t rpm, uint8_t throttle, uint16_t speed,
                   uint8_t gear, uint16_t boost, int16_t coolant_temp) {

    // Must have had WOT recently (prevents activation during normal driving)
    if (!status.wot_recently) {
        return false;
    }

    // Check RPM range
    if (rpm < config.min_rpm || rpm > config.max_rpm) {
        return false;
    }

    // Check speed range
    if (speed < config.min_speed) {
        return false;
    }
    if (config.max_speed > 0 && speed > config.max_speed) {
        return false;
    }

    // Check gear range
    if (gear < config.min_gear || gear > config.max_gear) {
        return false;
    }

    // Check throttle position
    // Active when throttle is closed (lift-off)
    if (status.active) {
        // Hysteresis: use deactivate threshold
        if (throttle > config.tps_deactivate) {
            return false;  // Throttle reapplied, deactivate
        }
    } else {
        // Use activate threshold
        if (throttle > config.tps_activate) {
            return false;  // Throttle not closed enough
        }
    }

    // Safety: coolant temperature
    if (coolant_temp > config.max_coolant_temp) {
        return false;
    }

    // Safety: boost pressure
    if (boost > config.max_boost) {
        return false;
    }

    return true;
}

// ============================================================================
// CONTROL OUTPUTS
// ============================================================================

int8_t getTimingAdjustment() {
    if (!status.active) {
        return 0;
    }

    // Return timing retard (negative value)
    return -config.timing_retard;
}

bool shouldCutCylinder(uint8_t cylinder) {
    if (!status.active) {
        return false;
    }

    // Fuel cut pattern based on ratio
    // 0% = no cut (all cylinders fire)
    // 25% = cut 1 in 4
    // 50% = cut 1 in 2 (alternating)
    // 75% = cut 3 in 4
    // 100% = cut all (not recommended)

    if (config.fuel_cut_ratio == 0) {
        return false;  // No fuel cut
    }

    if (config.fuel_cut_ratio >= 100) {
        return true;  // Cut all cylinders
    }

    // Use counter to generate pattern
    // For 50%: cut every other (counter % 2 == 0)
    // For 25%: cut every 4th (counter % 4 == 0)
    // For 75%: cut 3 out of 4 (counter % 4 != 3)

    if (config.fuel_cut_ratio <= 25) {
        // Cut 1 in 4
        return (fuel_cut_counter % 4 == 0);
    } else if (config.fuel_cut_ratio <= 50) {
        // Cut 1 in 2 (alternating)
        return (fuel_cut_counter % 2 == 0);
    } else {
        // Cut 3 in 4
        return (fuel_cut_counter % 4 != 3);
    }
}

// ============================================================================
// CONTROL
// ============================================================================

void setEnabled(bool enabled) {
    config.enabled = enabled;

    if (!enabled && status.active) {
        emergencyDeactivate();
    }

    Serial.printf("[RollingALS] %s\n", enabled ? "ENABLED" : "DISABLED");
}

const AntilagStatus* getStatus() {
    return &status;
}

void emergencyDeactivate() {
    Serial.println("[RollingALS] !!! EMERGENCY DEACTIVATION !!!");

    if (status.active) {
        uint32_t duration = millis() - status.activation_time;
        status.total_time += duration;
    }

    status.active = false;
    status.activation_time = 0;
    status.duration = 0;
    status.conditions_met = false;
}

// ============================================================================
// PRESET CONFIGURATIONS
// ============================================================================

AntilagConfig createStreetConfig() {
    AntilagConfig cfg = {};

    cfg.enabled = true;

    // Activation conditions (conservative)
    cfg.min_rpm = 3000;         // 3000 RPM minimum
    cfg.max_rpm = 7000;         // 7000 RPM maximum
    cfg.min_speed = 300;        // 30 km/h (3rd gear minimum)
    cfg.max_speed = 1200;       // 120 km/h maximum
    cfg.min_gear = 2;           // 2nd gear
    cfg.max_gear = 4;           // 4th gear

    // Throttle conditions
    cfg.tps_activate = 10;      // Activate below 10% TPS (lift-off)
    cfg.tps_deactivate = 30;    // Deactivate above 30% TPS (throttle reapplied)
    cfg.wot_memory_time = 2000; // Remember WOT for 2 seconds

    // Control parameters (moderate)
    cfg.timing_retard = 10;     // 10° timing retard
    cfg.fuel_cut_ratio = 25;    // 25% fuel cut (cut 1 in 4)
    cfg.boost_target = 100;     // Try to maintain 10 PSI

    // Safety
    cfg.max_coolant_temp = 1050; // 105°C max
    cfg.max_boost = 150;        // 15 PSI max
    cfg.max_duration = 3000;    // 3 second max continuous

    return cfg;
}

AntilagConfig createTrackConfig() {
    AntilagConfig cfg = {};

    cfg.enabled = true;

    // Activation conditions (aggressive)
    cfg.min_rpm = 4000;         // 4000 RPM
    cfg.max_rpm = 8500;         // 8500 RPM
    cfg.min_speed = 400;        // 40 km/h
    cfg.max_speed = 0;          // No max (unlimited)
    cfg.min_gear = 2;           // 2nd gear
    cfg.max_gear = 5;           // 5th gear

    // Throttle conditions
    cfg.tps_activate = 15;      // Activate below 15% TPS
    cfg.tps_deactivate = 40;    // Deactivate above 40% TPS
    cfg.wot_memory_time = 3000; // Remember WOT for 3 seconds

    // Control parameters (aggressive)
    cfg.timing_retard = 15;     // 15° timing retard
    cfg.fuel_cut_ratio = 50;    // 50% fuel cut (alternating)
    cfg.boost_target = 120;     // Try to maintain 12 PSI

    // Safety
    cfg.max_coolant_temp = 1100; // 110°C max
    cfg.max_boost = 180;        // 18 PSI max
    cfg.max_duration = 4000;    // 4 second max

    return cfg;
}

AntilagConfig createDragConfig() {
    AntilagConfig cfg = {};

    cfg.enabled = true;

    // Activation conditions (maximum)
    cfg.min_rpm = 5000;         // 5000 RPM
    cfg.max_rpm = 9000;         // 9000 RPM
    cfg.min_speed = 200;        // 20 km/h (1st gear launch)
    cfg.max_speed = 1600;       // 160 km/h (1/4 mile)
    cfg.min_gear = 1;           // 1st gear (launch)
    cfg.max_gear = 4;           // 4th gear (1/4 mile)

    // Throttle conditions
    cfg.tps_activate = 20;      // Activate below 20% TPS
    cfg.tps_deactivate = 50;    // Deactivate above 50% TPS
    cfg.wot_memory_time = 1500; // Remember WOT for 1.5 seconds

    // Control parameters (extreme)
    cfg.timing_retard = 20;     // 20° timing retard
    cfg.fuel_cut_ratio = 75;    // 75% fuel cut (heavy)
    cfg.boost_target = 150;     // Try to maintain 15 PSI

    // Safety
    cfg.max_coolant_temp = 1150; // 115°C max
    cfg.max_boost = 200;        // 20 PSI max
    cfg.max_duration = 2000;    // 2 second max (short drag shifts)

    return cfg;
}

} // namespace RollingAntilag
