/**
 * @file water_meth_injection.cpp
 * @brief Water/methanol injection implementation
 *
 * @author Created for Phase 5++ Haltech-style features
 * @date 2025-11-16
 */

#include "water_meth_injection.h"
#include <Arduino.h>

namespace WaterMethInjection {

// ============================================================================
// CONFIGURATION
// ============================================================================

static WMIConfig config = createDefault2StageConfig();

static WMIStatus status = {
    .active = false,
    .active_stage = 255,
    .pump_duty = 0,
    .current_boost = 0,
    .tank_level_ml = 3000,          // Assume full tank (3L typical)
    .tank_level_percent = 100,
    .low_level_warning = false,
    .flow_fault = false,
    .failsafe_active = false,
    .total_injected_ml = 0,
    .current_flow_rate = 0
};

// Pump control state
static uint8_t target_pump_duty = 0;
static uint8_t actual_pump_duty = 0;
static const uint8_t PUMP_RAMP_RATE = 10;  // % per update (prevents pressure spikes)

// Flow monitoring
static uint32_t last_flow_check_ms = 0;
static uint16_t flow_samples[10] = {0};
static uint8_t flow_sample_index = 0;

// Tank level estimation (flow-based if no level sensor)
static uint32_t last_tank_update_ms = 0;

// Manual override
static bool pump_override_enabled = false;
static uint8_t pump_override_duty = 0;

// ============================================================================
// INITIALIZATION
// ============================================================================

void init() {
    Serial.println("[WMI] Water/methanol injection initialized");
    Serial.printf("[WMI] Status: %s\n", config.enabled ? "ENABLED" : "DISABLED");
    Serial.printf("[WMI] Mixture: %s\n",
                 config.mixture_type == MIXTURE_50_50 ? "50/50" :
                 config.mixture_type == MIXTURE_100_METH ? "100% Methanol" :
                 config.mixture_type == MIXTURE_100_WATER ? "100% Water" :
                 config.mixture_type == MIXTURE_30_70 ? "30/70" : "Custom");
    Serial.printf("[WMI] Stages: %d\n", config.num_stages);

    for (uint8_t i = 0; i < config.num_stages; i++) {
        Serial.printf("[WMI]   Stage %d: %.1f-%.1f PSI, %d-%d%% duty\n",
                     i + 1,
                     config.stages[i].boost_start / 10.0f,
                     config.stages[i].boost_end / 10.0f,
                     config.stages[i].pump_duty_min,
                     config.stages[i].pump_duty_max);
    }

    Serial.printf("[WMI] Tank capacity: %d ml\n", config.tank_capacity_ml);
    Serial.printf("[WMI] Failsafe: %s\n",
                 config.failsafe_enabled ? "ENABLED" : "DISABLED");

    if (config.failsafe_enabled) {
        Serial.println("[WMI] WARNING: Boost will be limited if injection fails!");
    }

    last_flow_check_ms = millis();
    last_tank_update_ms = millis();
}

// ============================================================================
// MAIN UPDATE
// ============================================================================

uint8_t update(uint16_t boost, uint8_t throttle, int16_t iat) {
    status.current_boost = boost;

    if (!config.enabled) {
        status.active = false;
        status.pump_duty = 0;
        return 0;
    }

    // Check failsafe conditions
    if (checkFailsafe()) {
        status.failsafe_active = true;
        status.active = false;
        target_pump_duty = 0;

        Serial.println("[WMI] FAILSAFE ACTIVE - Injection disabled!");

        // External boost control should read status.failsafe_active
        // and reduce boost to config.failsafe_boost_limit
    } else {
        status.failsafe_active = false;
    }

    // Manual override
    if (pump_override_enabled) {
        target_pump_duty = pump_override_duty;
        status.active = (target_pump_duty > 0);
    } else {
        // Calculate pump duty based on boost
        target_pump_duty = calculatePumpDuty(boost);
        status.active = (target_pump_duty > 0);
    }

    // Determine active stage
    status.active_stage = determineStage(boost);

    // Gradually ramp pump duty (prevents pressure spikes)
    if (target_pump_duty > actual_pump_duty) {
        actual_pump_duty = min(target_pump_duty, actual_pump_duty + PUMP_RAMP_RATE);
    } else if (target_pump_duty < actual_pump_duty) {
        actual_pump_duty = max(target_pump_duty, actual_pump_duty - PUMP_RAMP_RATE);
    }

    status.pump_duty = actual_pump_duty;

    // Monitor tank level
    status.tank_level_ml = monitorTankLevel();
    status.tank_level_percent = (status.tank_level_ml * 100) / config.tank_capacity_ml;
    status.low_level_warning = (status.tank_level_ml < config.low_level_threshold_ml);

    // Monitor flow rate (every 1 second)
    if ((millis() - last_flow_check_ms) > 1000) {
        if (config.flow_monitoring_enabled) {
            status.flow_fault = !monitorFlowRate(actual_pump_duty);
        }
        last_flow_check_ms = millis();
    }

    // Output pump duty to PWM pin
    // TODO: analogWrite(WMI_PUMP_PIN, actual_pump_duty * 255 / 100);

    // Control solenoids for active stage
    if (status.active && status.active_stage < config.num_stages) {
        if (config.stages[status.active_stage].solenoid_active) {
            // TODO: digitalWrite(WMI_SOLENOID_PIN[status.active_stage], HIGH);
        }
    } else {
        // TODO: Turn off all solenoids
    }

    return actual_pump_duty;
}

// ============================================================================
// PUMP DUTY CALCULATION
// ============================================================================

uint8_t calculatePumpDuty(uint16_t boost) {
    // Find active stage
    uint8_t stage_idx = determineStage(boost);

    if (stage_idx >= config.num_stages) {
        return 0;  // No injection below boost threshold
    }

    const InjectionStage& stage = config.stages[stage_idx];

    // Interpolate duty cycle within this stage
    if (boost <= stage.boost_start) {
        return stage.pump_duty_min;
    } else if (boost >= stage.boost_end) {
        return stage.pump_duty_max;
    } else {
        // Linear interpolation
        uint16_t boost_range = stage.boost_end - stage.boost_start;
        uint16_t boost_offset = boost - stage.boost_start;
        uint8_t duty_range = stage.pump_duty_max - stage.pump_duty_min;

        uint8_t duty = stage.pump_duty_min +
                      ((duty_range * boost_offset) / boost_range);

        return duty;
    }
}

uint8_t determineStage(uint16_t boost) {
    // Find the highest stage whose boost_start threshold is met
    uint8_t active_stage = 255;  // No stage

    for (uint8_t i = 0; i < config.num_stages; i++) {
        if (boost >= config.stages[i].boost_start) {
            active_stage = i;
        } else {
            break;  // Stages should be in ascending order
        }
    }

    return active_stage;
}

// ============================================================================
// TANK LEVEL MONITORING
// ============================================================================

uint16_t monitorTankLevel() {
    // Method 1: Analog level sensor
    // TODO: Read analog tank level sensor if available
    // uint16_t sensor_value = analogRead(WMI_TANK_LEVEL_PIN);
    // uint16_t level_ml = map(sensor_value, 0, 1023, 0, config.tank_capacity_ml);
    // return level_ml;

    // Method 2: Flow-based estimation (if no level sensor)
    uint32_t now = millis();
    uint32_t dt_ms = now - last_tank_update_ms;

    if (dt_ms > 1000 && status.active) {  // Update every second
        // Estimate consumption based on pump duty and expected flow rate
        float duty_fraction = status.pump_duty / 100.0f;
        float flow_per_minute = config.expected_flow_rate * duty_fraction;
        float flow_per_second = flow_per_minute / 60.0f;
        float dt_seconds = dt_ms / 1000.0f;

        uint16_t consumed_ml = (uint16_t)(flow_per_second * dt_seconds);

        // Update total injected
        status.total_injected_ml += consumed_ml;

        // Update tank level
        if (status.tank_level_ml > consumed_ml) {
            status.tank_level_ml -= consumed_ml;
        } else {
            status.tank_level_ml = 0;
        }

        last_tank_update_ms = now;
    }

    return status.tank_level_ml;
}

// ============================================================================
// FLOW MONITORING
// ============================================================================

bool monitorFlowRate(uint8_t pump_duty) {
    // Expected flow rate at current duty
    float duty_fraction = pump_duty / 100.0f;
    uint16_t expected_flow = (uint16_t)(config.expected_flow_rate * duty_fraction);

    // TODO: Measure actual flow rate (requires flow sensor)
    // For now, assume flow is normal
    uint16_t actual_flow = expected_flow;  // Placeholder

    // Store sample for averaging
    flow_samples[flow_sample_index] = actual_flow;
    flow_sample_index = (flow_sample_index + 1) % 10;

    // Calculate average flow
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 10; i++) {
        sum += flow_samples[i];
    }
    uint16_t avg_flow = sum / 10;
    status.current_flow_rate = avg_flow;

