/**
 * @file nitrous_control.cpp
 * @brief Progressive multi-stage nitrous oxide injection implementation
 *
 * @author Created for Phase 5++ Haltech-style features
 * @date 2025-11-16
 */

#include "nitrous_control.h"
#include <Arduino.h>

namespace NitrousControl {

// ============================================================================
// CONFIGURATION
// ============================================================================

static NitrousConfig config;
static NitrousStage stages[MAX_STAGES];
static NitrousStatus status;

// ============================================================================
// INITIALIZATION
// ============================================================================

void init(const NitrousConfig& cfg) {
    config = cfg;

    Serial.println("[Nitrous] Initializing nitrous control system");
    Serial.printf("[Nitrous] Type: %s\n",
                 config.type == SYSTEM_DRY ? "DRY" :
                 config.type == SYSTEM_WET ? "WET" : "DISABLED");

    // Configure arm pin
    if (config.arm_pin > 0) {
        pinMode(config.arm_pin, INPUT_PULLUP);
    }

    // Configure purge pin
    if (config.purge_pin > 0) {
        pinMode(config.purge_pin, OUTPUT);
        digitalWrite(config.purge_pin, LOW);
    }

    // Initialize stages
    for (uint8_t i = 0; i < MAX_STAGES; i++) {
        stages[i] = {};
        stages[i].enabled = false;

        status.stage_active[i] = false;
        status.stage_activation_time[i] = 0;
    }

    // Initialize status
    status = {};
    status.armed = false;
    status.active = false;

    Serial.printf("[Nitrous] Maximum stages: %d\n", MAX_STAGES);
    Serial.printf("[Nitrous] Bottle pressure range: %d-%d PSI\n",
                 config.min_bottle_pressure, config.max_bottle_pressure);
}

// ============================================================================
// MAIN UPDATE
// ============================================================================

void update(uint16_t rpm, uint8_t throttle, int16_t coolant_temp,
            uint16_t boost, uint16_t fuel_pressure) {

    // Check if system is armed
    if (config.arm_pin > 0) {
        bool arm_state = digitalRead(config.arm_pin);
        if (!config.arm_active_high) {
            arm_state = !arm_state;  // Invert logic
        }
        status.armed = arm_state;
    } else {
        status.armed = false;  // No arm switch = always disarmed
    }

    // Check safety interlocks
    bool safety_ok = checkSafetyInterlocks(coolant_temp, fuel_pressure);

    // Update purge
    if (status.purging) {
        uint32_t purge_elapsed = millis() - status.purge_start_time;
        if (purge_elapsed >= config.purge_duration) {
            deactivatePurge();
        }
    }

    // If not armed or safety failed, deactivate all stages
    if (!status.armed || !safety_ok || config.type == SYSTEM_DISABLED) {
        for (uint8_t i = 0; i < MAX_STAGES; i++) {
            if (status.stage_active[i]) {
                // Deactivate stage
                if (stages[i].nitrous_pin > 0) {
                    digitalWrite(stages[i].nitrous_pin, LOW);
                }
                if (stages[i].fuel_pin > 0 && config.type == SYSTEM_WET) {
                    digitalWrite(stages[i].fuel_pin, LOW);
                }

                // Update statistics
                uint32_t active_time = millis() - status.stage_activation_time[i];
                status.total_time += active_time;

                status.stage_active[i] = false;
                status.stage_activation_time[i] = 0;

                Serial.printf("[Nitrous] Stage %d deactivated\n", i + 1);
            }
        }
        status.active = false;
        status.active_stages = 0;
        return;
    }

    // Update each stage
    uint8_t active_count = 0;
    bool any_active = false;

    for (uint8_t i = 0; i < MAX_STAGES; i++) {
        if (!stages[i].enabled) {
            continue;
        }

        bool should_activate = shouldStageActivate(i, rpm, throttle, boost);

        if (should_activate && !status.stage_active[i]) {
            // Check delay from previous stage
            if (i > 0) {
                // Need to wait for delay after previous stage activated
                if (!status.stage_active[i - 1]) {
                    continue;  // Previous stage not active yet
                }

                uint32_t prev_active_time = millis() - status.stage_activation_time[i - 1];
                if (prev_active_time < stages[i].delay_ms) {
                    continue;  // Delay not elapsed yet
                }
            }

            // Activate stage
            if (stages[i].nitrous_pin > 0) {
                digitalWrite(stages[i].nitrous_pin, HIGH);
            }
            if (stages[i].fuel_pin > 0 && config.type == SYSTEM_WET) {
                digitalWrite(stages[i].fuel_pin, HIGH);
            }

            status.stage_active[i] = true;
            status.stage_activation_time[i] = millis();
            status.total_shots++;

            Serial.printf("[Nitrous] Stage %d activated (%d HP)\n",
                         i + 1, stages[i].hp_rating);
        } else if (!should_activate && status.stage_active[i]) {
            // Check minimum active time
            uint32_t active_time = millis() - status.stage_activation_time[i];
            if (active_time < stages[i].min_active_time) {
                // Keep active for minimum time
            } else {
                // Deactivate stage
                if (stages[i].nitrous_pin > 0) {
                    digitalWrite(stages[i].nitrous_pin, LOW);
                }
                if (stages[i].fuel_pin > 0 && config.type == SYSTEM_WET) {
                    digitalWrite(stages[i].fuel_pin, LOW);
                }

                // Update statistics
                status.total_time += active_time;

                status.stage_active[i] = false;
                status.stage_activation_time[i] = 0;

                Serial.printf("[Nitrous] Stage %d deactivated\n", i + 1);
            }
        }

        if (status.stage_active[i]) {
            active_count++;
            any_active = true;
        }
    }

    status.active_stages = active_count;
    status.active = any_active;
}

// ============================================================================
// STAGE CONFIGURATION
// ============================================================================

void configureStage(uint8_t stage_index, const NitrousStage& stage) {
    if (stage_index >= MAX_STAGES) {
        Serial.printf("[Nitrous] ERROR: Invalid stage %d\n", stage_index);
        return;
    }

    stages[stage_index] = stage;

    // Configure pins
    if (stage.nitrous_pin > 0) {
        pinMode(stage.nitrous_pin, OUTPUT);
        digitalWrite(stage.nitrous_pin, LOW);
    }

    if (stage.fuel_pin > 0 && config.type == SYSTEM_WET) {
        pinMode(stage.fuel_pin, OUTPUT);
        digitalWrite(stage.fuel_pin, LOW);
    }

    Serial.printf("[Nitrous] Stage %d configured: %s (%d HP)\n",
                 stage_index + 1, stage.name, stage.hp_rating);
    Serial.printf("[Nitrous]   RPM window: %d-%d\n",
                 stage.rpm_activate,
                 stage.rpm_deactivate > 0 ? stage.rpm_deactivate : 9999);
    Serial.printf("[Nitrous]   Min TPS: %d%%\n", stage.tps_min);
    if (stage.delay_ms > 0) {
        Serial.printf("[Nitrous]   Delay: %d ms\n", stage.delay_ms);
    }
}

// ============================================================================
// ACTIVATION LOGIC
// ============================================================================

bool shouldStageActivate(uint8_t stage_index, uint16_t rpm,
                        uint8_t throttle, uint16_t boost) {
    if (stage_index >= MAX_STAGES) {
        return false;
    }

    const NitrousStage& stage = stages[stage_index];

    if (!stage.enabled) {
        return false;
    }

    // Check RPM window
    if (rpm < stage.rpm_activate) {
        return false;
    }

    if (stage.rpm_deactivate > 0 && rpm > stage.rpm_deactivate) {
        return false;
    }

    // Check TPS
    if (throttle < stage.tps_min) {
        return false;
    }

    // Check global TPS minimum
    if (throttle < config.min_tps) {
        return false;
    }

    // Check boost limit (if set)
    if (stage.boost_max > 0 && boost > stage.boost_max) {
        return false;
    }

    return true;
}

// ============================================================================
// PURGE CONTROL
// ============================================================================

void activatePurge() {
    if (config.purge_pin == 0) {
        Serial.println("[Nitrous] ERROR: No purge pin configured");
        return;
    }

    if (status.purging) {
        Serial.println("[Nitrous] Purge already active");
        return;
    }

    digitalWrite(config.purge_pin, HIGH);
    status.purging = true;
    status.purge_start_time = millis();

    Serial.printf("[Nitrous] Purge activated (%d ms)\n", config.purge_duration);
}

void deactivatePurge() {
    if (config.purge_pin > 0) {
        digitalWrite(config.purge_pin, LOW);
    }

    status.purging = false;
    Serial.println("[Nitrous] Purge deactivated");
}

// ============================================================================
// SAFETY CHECKS
// ============================================================================

bool isArmed() {
    return status.armed;
}

bool checkSafetyInterlocks(int16_t coolant_temp, uint16_t fuel_pressure) {
    bool all_ok = true;

    // Coolant temperature check
    status.coolant_temp_ok = (coolant_temp >= config.min_coolant_temp &&
                              coolant_temp <= config.max_coolant_temp);
    if (!status.coolant_temp_ok) {
        all_ok = false;
    }

    // Fuel pressure check
    status.fuel_pressure_ok = (fuel_pressure >= config.min_fuel_pressure);
    if (!status.fuel_pressure_ok) {
        all_ok = false;
    }

    // Bottle pressure check (simulated - would need sensor)
    // For now, assume OK
    status.bottle_pressure_ok = true;
    status.bottle_pressure = 1000;  // PSI (would read from sensor)

    if (status.bottle_pressure < config.min_bottle_pressure ||
        status.bottle_pressure > config.max_bottle_pressure) {
        status.bottle_pressure_ok = false;
        all_ok = false;
    }

    return all_ok;
}

void emergencyShutdown() {
    Serial.println("[Nitrous] !!! EMERGENCY SHUTDOWN !!!");

    for (uint8_t i = 0; i < MAX_STAGES; i++) {
        if (stages[i].nitrous_pin > 0) {
            digitalWrite(stages[i].nitrous_pin, LOW);
        }
        if (stages[i].fuel_pin > 0) {
            digitalWrite(stages[i].fuel_pin, LOW);
        }

        status.stage_active[i] = false;
    }

    if (config.purge_pin > 0) {
        digitalWrite(config.purge_pin, LOW);
    }

    status.active = false;
    status.active_stages = 0;
    status.purging = false;
}

// ============================================================================
// ACCESSORS
// ============================================================================

const NitrousStatus* getStatus() {
    return &status;
}

uint16_t getTotalHP() {
    uint16_t total = 0;

    for (uint8_t i = 0; i < MAX_STAGES; i++) {
        if (status.stage_active[i]) {
            total += stages[i].hp_rating;
        }
    }

    return total;
}

uint16_t calculateFuelEnrichment() {
    // For dry systems, calculate fuel enrichment needed
    // Nitrous adds ~10% fuel requirement per 50 HP

    if (config.type != SYSTEM_DRY) {
        return 0;  // Wet system handles fuel via solenoids
    }

    uint16_t total_hp = getTotalHP();

    // 50 HP = ~10% fuel enrichment
    // 100 HP = ~20% fuel enrichment
    // etc.

    uint16_t enrichment = (total_hp * 10) / 50;  // % * 10

    return enrichment;
}

int8_t calculateTimingRetard() {
    // Retard timing for safety with nitrous
    // ~2° per 50 HP

    uint16_t total_hp = getTotalHP();

    int8_t retard = (total_hp * 2) / 50;

    // Clamp to reasonable range (0-10°)
    return constrain(retard, 0, 10);
}

// ============================================================================
// PRESET CONFIGURATIONS
// ============================================================================

NitrousConfig create3StageStreetConfig() {
    NitrousConfig cfg = {};

    cfg.type = SYSTEM_DRY;  // Dry system (ECU adds fuel)

    // Safety limits
    cfg.min_coolant_temp = 600;   // 60°C minimum (warm engine)
    cfg.max_coolant_temp = 1050;  // 105°C maximum
    cfg.min_tps = 80;             // 80% TPS minimum (near WOT)
    cfg.min_fuel_pressure = 400;  // 40 PSI minimum
    cfg.min_bottle_pressure = 900;  // 900 PSI
    cfg.max_bottle_pressure = 1100; // 1100 PSI

    // Purge
    cfg.purge_pin = 10;           // Example pin
    cfg.purge_duration = 1000;    // 1 second purge

    // Arm switch
    cfg.arm_pin = 11;             // Example pin
    cfg.arm_active_high = true;

    return cfg;
}

NitrousConfig create6StageDragConfig() {
    NitrousConfig cfg = {};

    cfg.type = SYSTEM_WET;  // Wet system (has fuel solenoids)

    // Safety limits (more aggressive)
    cfg.min_coolant_temp = 700;   // 70°C minimum
    cfg.max_coolant_temp = 1100;  // 110°C maximum
    cfg.min_tps = 90;             // 90% TPS minimum (WOT)
    cfg.min_fuel_pressure = 500;  // 50 PSI minimum
    cfg.min_bottle_pressure = 900;
    cfg.max_bottle_pressure = 1100;

    // Purge
    cfg.purge_pin = 10;
    cfg.purge_duration = 2000;    // 2 second purge

    // Arm switch
    cfg.arm_pin = 11;
    cfg.arm_active_high = true;

    return cfg;
}

NitrousConfig createSingleStageConfig() {
    NitrousConfig cfg = {};

    cfg.type = SYSTEM_DRY;

    // Safety limits (conservative)
    cfg.min_coolant_temp = 600;
    cfg.max_coolant_temp = 1000;  // 100°C max
    cfg.min_tps = 75;             // 75% TPS
    cfg.min_fuel_pressure = 350;  // 35 PSI
    cfg.min_bottle_pressure = 900;
    cfg.max_bottle_pressure = 1100;

    // Purge
    cfg.purge_pin = 10;
    cfg.purge_duration = 1000;

    // Arm switch
    cfg.arm_pin = 11;
    cfg.arm_active_high = true;

    return cfg;
}

} // namespace NitrousControl
