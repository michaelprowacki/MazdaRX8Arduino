/**
 * @file torque_management.cpp
 * @brief Advanced torque management implementation
 *
 * @author Created for Phase 5++ Haltech-style features
 * @date 2025-11-16
 */

#include "torque_management.h"
#include <Arduino.h>

namespace TorqueManagement {

// ============================================================================
// CONFIGURATION
// ============================================================================

static TorqueConfig config;
static TorqueStatus status;

// RX8 6-speed gear ratios (for driveshaft RPM calculation)
const float GEAR_RATIOS[7] = {
    0.0f,    // Neutral
    3.815f,  // 1st gear
    2.260f,  // 2nd gear
    1.640f,  // 3rd gear
    1.187f,  // 4th gear
    1.000f,  // 5th gear
    0.843f   // 6th gear
};

// Previous values for shift detection
static uint16_t prev_rpm = 0;
static uint8_t prev_gear = 0;

// ============================================================================
// INITIALIZATION
// ============================================================================

void init(const TorqueConfig& cfg) {
    config = cfg;

    Serial.println("[TorqueMgmt] Initializing torque management");
    Serial.printf("[TorqueMgmt] Mode: %d\n", config.mode);
    Serial.printf("[TorqueMgmt] Max driveshaft RPM: %d\n", config.max_driveshaft_rpm);
    Serial.printf("[TorqueMgmt] Torque limits by gear:\n");
    for (uint8_t i = 1; i <= 6; i++) {
        Serial.printf("[TorqueMgmt]   Gear %d: %d lb-ft\n",
                     i, config.torque_limit_gear[i]);
    }

    // Initialize status
    status = {};
    status.active = false;
    status.current_mode = config.mode;
}

// ============================================================================
// MAIN UPDATE
// ============================================================================

void update(uint16_t rpm, uint8_t throttle, uint8_t gear,
            uint16_t boost, uint16_t wheel_slip) {

    // Skip if disabled
    if (config.mode == MODE_DISABLED) {
        status.active = false;
        status.timing_adjustment = 0;
        status.boost_adjustment = 0;
        return;
    }

    // Estimate current torque
    status.current_torque = estimateTorque(rpm, boost, throttle);

    // Calculate driveshaft RPM
    status.driveshaft_rpm = calculateDriveshaftRPM(rpm, gear);

    // Detect shifts
    if (detectShift(rpm, gear)) {
        activateShiftReduction();
    }

    // Check if shift reduction is active
    if (status.shifting) {
        uint32_t shift_elapsed = millis() - status.shift_start_time;
        if (shift_elapsed >= config.shift_duration) {
            deactivateShiftReduction();
        }
    }

    // Calculate torque limit based on mode
    status.torque_limit = calculateTorqueLimit(gear, status.driveshaft_rpm,
                                               wheel_slip);

    // Calculate target torque
    if (config.mode == MODE_PROGRESSIVE) {
        // Progressive mode: ramp up to target
        uint16_t ramp_increment = (config.ramp_rate * 10) / 1000;  // per 10ms update

        if (status.target_torque < status.current_torque) {
            status.target_torque += ramp_increment;
            status.target_torque = min(status.target_torque, status.current_torque);
        } else {
            status.target_torque = status.current_torque;
        }

        // Clamp to torque limit
        status.target_torque = min(status.target_torque, status.torque_limit);
    } else {
        // Other modes: use torque limit directly
        status.target_torque = min(status.current_torque, status.torque_limit);
    }

    // Calculate torque reduction needed
    if (status.current_torque > status.torque_limit) {
        // Calculate reduction percentage
        uint16_t excess_torque = status.current_torque - status.torque_limit;
        status.torque_reduction = (excess_torque * 100) / status.current_torque;

        // Clamp to 0-100%
        status.torque_reduction = min(status.torque_reduction, 100);

        // Apply shift reduction if active
        if (status.shifting) {
            status.torque_reduction = max(status.torque_reduction,
                                         config.shift_reduction);
        }

        status.active = true;
        status.limit_events++;

        // Calculate control outputs
        status.timing_adjustment = calculateTimingRetard(status.torque_reduction);
        status.boost_adjustment = calculateBoostReduction(status.torque_reduction, boost);

        if (status.limit_events % 50 == 1) {  // Log every 50th event
            Serial.printf("[TorqueMgmt] Limiting: %d -> %d lb-ft (%d%% reduction)\n",
                         status.current_torque, status.torque_limit,
                         status.torque_reduction);
            Serial.printf("[TorqueMgmt]   Timing: %d°, Boost: %.1f PSI\n",
                         status.timing_adjustment, status.boost_adjustment / 10.0f);
        }
    } else {
        // No limiting needed
        status.torque_reduction = 0;
        status.timing_adjustment = 0;
        status.boost_adjustment = 0;

        if (status.active) {
            status.active = false;
        }
    }

    // Update previous values for shift detection
    prev_rpm = rpm;
    prev_gear = gear;
}

// ============================================================================
// TORQUE ESTIMATION
// ============================================================================

uint16_t estimateTorque(uint16_t rpm, uint16_t boost, uint8_t throttle) {
    // Simplified torque estimation for turbocharged rotary engine
    // Real implementation would use detailed torque tables

    // Base torque curve (naturally aspirated 13B-MSP)
    // Peak torque ~159 lb-ft @ 5500 RPM
    uint16_t base_torque;

    if (rpm < 2000) {
        base_torque = 100;  // Low torque at idle
    } else if (rpm < 5500) {
        // Rising to peak
        base_torque = 100 + ((rpm - 2000) * 59) / 3500;
    } else if (rpm < 7500) {
        // At/near peak
        base_torque = 159;
    } else {
        // Falling off
        base_torque = 159 - ((rpm - 7500) * 30) / 2000;
    }

    // Boost multiplier
    // Every PSI of boost adds ~8-10 lb-ft (conservative estimate)
    uint16_t boost_psi = boost / 10;
    uint16_t boost_torque = boost_psi * 9;  // 9 lb-ft per PSI

    // Total torque
    uint16_t total_torque = base_torque + boost_torque;

    // Throttle scaling
    total_torque = (total_torque * throttle) / 100;

    return total_torque;
}

// ============================================================================
// DRIVESHAFT RPM CALCULATION
// ============================================================================

uint16_t calculateDriveshaftRPM(uint16_t engine_rpm, uint8_t gear) {
    if (gear == 0 || gear > 6) {
        return 0;  // Neutral or invalid gear
    }

    // Driveshaft RPM = Engine RPM / (Gear Ratio * Final Drive Ratio)
    float gear_ratio = GEAR_RATIOS[gear];
    float total_ratio = gear_ratio * config.final_drive_ratio;

    uint16_t driveshaft_rpm = engine_rpm / total_ratio;

    return driveshaft_rpm;
}

// ============================================================================
// TORQUE LIMIT CALCULATION
// ============================================================================

uint16_t calculateTorqueLimit(uint8_t gear, uint16_t driveshaft_rpm,
                              uint16_t wheel_slip) {
    uint16_t limit = config.max_torque;  // Start with max

    // Apply gear-based limit
    if (gear > 0 && gear <= 6) {
        limit = min(limit, config.torque_limit_gear[gear]);
    }

    // Apply driveshaft RPM limit
    if (driveshaft_rpm > config.max_driveshaft_rpm) {
        // Reduce torque if driveshaft RPM too high
        uint16_t rpm_reduction = ((driveshaft_rpm - config.max_driveshaft_rpm) * 100) /
                                 config.max_driveshaft_rpm;
        rpm_reduction = min(rpm_reduction, 50);  // Max 50% reduction

        limit = (limit * (100 - rpm_reduction)) / 100;
    }

    // Apply traction-based reduction (if enabled)
    if (config.integrate_traction_control && wheel_slip > 0) {
        // Reduce torque based on wheel slip
        uint16_t slip_percent = wheel_slip / 10;  // Convert to %
        uint16_t slip_reduction = slip_percent * config.slip_torque_reduction;
        slip_reduction = min(slip_reduction, 50);  // Max 50% reduction

        limit = (limit * (100 - slip_reduction)) / 100;
    }

    // Clamp to min/max
    limit = constrain(limit, config.min_torque, config.max_torque);

    return limit;
}

// ============================================================================
// CONTROL CALCULATIONS
// ============================================================================

int8_t calculateTimingRetard(uint16_t torque_reduction) {
    // Calculate timing retard needed to achieve torque reduction
    // Rough approximation: 1° retard = ~2-3% torque reduction

    int8_t retard = (torque_reduction * config.timing_authority) / 100;

    // Clamp to configured authority
    retard = min(retard, config.timing_authority);

    return retard;
}

uint16_t calculateBoostReduction(uint16_t torque_reduction, uint16_t current_boost) {
    // Calculate boost reduction needed to achieve torque reduction
    // Rough approximation: 1 PSI reduction = ~3% torque reduction

    uint16_t boost_psi = current_boost / 10;
    uint16_t reduction_psi = (torque_reduction * config.boost_authority) / 100;

    // Clamp to configured authority and current boost
    reduction_psi = min(reduction_psi, config.boost_authority);
    reduction_psi = min(reduction_psi, boost_psi);

    return reduction_psi * 10;  // Return in PSI * 10
}

// ============================================================================
// SHIFT DETECTION
// ============================================================================

bool detectShift(uint16_t rpm, uint8_t gear) {
    // Detect shift by:
    // 1. Gear change detected
    // 2. RPM drop > 500 (typical shift drop)

    if (gear != prev_gear && gear > 0 && prev_gear > 0) {
        // Gear changed
        int16_t rpm_delta = rpm - prev_rpm;
        if (rpm_delta < -500) {
            // Upshift detected (RPM dropped)
            Serial.printf("[TorqueMgmt] Upshift detected: %d -> %d\n",
                         prev_gear, gear);
            return true;
        }
    }

    return false;
}

void activateShiftReduction() {
    if (config.mode != MODE_SHIFT && config.shift_reduction == 0) {
        return;  // Shift reduction not configured
    }

    status.shifting = true;
    status.shift_start_time = millis();

    Serial.printf("[TorqueMgmt] Shift torque reduction activated (%d%%, %d ms)\n",
                 config.shift_reduction, config.shift_duration);
}

void deactivateShiftReduction() {
    status.shifting = false;
    Serial.println("[TorqueMgmt] Shift torque reduction deactivated");
}

// ============================================================================
// CONTROL
// ============================================================================

void setMode(TorqueMode mode) {
    config.mode = mode;
    status.current_mode = mode;

    Serial.printf("[TorqueMgmt] Mode set to: %d\n", mode);
}

const TorqueStatus* getStatus() {
    return &status;
}

// ============================================================================
// PRESET CONFIGURATIONS
// ============================================================================

TorqueConfig createDrivelineProtectionConfig() {
    TorqueConfig cfg = {};

    cfg.mode = MODE_DRIVELINE;

    // Gear-based torque limits (RX8 driveline protection)
    cfg.torque_limit_gear[0] = 0;     // Neutral
    cfg.torque_limit_gear[1] = 250;   // 1st: 250 lb-ft (prevent wheelspin)
    cfg.torque_limit_gear[2] = 300;   // 2nd: 300 lb-ft
    cfg.torque_limit_gear[3] = 350;   // 3rd: 350 lb-ft
    cfg.torque_limit_gear[4] = 400;   // 4th: 400 lb-ft
    cfg.torque_limit_gear[5] = 400;   // 5th: 400 lb-ft
    cfg.torque_limit_gear[6] = 400;   // 6th: 400 lb-ft

    // Driveshaft RPM limiting
    cfg.max_driveshaft_rpm = 4500;    // Safe limit for RX8 driveshaft
    cfg.final_drive_ratio = 4.44f;    // RX8 final drive

    // Progressive delivery
    cfg.ramp_rate = 200;              // 200 lb-ft/sec ramp
    cfg.min_torque = 100;             // Minimum
    cfg.max_torque = 400;             // Maximum

    // Shift torque reduction
    cfg.shift_reduction = 30;         // 30% reduction during shift
    cfg.shift_duration = 200;         // 200ms shift duration

    // Traction control integration
    cfg.integrate_traction_control = false;  // Not needed for driveline mode
    cfg.slip_torque_reduction = 0;

    // Control method
    cfg.timing_authority = 10;        // Up to 10° timing retard
    cfg.boost_authority = 5;          // Up to 5 PSI boost reduction

    return cfg;
}

TorqueConfig createTractionOptimizationConfig() {
    TorqueConfig cfg = {};

    cfg.mode = MODE_TRACTION;

    // Gear-based torque limits (optimized for traction)
    cfg.torque_limit_gear[0] = 0;
    cfg.torque_limit_gear[1] = 220;   // 1st: Lower for traction
    cfg.torque_limit_gear[2] = 280;   // 2nd: Lower for traction
    cfg.torque_limit_gear[3] = 350;
    cfg.torque_limit_gear[4] = 400;
    cfg.torque_limit_gear[5] = 400;
    cfg.torque_limit_gear[6] = 400;

    // Driveshaft RPM limiting
    cfg.max_driveshaft_rpm = 4500;
    cfg.final_drive_ratio = 4.44f;

    // Progressive delivery (slower for traction)
    cfg.ramp_rate = 150;              // 150 lb-ft/sec (slower)
    cfg.min_torque = 100;
    cfg.max_torque = 400;

    // Shift torque reduction
    cfg.shift_reduction = 40;         // 40% reduction (smoother shifts)
    cfg.shift_duration = 250;

    // Traction control integration (ENABLED)
    cfg.integrate_traction_control = true;
    cfg.slip_torque_reduction = 3;    // 3% torque reduction per 1% slip

    // Control method
    cfg.timing_authority = 12;        // Up to 12° timing retard
    cfg.boost_authority = 5;

    return cfg;
}

TorqueConfig createDragRacingConfig() {
    TorqueConfig cfg = {};

    cfg.mode = MODE_PROGRESSIVE;

    // Gear-based torque limits (drag racing)
    cfg.torque_limit_gear[0] = 0;
    cfg.torque_limit_gear[1] = 240;   // 1st: Controlled launch
    cfg.torque_limit_gear[2] = 320;   // 2nd: Progressive
    cfg.torque_limit_gear[3] = 380;   // 3rd: Near max
    cfg.torque_limit_gear[4] = 400;   // 4th: Maximum
    cfg.torque_limit_gear[5] = 400;
    cfg.torque_limit_gear[6] = 400;

    // Driveshaft RPM limiting
    cfg.max_driveshaft_rpm = 5000;    // Higher for drag (aggressive)
    cfg.final_drive_ratio = 4.44f;

    // Progressive delivery (optimized for 1/4 mile)
    cfg.ramp_rate = 250;              // 250 lb-ft/sec (fast but controlled)
    cfg.min_torque = 150;
    cfg.max_torque = 400;

    // Shift torque reduction (minimal for drag)
    cfg.shift_reduction = 20;         // 20% reduction (quick shifts)
    cfg.shift_duration = 150;         // 150ms (fast shifts)

    // Traction control integration
    cfg.integrate_traction_control = true;
    cfg.slip_torque_reduction = 5;    // 5% reduction per 1% slip (aggressive TC)

    // Control method
    cfg.timing_authority = 10;
    cfg.boost_authority = 3;          // Limited boost reduction (want power)

    return cfg;
}

} // namespace TorqueManagement