    // Check if flow is within acceptable range (±30%)
    uint16_t flow_min = expected_flow * 70 / 100;
    uint16_t flow_max = expected_flow * 130 / 100;

    if (pump_duty > 10 && (avg_flow < flow_min || avg_flow > flow_max)) {
        Serial.printf("[WMI] Flow fault: Expected %d ml/min, got %d ml/min\n",
                     expected_flow, avg_flow);
        return false;  // Flow fault
    }

    return true;  // Flow normal
}

// ============================================================================
// FAILSAFE
// ============================================================================

bool checkFailsafe() {
    if (!config.failsafe_enabled) {
        return false;
    }

    // Trigger failsafe if:
    // 1. Tank level critically low (< 5%)
    if (status.tank_level_percent < 5) {
        Serial.println("[WMI] Failsafe: Tank nearly empty!");
        return true;
    }

    // 2. Flow fault detected
    if (status.flow_fault) {
        Serial.println("[WMI] Failsafe: Flow fault (clogged nozzle?)");
        return true;
    }

    // 3. Pump duty high but no injection expected
    // (could indicate pump failure)
    if (status.pump_duty > 50 && status.current_flow_rate < 10) {
        Serial.println("[WMI] Failsafe: Pump running but no flow!");
        return true;
    }

    return false;
}

