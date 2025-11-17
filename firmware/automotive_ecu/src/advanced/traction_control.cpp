/**
 * @file traction_control.cpp
 * @brief Active traction control implementation
 *
 * @author Created for Phase 5++ Haltech-style features
 * @date 2025-11-16
 */

#include "traction_control.h"
#include <Arduino.h>

namespace TractionControl {

// ============================================================================
// CONFIGURATION
// ============================================================================

static TCConfig config = createStreetConfig();

static TCStatus status = {
    .active = false,
    .slip_detected = false,
    .slip_ratio = 0,
    .power_reduction = 0,
    .ignition_cuts = 0,
    .timing_retard = 0,
    .intervention_count = 0,
    .wheel_speeds = {0, 0, 0, 0}
};

// Power reduction state (for gradual application)
static uint8_t target_power_reduction = 0;
static uint8_t actual_power_reduction = 0;

// ============================================================================
// INITIALIZATION
// ============================================================================

void init() {
    Serial.println("[TC] Traction control initialized");
    Serial.printf("[TC] Mode: %s\n",
                 config.mode == TC_OFF ? "OFF" :
                 config.mode == TC_SOFT ? "SOFT" :
                 config.mode == TC_MEDIUM ? "MEDIUM" : "HARD");
    Serial.printf("[TC] Slip threshold: %.1f%%\n", config.slip_threshold / 10.0f);
    Serial.printf("[TC] Speed range: %.1f-%.1f km/h\n",
                 config.min_speed / 10.0f,
                 config.max_speed > 0 ? config.max_speed / 10.0f : 999.9f);
}

// ============================================================================
// MAIN UPDATE
// ============================================================================

uint8_t update(const WheelSpeeds& wheel_speeds, uint8_t throttle,
               uint16_t rpm, uint8_t gear) {

    // Store current wheel speeds
    status.wheel_speeds = wheel_speeds;

    // Check if TC should be active
    uint16_t vehicle_speed = (wheel_speeds.front_left + wheel_speeds.front_right) / 2;

    if (config.mode == TC_OFF || !shouldBeActive(vehicle_speed, throttle)) {
        status.active = false;
        status.slip_detected = false;
        status.power_reduction = 0;
        actual_power_reduction = 0;
        return 0;
    }

    // Calculate slip ratio
    status.slip_ratio = calculateSlipRatio(wheel_speeds);

    // Detect slip
    status.slip_detected = (status.slip_ratio > config.slip_threshold);

    if (status.slip_detected) {
        // Calculate target power reduction
        target_power_reduction = calculatePowerReduction(status.slip_ratio);

        // Clamp to max
        target_power_reduction = min(target_power_reduction, config.max_power_reduction);

        // Gradually apply power reduction
        if (actual_power_reduction < target_power_reduction) {
            uint8_t increase = config.intervention_rate;
            actual_power_reduction = min(target_power_reduction,
                                        actual_power_reduction + increase);
        }

        status.active = true;
        status.intervention_count++;

        if (status.intervention_count % 10 == 1) {  // Log every 10th intervention
            Serial.printf("[TC] Slip detected: %.1f%%, reducing power %d%%\n",
                         status.slip_ratio / 10.0f, actual_power_reduction);
        }
    } else {
        // Gradually recover power
        if (actual_power_reduction > 0) {
            uint8_t decrease = config.recovery_rate;
            if (actual_power_reduction > decrease) {
                actual_power_reduction -= decrease;
            } else {
                actual_power_reduction = 0;
            }
        }

        if (actual_power_reduction == 0) {
            status.active = false;
        }
    }

    status.power_reduction = actual_power_reduction;

    // Calculate timing retard
    status.timing_retard = calculateTimingRetard(actual_power_reduction);

    return actual_power_reduction;
}

// ============================================================================
// SLIP CALCULATION
// ============================================================================

uint16_t calculateSlipRatio(const WheelSpeeds& wheel_speeds) {
    // For RWD (RX8): Compare rear wheels (driven) to front wheels (non-driven)

    // Calculate average front speed (non-driven wheels = reference)
    uint16_t front_avg = (wheel_speeds.front_left + wheel_speeds.front_right) / 2;

    // Calculate average rear speed (driven wheels)
    uint16_t rear_avg = (wheel_speeds.rear_left + wheel_speeds.rear_right) / 2;

    // Avoid division by zero
    if (front_avg < 10) {  // Below 1 km/h
        return 0;
    }

    // Slip ratio = (driven_speed - reference_speed) / reference_speed * 100
    // Expressed as % * 10 (e.g., 100 = 10% slip)
    int32_t slip = ((int32_t)(rear_avg - front_avg) * 1000) / front_avg;

    // Clamp to 0-200% (0-20% slip max)
    if (slip < 0) slip = 0;
    if (slip > 2000) slip = 2000;

    return (uint16_t)slip;
}

// ============================================================================
// POWER REDUCTION
// ============================================================================

uint8_t calculatePowerReduction(uint16_t slip_ratio) {
    // Progressive power reduction based on slip amount

    // No slip or very light slip (< threshold): No reduction
    if (slip_ratio <= config.slip_threshold) {
        return 0;
    }

    // Calculate slip above threshold
    uint16_t excess_slip = slip_ratio - config.slip_threshold;

    // Progressive mapping:
    // 0-50 (0-5% excess): 10-30% reduction
    // 50-100 (5-10% excess): 30-60% reduction
    // 100+ (10%+ excess): 60-100% reduction

    uint8_t reduction;

    if (excess_slip < 50) {
        // Light slip: 10-30% reduction
        reduction = 10 + (excess_slip * 20) / 50;
    } else if (excess_slip < 100) {
        // Medium slip: 30-60% reduction
        reduction = 30 + ((excess_slip - 50) * 30) / 50;
    } else {
        // Heavy slip: 60-100% reduction
        uint16_t heavy = excess_slip - 100;
        reduction = 60 + min(40, (heavy * 40) / 100);
    }

    return reduction;
}

uint8_t applyPowerReduction(uint8_t power_reduction, TCMode mode) {
    if (power_reduction == 0) {
        return 0;  // No intervention
    }

    switch (mode) {
        case TC_SOFT:
            // Soft mode: Timing retard only, no ignition cut
            return 2;  // Signal for timing retard

        case TC_MEDIUM:
            // Medium mode: Timing retard + occasional ignition cut
            if (power_reduction > 50) {
                // Heavy slip: Cut every other ignition event
                status.ignition_cuts++;
                return (status.ignition_cuts % 2 == 0) ? 1 : 2;
            } else {
                // Light slip: Timing retard only
                return 2;
            }

        case TC_HARD:
            // Hard mode: Aggressive ignition cutting
            status.ignition_cuts++;
            return 1;  // Cut ignition

        default:
            return 0;
    }
}

int8_t calculateTimingRetard(uint8_t power_reduction) {
    // Convert power reduction % to timing retard degrees
    // 0% reduction = 0° retard
    // 100% reduction = 10° retard (max)

    int8_t retard = (power_reduction * 10) / 100;

    return retard;
}

// ============================================================================
// ACTIVATION LOGIC
// ============================================================================

bool shouldBeActive(uint16_t speed, uint8_t throttle) {
    // Don't activate if disabled
    if (config.mode == TC_OFF) {
        return false;
    }

    // Only activate above minimum speed
    if (speed < config.min_speed) {
        return false;
    }

    // Don't activate above maximum speed (if set)
    if (config.max_speed > 0 && speed > config.max_speed) {
        return false;
    }

    // Only activate with significant throttle (>30%)
    if (throttle < 30) {
        return false;
    }

    return true;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

const TCStatus* getStatus() {
    return &status;
}

void configure(const TCConfig& new_config) {
    config = new_config;
    Serial.println("[TC] Configuration updated");
    init();
}

void setMode(TCMode mode) {
    config.mode = mode;
    Serial.printf("[TC] Mode set to: %s\n",
                 mode == TC_OFF ? "OFF" :
                 mode == TC_SOFT ? "SOFT" :
                 mode == TC_MEDIUM ? "MEDIUM" : "HARD");
}

void setSlipThreshold(uint8_t threshold) {
    config.slip_threshold = threshold;
    Serial.printf("[TC] Slip threshold set to: %.1f%%\n", threshold / 10.0f);
}

void setLaunchMode(bool enabled) {
    config.launch_mode_active = enabled;

    if (enabled) {
        // More aggressive TC for launch
        config.slip_threshold = 80;  // 8% slip (tighter)
        config.intervention_rate = 20;  // Faster intervention
        Serial.println("[TC] Launch mode: More aggressive TC");
    } else {
        // Normal TC settings
        config.slip_threshold = 100;  // 10% slip (normal)
        config.intervention_rate = 10;  // Normal intervention
        Serial.println("[TC] Launch mode: Normal TC");
    }
}

void resetCounters() {
    status.intervention_count = 0;
    status.ignition_cuts = 0;
    Serial.println("[TC] Counters reset");
}

// ============================================================================
// PRESET CONFIGURATIONS
// ============================================================================

TCConfig createStreetConfig() {
    return {
        .mode = TC_SOFT,            // Gentle intervention for street
        .slip_threshold = 100,      // 10% slip
        .max_power_reduction = 60,  // Max 60% reduction
        .min_speed = 20,            // 2 km/h minimum
        .max_speed = 1000,          // 100 km/h maximum (most useful <100)
        .launch_mode_active = false,
        .intervention_rate = 10,    // Gradual application
        .recovery_rate = 5          // Slow recovery (smooth)
    };
}

TCConfig createTrackConfig() {
    return {
        .mode = TC_MEDIUM,          // Moderate intervention
        .slip_threshold = 120,      // 12% slip (allow more slip)
        .max_power_reduction = 80,  // Max 80% reduction
        .min_speed = 20,            // 2 km/h minimum
        .max_speed = 0,             // No maximum speed
        .launch_mode_active = false,
        .intervention_rate = 15,    // Fast application
        .recovery_rate = 10         // Fast recovery
    };
}

TCConfig createDragConfig() {
    return {
        .mode = TC_HARD,            // Aggressive intervention
        .slip_threshold = 80,       // 8% slip (tight control)
        .max_power_reduction = 100, // Max 100% reduction
        .min_speed = 10,            // 1 km/h minimum (launch)
        .max_speed = 800,           // 80 km/h (1/4 mile)
        .launch_mode_active = true, // Launch mode enabled
        .intervention_rate = 20,    // Very fast application
        .recovery_rate = 15         // Fast recovery
    };
}

} // namespace TractionControl
