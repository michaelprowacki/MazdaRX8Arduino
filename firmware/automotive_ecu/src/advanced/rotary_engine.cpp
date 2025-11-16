/**
 * @file rotary_engine.cpp
 * @brief Advanced rotary engine management implementation
 *
 * Implements rotary-specific features for the Mazda 13B-MSP (RENESIS):
 * - Oil Metering Pump (OMP) control
 * - Dual ignition system (leading/trailing)
 * - Apex seal health monitoring
 * - Compression monitoring
 * - Oil consumption tracking
 *
 * @author Created for Phase 5+ advanced features
 * @date 2025-11-16
 */

#include "rotary_engine.h"
#include <Arduino.h>

namespace RotaryEngine {

// ============================================================================
// PRIVATE STATE
// ============================================================================

static OMPConfig omp_config = {
    .idle_rate = 30,        // 30% at idle
    .cruise_rate = 50,      // 50% at cruise
    .wot_rate = 100,        // 100% at WOT
    .rpm_threshold = 5000,  // Increase injection above 5000 RPM
    .premix_mode = false    // OMP enabled by default
};

static IgnitionConfig ignition_config = {
    .leading_advance_idle = 10,     // 10° BTDC at idle
    .leading_advance_cruise = 15,   // 15° BTDC at cruise
    .leading_advance_wot = 20,      // 20° BTDC at WOT
    .trailing_offset = 10,          // 10° retarded from leading
    .split_mode = true              // Enable split timing
};

static HealthStatus health_status = {
    .apex_seal_condition = 100,
    .compression_rotor1 = 100,
    .compression_rotor2 = 100,
    .oil_consumption_ml_100km = 500,  // 500ml/100km is normal for rotary
    .coolant_seal_ok = true,
    .oil_metering_ok = true,
    .hours_since_rebuild = 0
};

// Knock detection state
static uint8_t knock_retard_current = 0;  // Current knock retard (degrees)
static const uint8_t MAX_KNOCK_RETARD = 8;  // Max 8° retard

// Operating hours tracking
static uint32_t last_hours_update_ms = 0;

// ============================================================================
// INITIALIZATION
// ============================================================================

void init() {
    Serial.println("[ROTARY] Rotary engine management initialized");
    Serial.println("[ROTARY] 13B-MSP (RENESIS) configuration loaded");
    Serial.printf("[ROTARY] OMP: %s\n", omp_config.premix_mode ? "DISABLED (premix)" : "ENABLED");
    Serial.printf("[ROTARY] Ignition: Leading/Trailing dual spark\n");

    last_hours_update_ms = millis();
}

// ============================================================================
// MAIN UPDATE
// ============================================================================

void update(uint16_t rpm, uint8_t throttle, int16_t coolant_temp) {
    // Update OMP based on current conditions
    if (!omp_config.premix_mode) {
        uint8_t omp_duty = controlOMP(rpm, throttle);
        // TODO: Output OMP_duty to PWM pin controlling OMP solenoid
    }

    // Update ignition timing
    bool knock = false;  // TODO: Read from knock sensor
    uint8_t leading = calculateLeadingTiming(rpm, throttle, knock);
    uint8_t trailing = calculateTrailingTiming(leading, rpm);
    // TODO: Output timing to ignition drivers

    // Update operating hours (every minute)
    if (rpm > 0 && (millis() - last_hours_update_ms) > 60000) {
        recordOperatingHours(1.0f / 60.0f);  // 1 minute = 1/60 hour
        last_hours_update_ms = millis();
    }

    // Periodic health monitoring (every 10 seconds)
    static uint32_t last_health_check = 0;
    if ((millis() - last_health_check) > 10000) {
        monitorApexSeals();
        checkCoolantSeal();
        last_health_check = millis();
    }
}

// ============================================================================
// OIL METERING PUMP (OMP) CONTROL
// ============================================================================

uint8_t controlOMP(uint16_t rpm, uint8_t throttle) {
    if (omp_config.premix_mode) {
        return 0;  // OMP disabled for premix users
    }

    // Base oil injection rate
    uint8_t base_rate;

    if (throttle < 20) {
        // Idle/light throttle
        base_rate = omp_config.idle_rate;
    } else if (throttle < 80) {
        // Cruise - interpolate between idle and cruise rates
        uint8_t blend = (throttle - 20) * 100 / 60;  // 0-100%
        base_rate = omp_config.idle_rate +
                   ((omp_config.cruise_rate - omp_config.idle_rate) * blend / 100);
    } else {
        // WOT - interpolate between cruise and WOT rates
        uint8_t blend = (throttle - 80) * 100 / 20;  // 0-100%
        base_rate = omp_config.cruise_rate +
                   ((omp_config.wot_rate - omp_config.cruise_rate) * blend / 100);
    }

    // Increase injection at high RPM (apex seals need more lubrication)
    if (rpm > omp_config.rpm_threshold) {
        uint16_t rpm_over = rpm - omp_config.rpm_threshold;
        uint8_t rpm_bonus = (rpm_over * 20) / 1000;  // +2% per 100 RPM over threshold
        base_rate = min(100, base_rate + rpm_bonus);
    }

    // Cold engine needs more oil
    if (coolant_temp < 600) {  // 60°C
        base_rate = min(100, base_rate + 15);  // +15% when cold
    }

    return base_rate;
}

// ============================================================================
// DUAL IGNITION SYSTEM
// ============================================================================

uint8_t calculateLeadingTiming(uint16_t rpm, uint8_t throttle, bool knock_detected) {
    // Base timing from config
    uint8_t base_timing;

    if (throttle < 20) {
        base_timing = ignition_config.leading_advance_idle;
    } else if (throttle < 80) {
        // Interpolate idle to cruise
        uint8_t blend = (throttle - 20) * 100 / 60;
        base_timing = ignition_config.leading_advance_idle +
                     ((ignition_config.leading_advance_cruise -
                       ignition_config.leading_advance_idle) * blend / 100);
    } else {
        // Interpolate cruise to WOT
        uint8_t blend = (throttle - 80) * 100 / 20;
        base_timing = ignition_config.leading_advance_cruise +
                     ((ignition_config.leading_advance_wot -
                       ignition_config.leading_advance_cruise) * blend / 100);
    }

    // RPM-based advance (rotaries can handle more advance at high RPM)
    if (rpm > 3000) {
        uint16_t rpm_over = rpm - 3000;
        uint8_t rpm_advance = (rpm_over * 10) / 1000;  // +1° per 100 RPM
        base_timing = min(35, base_timing + rpm_advance);  // Cap at 35° BTDC
    }

    // Knock retard
    if (knock_detected) {
        knock_retard_current = min(MAX_KNOCK_RETARD, knock_retard_current + 2);
        Serial.println("[ROTARY] KNOCK DETECTED! Retarding timing");
    } else {
        // Gradually recover from knock retard
        if (knock_retard_current > 0) {
            knock_retard_current--;
        }
    }

    // Apply knock retard
    if (base_timing > knock_retard_current) {
        base_timing -= knock_retard_current;
    } else {
        base_timing = 0;
    }

    return base_timing;
}

uint8_t calculateTrailingTiming(uint8_t leading_timing, uint16_t rpm) {
    // Trailing spark is typically retarded from leading
    uint8_t trailing = leading_timing;

    if (trailing > ignition_config.trailing_offset) {
        trailing -= ignition_config.trailing_offset;
    } else {
        trailing = 0;
    }

    // Split timing mode: increase trailing offset under high load
    if (ignition_config.split_mode && rpm > 6000) {
        // At very high RPM, retard trailing even more for better flame propagation
        uint8_t extra_offset = (rpm - 6000) / 500;  // +1° per 500 RPM
        if (trailing > extra_offset) {
            trailing -= extra_offset;
        } else {
            trailing = 0;
        }
    }

    return trailing;
}

// ============================================================================
// HEALTH MONITORING
// ============================================================================

HealthStatus monitorApexSeals() {
    // Apex seal health estimation based on:
    // 1. Compression monitoring (requires periodic compression tests)
    // 2. Oil consumption rate
    // 3. Blow-by detection (crankcase pressure)
    // 4. Operating hours

    // Simplified health scoring
    uint16_t health_score = 100;

    // Age-based degradation
    if (health_status.hours_since_rebuild > 500) {
        uint16_t hours_over = health_status.hours_since_rebuild - 500;
        uint8_t age_penalty = min(50, hours_over / 20);  // -1% per 20 hours
        health_score -= age_penalty;
    }

    // Oil consumption penalty
    if (health_status.oil_consumption_ml_100km > 1000) {
        // Excessive oil consumption indicates seal wear
        uint16_t excess = health_status.oil_consumption_ml_100km - 1000;
        uint8_t oil_penalty = min(30, excess / 100);
        health_score -= oil_penalty;
    }

    // Compression-based assessment
    if (health_status.compression_rotor1 < 90 || health_status.compression_rotor2 < 90) {
        health_score -= 20;  // Low compression = poor seal
    }

    health_status.apex_seal_condition = max(0, health_score);

    return health_status;
}

bool checkCoolantSeal() {
    // Coolant seal failure symptoms:
    // 1. Coolant loss with no external leaks
    // 2. White smoke from exhaust (steam)
    // 3. Rapid temperature fluctuations
    // 4. Coolant in oil (milky appearance)

    // TODO: Implement actual sensor readings
    // For now, assume seal is OK
    health_status.coolant_seal_ok = true;

    return health_status.coolant_seal_ok;
}

uint16_t estimateOilConsumption(uint32_t distance_km, uint16_t oil_added_ml) {
    if (distance_km == 0) {
        return health_status.oil_consumption_ml_100km;
    }

    // Calculate consumption rate per 100km
    uint16_t consumption_rate = (oil_added_ml * 100) / distance_km;

    // Update health status with rolling average
    health_status.oil_consumption_ml_100km =
        (health_status.oil_consumption_ml_100km * 3 + consumption_rate) / 4;

    // Log if consumption is abnormal
    if (consumption_rate > 1500) {
        Serial.printf("[ROTARY] WARNING: High oil consumption: %d ml/100km\n", consumption_rate);
        Serial.println("[ROTARY] Check apex seals and oil metering system");
    } else if (consumption_rate < 200) {
        Serial.printf("[ROTARY] WARNING: Low oil consumption: %d ml/100km\n", consumption_rate);
        Serial.println("[ROTARY] Check OMP function - apex seals may not be getting oil!");
    }

    return consumption_rate;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

const HealthStatus* getHealthStatus() {
    return &health_status;
}

void configureOMP(const OMPConfig& config) {
    omp_config = config;
    Serial.println("[ROTARY] OMP configuration updated:");
    Serial.printf("[ROTARY]   Idle: %d%%, Cruise: %d%%, WOT: %d%%\n",
                 config.idle_rate, config.cruise_rate, config.wot_rate);
    Serial.printf("[ROTARY]   Mode: %s\n",
                 config.premix_mode ? "PREMIX (OMP disabled)" : "OMP enabled");
}

void configureIgnition(const IgnitionConfig& config) {
    ignition_config = config;
    Serial.println("[ROTARY] Ignition configuration updated:");
    Serial.printf("[ROTARY]   Leading: %d° (idle), %d° (cruise), %d° (WOT)\n",
                 config.leading_advance_idle,
                 config.leading_advance_cruise,
                 config.leading_advance_wot);
    Serial.printf("[ROTARY]   Trailing offset: %d°, Split mode: %s\n",
                 config.trailing_offset,
                 config.split_mode ? "ON" : "OFF");
}

uint32_t getRecommendedRebuildHours() {
    // Typical 13B-MSP rebuild interval: 80,000-100,000 miles
    // At average 40 mph = 2000-2500 hours
    const uint32_t NOMINAL_REBUILD_HOURS = 2000;

    // Adjust based on health status
    uint32_t recommended = NOMINAL_REBUILD_HOURS;

    if (health_status.apex_seal_condition < 70) {
        recommended = health_status.hours_since_rebuild + 100;  // Soon!
    } else if (health_status.apex_seal_condition < 85) {
        recommended = health_status.hours_since_rebuild + 500;
    }

    return recommended;
}

void recordOperatingHours(float hours) {
    health_status.hours_since_rebuild += (uint32_t)(hours * 100);  // Store in 0.01h units

    // Log milestones
    static uint32_t last_milestone = 0;
    uint32_t current_hours = health_status.hours_since_rebuild / 100;

    if (current_hours / 100 > last_milestone / 100) {  // Every 100 hours
        Serial.printf("[ROTARY] Operating hours: %d (since rebuild)\n", current_hours);
        Serial.printf("[ROTARY] Apex seal health: %d%%\n",
                     health_status.apex_seal_condition);
        Serial.printf("[ROTARY] Recommended rebuild: %d hours\n",
                     getRecommendedRebuildHours());
        last_milestone = current_hours;
    }
}

} // namespace RotaryEngine