// ============================================================================
// PERFORMANCE CALCULATIONS
// ============================================================================

int8_t calculateTimingAdvance(uint16_t boost, uint8_t pump_duty) {
    // Water/meth injection allows more aggressive timing:
    // - Charge cooling increases knock threshold
    // - Methanol has high octane (109)
    //
    // Conservative: +2-3° at full injection
    // Aggressive: +4-5° at full injection

    if (pump_duty == 0) {
        return 0;  // No advance without injection
    }

    // Max advance based on mixture type
    int8_t max_advance = 3;  // Conservative default

    switch (config.mixture_type) {
        case MIXTURE_100_METH:
            max_advance = 5;  // Methanol has highest octane
            break;
        case MIXTURE_50_50:
            max_advance = 3;  // Balanced
            break;
        case MIXTURE_100_WATER:
            max_advance = 2;  // Water has no octane boost, just cooling
            break;
        case MIXTURE_30_70:
            max_advance = 4;  // High methanol content
            break;
        default:
            max_advance = 3;
    }

    // Scale by pump duty
    int8_t advance = (max_advance * pump_duty) / 100;

    return advance;
}

uint16_t calculateSafeBoostIncrease(uint16_t base_boost, uint8_t pump_duty) {
    // Water/meth injection allows 2-3 PSI more boost safely
    // due to charge cooling and knock suppression

    if (pump_duty < 50) {
        return 0;  // Need significant injection for boost increase
    }

    // Max safe boost increase: 2-3 PSI (20-30 in PSI * 10)
    uint16_t max_increase = 25;  // 2.5 PSI

    // Scale by pump duty (50-100% duty → 0-100% of max increase)
    uint8_t duty_above_threshold = pump_duty - 50;
    uint16_t boost_increase = (max_increase * duty_above_threshold) / 50;

    return boost_increase;
}

