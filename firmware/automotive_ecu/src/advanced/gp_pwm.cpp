/**
 * @file gp_pwm.cpp
 * @brief General Purpose PWM outputs implementation
 *
 * @author Created for Phase 5++ Haltech-style features
 * @date 2025-11-16
 */

#include "gp_pwm.h"
#include <Arduino.h>

namespace GPPWM {

// ============================================================================
// CONFIGURATION
// ============================================================================

static PWMConfig configs[MAX_PWM_OUTPUTS];
static PWMStatus status[MAX_PWM_OUTPUTS];

// ============================================================================
// INITIALIZATION
// ============================================================================

void init() {
    Serial.println("[GPPWM] Initializing general purpose PWM outputs");

    // Initialize all outputs as disabled
    for (uint8_t i = 0; i < MAX_PWM_OUTPUTS; i++) {
        configs[i] = {};
        configs[i].mode = MODE_DISABLED;
        configs[i].enabled = false;
        configs[i].failsafe_duty = 0;

        status[i] = {};
        status[i].current_duty = 0;
        status[i].target_duty = 0;
    }

    Serial.printf("[GPPWM] %d PWM outputs available\n", MAX_PWM_OUTPUTS);
}

// ============================================================================
// MAIN UPDATE
// ============================================================================

void update(uint16_t rpm, uint8_t throttle, int16_t coolant_temp,
            int16_t oil_temp, uint16_t boost, uint16_t speed,
            uint16_t battery_voltage) {

    for (uint8_t i = 0; i < MAX_PWM_OUTPUTS; i++) {
        // Skip disabled outputs
        if (!configs[i].enabled || configs[i].mode == MODE_DISABLED) {
            status[i].active = false;
            continue;
        }

        // Determine input value based on mode
        int16_t input_value = 0;

        switch (configs[i].mode) {
            case MODE_MANUAL:
                // Manual mode: use target duty directly
                break;

            case MODE_RPM:
                input_value = rpm;
                status[i].target_duty = calculateDuty(i, input_value);
                break;

            case MODE_TPS:
                input_value = throttle;
                status[i].target_duty = calculateDuty(i, input_value);
                break;

            case MODE_COOLANT_TEMP:
                input_value = coolant_temp / 10;  // Convert to °C
                status[i].target_duty = calculateDuty(i, input_value);
                break;

            case MODE_OIL_TEMP:
                input_value = oil_temp / 10;  // Convert to °C
                status[i].target_duty = calculateDuty(i, input_value);
                break;

            case MODE_BOOST:
                input_value = boost / 10;  // Convert to PSI
                status[i].target_duty = calculateDuty(i, input_value);
                break;

            case MODE_SPEED:
                input_value = speed / 10;  // Convert to km/h
                status[i].target_duty = calculateDuty(i, input_value);
                break;

            case MODE_VOLTAGE:
                input_value = battery_voltage / 100;  // Convert to V
                status[i].target_duty = calculateDuty(i, input_value);
                break;

            case MODE_FAULT:
                // Fault mode: on/off based on fault status
                status[i].target_duty = status[i].fault ? 100 : 0;
                break;

            default:
                status[i].target_duty = 0;
                break;
        }

        status[i].input_value = input_value;

        // Apply ramping (gradual change)
        if (configs[i].ramp_rate > 0) {
            int8_t diff = status[i].target_duty - status[i].current_duty;

            if (abs(diff) <= configs[i].ramp_rate) {
                // Within ramp rate: reach target
                status[i].current_duty = status[i].target_duty;
            } else {
                // Gradual approach
                if (diff > 0) {
                    status[i].current_duty += configs[i].ramp_rate;
                } else {
                    status[i].current_duty -= configs[i].ramp_rate;
                }
            }
        } else {
            // Instant change
            status[i].current_duty = status[i].target_duty;
        }

        // Clamp to min/max
        status[i].current_duty = constrain(status[i].current_duty,
                                           configs[i].duty_min,
                                           configs[i].duty_max);

        // Apply to hardware
        uint8_t pwm_value = map(status[i].current_duty, 0, 100, 0, 255);

        // Invert if configured
        if (configs[i].invert) {
            pwm_value = 255 - pwm_value;
        }

        analogWrite(configs[i].pin, pwm_value);

        // Update on-time tracking
        if (status[i].current_duty > 0) {
            status[i].active = true;
            status[i].on_time += 10;  // Assume 10ms update rate
        } else {
            status[i].active = false;
        }
    }
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void configure(uint8_t output, const PWMConfig& config) {
    if (output >= MAX_PWM_OUTPUTS) {
        Serial.printf("[GPPWM] ERROR: Invalid output %d\n", output);
        return;
    }

    configs[output] = config;

    // Configure pin as output
    pinMode(config.pin, OUTPUT);

    Serial.printf("[GPPWM] Output %d configured: %s\n", output, config.name);
    Serial.printf("[GPPWM]   Mode: %d, Pin: %d\n", config.mode, config.pin);
    Serial.printf("[GPPWM]   Range: %d-%d -> %d-%d%%\n",
                 config.input_min, config.input_max,
                 config.duty_min, config.duty_max);
}

void setManualDuty(uint8_t output, uint8_t duty) {
    if (output >= MAX_PWM_OUTPUTS) {
        return;
    }

    if (configs[output].mode != MODE_MANUAL) {
        Serial.printf("[GPPWM] WARNING: Output %d not in manual mode\n", output);
        return;
    }

    duty = constrain(duty, 0, 100);
    status[output].target_duty = duty;

    Serial.printf("[GPPWM] Output %d manual duty set to %d%%\n", output, duty);
}

void setEnabled(uint8_t output, bool enabled) {
    if (output >= MAX_PWM_OUTPUTS) {
        return;
    }

    configs[output].enabled = enabled;

    if (!enabled) {
        // Disable output
        analogWrite(configs[output].pin, 0);
        status[output].current_duty = 0;
        status[output].target_duty = 0;
        status[output].active = false;
    }

    Serial.printf("[GPPWM] Output %d: %s\n", output, enabled ? "ENABLED" : "DISABLED");
}

// ============================================================================
// ACCESSORS
// ============================================================================

const PWMStatus* getStatus(uint8_t output) {
    if (output >= MAX_PWM_OUTPUTS) {
        return nullptr;
    }
    return &status[output];
}

const PWMConfig* getConfig(uint8_t output) {
    if (output >= MAX_PWM_OUTPUTS) {
        return nullptr;
    }
    return &configs[output];
}

// ============================================================================
// DUTY CYCLE CALCULATION
// ============================================================================

uint8_t calculateDuty(uint8_t output, int16_t input_value) {
    if (output >= MAX_PWM_OUTPUTS) {
        return 0;
    }

    const PWMConfig& cfg = configs[output];

    // Clamp input to range
    input_value = constrain(input_value, cfg.input_min, cfg.input_max);

    // Map input to duty cycle
    int16_t duty = map(input_value,
                       cfg.input_min, cfg.input_max,
                       cfg.duty_min, cfg.duty_max);

    // Clamp to 0-100%
    duty = constrain(duty, 0, 100);

    return (uint8_t)duty;
}

// ============================================================================
// FAILSAFE
// ============================================================================

void applyFailsafe() {
    Serial.println("[GPPWM] Applying failsafe to all outputs");

    for (uint8_t i = 0; i < MAX_PWM_OUTPUTS; i++) {
        if (configs[i].enabled) {
            status[i].target_duty = configs[i].failsafe_duty;
            status[i].current_duty = configs[i].failsafe_duty;

            uint8_t pwm_value = map(configs[i].failsafe_duty, 0, 100, 0, 255);
            if (configs[i].invert) {
                pwm_value = 255 - pwm_value;
            }
            analogWrite(configs[i].pin, pwm_value);

            Serial.printf("[GPPWM]   Output %d (%s): %d%%\n",
                         i, configs[i].name, configs[i].failsafe_duty);
        }
    }
}

// ============================================================================
// DIAGNOSTICS
// ============================================================================

void printDiagnostics() {
    Serial.println("[GPPWM] ===== PWM Diagnostics =====");

    for (uint8_t i = 0; i < MAX_PWM_OUTPUTS; i++) {
        if (configs[i].enabled) {
            Serial.printf("[GPPWM] Output %d: %s\n", i, configs[i].name);
            Serial.printf("[GPPWM]   Pin: %d, Mode: %d\n",
                         configs[i].pin, configs[i].mode);
            Serial.printf("[GPPWM]   Input: %d, Duty: %d%% (target: %d%%)\n",
                         status[i].input_value,
                         status[i].current_duty,
                         status[i].target_duty);
            Serial.printf("[GPPWM]   Active: %s, On time: %lu ms\n",
                         status[i].active ? "YES" : "NO",
                         status[i].on_time);
        }
    }

    Serial.println("[GPPWM] ===========================");
}

// ============================================================================
// PRESET CONFIGURATIONS
// ============================================================================

PWMConfig createRadiatorFanConfig(uint8_t pin) {
    PWMConfig cfg = {};

    strcpy(cfg.name, "Radiator Fan");
    cfg.pin = pin;
    cfg.mode = MODE_COOLANT_TEMP;

    // 80°C: 0%, 100°C: 100%
    cfg.input_min = 80;
    cfg.input_max = 100;
    cfg.duty_min = 0;
    cfg.duty_max = 100;

    cfg.frequency = 100;        // 100 Hz
    cfg.failsafe_duty = 100;    // Full on if fault
    cfg.ramp_rate = 5;          // 5% per update (gradual)
    cfg.enabled = true;
    cfg.invert = false;

    return cfg;
}

PWMConfig createFuelPumpStagingConfig(uint8_t pin) {
    PWMConfig cfg = {};

    strcpy(cfg.name, "Fuel Pump Stage 2");
    cfg.pin = pin;
    cfg.mode = MODE_BOOST;

    // Below 10 PSI: Off, Above 10 PSI: On
    cfg.input_min = 10;
    cfg.input_max = 11;  // Sharp transition
    cfg.duty_min = 0;
    cfg.duty_max = 100;

    cfg.frequency = 1000;       // 1 kHz (fast switching)
    cfg.failsafe_duty = 100;    // Full on if fault (safety)
    cfg.ramp_rate = 0;          // Instant on/off
    cfg.enabled = true;
    cfg.invert = false;

    return cfg;
}

PWMConfig createShiftLightConfig(uint8_t pin) {
    PWMConfig cfg = {};

    strcpy(cfg.name, "Shift Light");
    cfg.pin = pin;
    cfg.mode = MODE_RPM;

    // 8500-9000 RPM: Flash (PWM gives visual effect)
    cfg.input_min = 8500;
    cfg.input_max = 9000;
    cfg.duty_min = 0;
    cfg.duty_max = 100;

    cfg.frequency = 5;          // 5 Hz (flashing effect)
    cfg.failsafe_duty = 0;      // Off if fault
    cfg.ramp_rate = 0;          // Instant
    cfg.enabled = true;
    cfg.invert = false;

    return cfg;
}

PWMConfig createOilCoolerFanConfig(uint8_t pin) {
    PWMConfig cfg = {};

    strcpy(cfg.name, "Oil Cooler Fan");
    cfg.pin = pin;
    cfg.mode = MODE_OIL_TEMP;

    // 90°C: 0%, 110°C: 100%
    cfg.input_min = 90;
    cfg.input_max = 110;
    cfg.duty_min = 0;
    cfg.duty_max = 100;

    cfg.frequency = 100;        // 100 Hz
    cfg.failsafe_duty = 100;    // Full on if fault
    cfg.ramp_rate = 5;          // 5% per update
    cfg.enabled = true;
    cfg.invert = false;

    return cfg;
}

PWMConfig createBoostGaugeConfig(uint8_t pin) {
    PWMConfig cfg = {};

    strcpy(cfg.name, "Boost Gauge Driver");
    cfg.pin = pin;
    cfg.mode = MODE_BOOST;

    // 0 PSI: 0%, 20 PSI: 100%
    cfg.input_min = 0;
    cfg.input_max = 20;
    cfg.duty_min = 0;
    cfg.duty_max = 100;

    cfg.frequency = 100;        // 100 Hz
    cfg.failsafe_duty = 0;      // Off if fault
    cfg.ramp_rate = 10;         // Fast response
    cfg.enabled = true;
    cfg.invert = false;

    return cfg;
}

PWMConfig createWarningLightConfig(uint8_t pin) {
    PWMConfig cfg = {};

    strcpy(cfg.name, "Warning Light");
    cfg.pin = pin;
    cfg.mode = MODE_FAULT;

    // Fault mode: controlled by fault flag
    cfg.input_min = 0;
    cfg.input_max = 1;
    cfg.duty_min = 0;
    cfg.duty_max = 100;

    cfg.frequency = 1;          // 1 Hz (slow flash)
    cfg.failsafe_duty = 100;    // On if fault
    cfg.ramp_rate = 0;          // Instant
    cfg.enabled = true;
    cfg.invert = false;

    return cfg;
}

} // namespace GPPWM