int16_t estimateTempReduction(int16_t iat_before, uint8_t pump_duty) {
    // Water/meth injection can reduce intake temps by 30-50°C
    // depending on:
    // - Mixture type (water cools better than methanol)
    // - Flow rate
    // - Ambient conditions

    if (pump_duty == 0) {
        return 0;
    }

    // Max temp reduction based on mixture
    int16_t max_reduction = 400;  // 40°C default

    switch (config.mixture_type) {
        case MIXTURE_100_WATER:
            max_reduction = 500;  // Water has highest cooling
            break;
        case MIXTURE_50_50:
            max_reduction = 400;  // Balanced
            break;
        case MIXTURE_100_METH:
            max_reduction = 300;  // Methanol cools less than water
            break;
        case MIXTURE_30_70:
            max_reduction = 350;  // Moderate cooling
            break;
        default:
            max_reduction = 400;
    }

    // Scale by pump duty
    int16_t temp_reduction = (max_reduction * pump_duty) / 100;

    return temp_reduction;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

const WMIStatus* getStatus() {
    return &status;
}

void configure(const WMIConfig& new_config) {
    config = new_config;
    Serial.println("[WMI] Configuration updated");
    init();
}

void setEnabled(bool enabled) {
    config.enabled = enabled;
    Serial.printf("[WMI] Water/meth injection %s\n",
                 enabled ? "ENABLED" : "DISABLED");

    if (enabled) {
        Serial.println("[WMI] IMPORTANT: Safety checklist:");
        Serial.println("[WMI] - Tank filled with correct mixture");
        Serial.println("[WMI] - All lines purged of air");
        Serial.println("[WMI] - Nozzle(s) not clogged");
        Serial.println("[WMI] - Flow sensor calibrated (if used)");
        Serial.println("[WMI] - Failsafe boost limit set correctly");
        Serial.println("[WMI] - Tune verified with injection active!");
    }
}

void setMixtureType(MixtureType mixture) {
    config.mixture_type = mixture;
    Serial.printf("[WMI] Mixture set to: %s\n",
                 mixture == MIXTURE_50_50 ? "50/50" :
                 mixture == MIXTURE_100_METH ? "100% Methanol" :
                 mixture == MIXTURE_100_WATER ? "100% Water" :
                 mixture == MIXTURE_30_70 ? "30/70" : "Custom");

    if (mixture == MIXTURE_100_WATER) {
        Serial.println("[WMI] WARNING: Use DISTILLED water only!");
        Serial.println("[WMI] WARNING: 100% water provides NO octane boost!");
    }
}

void setPumpDutyOverride(uint8_t duty) {
    pump_override_enabled = true;
    pump_override_duty = constrain(duty, 0, 100);
    Serial.printf("[WMI] Pump duty override: %d%%\n", pump_override_duty);
}

void clearPumpDutyOverride() {
    pump_override_enabled = false;
    Serial.println("[WMI] Pump duty override cleared");
}

void resetTankLevel(uint16_t capacity_ml) {
    status.tank_level_ml = capacity_ml;
    status.tank_level_percent = 100;
    status.total_injected_ml = 0;
    Serial.printf("[WMI] Tank level reset to %d ml\n", capacity_ml);
}

uint16_t getEstimatedRange(uint16_t current_level, uint16_t avg_flow_rate) {
    if (avg_flow_rate == 0) {
        return 0;  // No usage data
    }

    // Minutes remaining = current_level / avg_flow_rate
    uint16_t minutes = current_level / avg_flow_rate;

    return minutes;
}

// ============================================================================
// PRESET CONFIGURATIONS
// ============================================================================

WMIConfig createDefault2StageConfig() {
    WMIConfig cfg;

    cfg.enabled = false;
    cfg.mixture_type = MIXTURE_50_50;
    cfg.num_stages = 2;
    cfg.tank_capacity_ml = 3000;        // 3L tank
    cfg.low_level_threshold_ml = 300;   // 10% warning
    cfg.failsafe_enabled = true;
    cfg.failsafe_boost_limit = 70;      // 7 PSI in failsafe
    cfg.flow_monitoring_enabled = false;  // Requires flow sensor
    cfg.expected_flow_rate = 500;       // 500 ml/min at full duty

    // Stage 1: Light injection (5-10 PSI)
    cfg.stages[0] = {
        .boost_start = 50,      // 5.0 PSI
        .boost_end = 100,       // 10.0 PSI
        .pump_duty_min = 20,    // 20% duty
        .pump_duty_max = 50,    // 50% duty
        .solenoid_active = true
    };

    // Stage 2: Full injection (10+ PSI)
    cfg.stages[1] = {
        .boost_start = 100,     // 10.0 PSI
        .boost_end = 150,       // 15.0 PSI
        .pump_duty_min = 50,    // 50% duty
        .pump_duty_max = 100,   // 100% duty
        .solenoid_active = true
    };

    return cfg;
}

WMIConfig createAggressive4StageConfig() {
    WMIConfig cfg;

    cfg.enabled = false;
    cfg.mixture_type = MIXTURE_30_70;   // Aggressive mixture
    cfg.num_stages = 4;
    cfg.tank_capacity_ml = 5000;        // 5L tank (larger for high consumption)
    cfg.low_level_threshold_ml = 500;
    cfg.failsafe_enabled = true;
    cfg.failsafe_boost_limit = 90;      // 9 PSI in failsafe
    cfg.flow_monitoring_enabled = true;
    cfg.expected_flow_rate = 800;       // 800 ml/min at full duty (high flow)

    // Stage 1: 7-10 PSI
    cfg.stages[0] = {
        .boost_start = 70,
        .boost_end = 100,
        .pump_duty_min = 15,
        .pump_duty_max = 35,
        .solenoid_active = true
    };

    // Stage 2: 10-13 PSI
    cfg.stages[1] = {
        .boost_start = 100,
        .boost_end = 130,
        .pump_duty_min = 35,
        .pump_duty_max = 60,
        .solenoid_active = true
    };

    // Stage 3: 13-16 PSI
    cfg.stages[2] = {
        .boost_start = 130,
        .boost_end = 160,
        .pump_duty_min = 60,
        .pump_duty_max = 80,
        .solenoid_active = true
    };

    // Stage 4: 16+ PSI (max power)
    cfg.stages[3] = {
        .boost_start = 160,
        .boost_end = 200,       // 20 PSI
        .pump_duty_min = 80,
        .pump_duty_max = 100,
        .solenoid_active = true
    };

    return cfg;
}

} // namespace WaterMethInjection
