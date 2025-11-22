/**
 * @file decel_fuel_cut.cpp
 * @brief Deceleration fuel cut implementation
 */

#include "decel_fuel_cut.h"
#include <Arduino.h>

namespace DecelFuelCut {

static DFCOConfig config = {
    .enabled = true,
    .activation_rpm = 1500,
    .deactivation_rpm = 1200,
    .throttle_threshold = 2,
    .min_coolant_temp = 500,        // 50°C
    .reenable_delay = 5              // 50ms
};

static DFCOStatus status = {
    .active = false,
    .current_rpm = 0,
    .current_throttle = 0,
    .cut_duration_ms = 0,
    .total_cut_time_ms = 0
};

static uint32_t cut_start_time = 0;
static uint32_t last_deactivation = 0;

void init() {
    Serial.println("[DFCO] Decel fuel cut initialized");
    Serial.printf("[DFCO] Activation: %d RPM, throttle < %d%%\n",
                 config.activation_rpm, config.throttle_threshold);
}

bool update(uint16_t rpm, uint8_t throttle, int16_t coolant_temp) {
    status.current_rpm = rpm;
    status.current_throttle = throttle;

    if (!config.enabled || coolant_temp < config.min_coolant_temp) {
        status.active = false;
        return false;
    }

    bool should_cut = (rpm > config.activation_rpm &&
                      throttle < config.throttle_threshold);

    if (should_cut && !status.active) {
        // Activate DFCO
        status.active = true;
        cut_start_time = millis();
        Serial.println("[DFCO] Fuel cut activated");
    } else if (!should_cut && status.active) {
        // Check if RPM dropped below deactivation threshold
        if (rpm < config.deactivation_rpm) {
            // Re-enable fuel
            status.active = false;
            status.cut_duration_ms = millis() - cut_start_time;
            status.total_cut_time_ms += status.cut_duration_ms;
            last_deactivation = millis();
            Serial.printf("[DFCO] Fuel restored (cut for %lu ms)\n",
                         status.cut_duration_ms);
        }
    }

    // Prevent immediate re-activation (hysteresis)
    if (!status.active && (millis() - last_deactivation) < (config.reenable_delay * 10)) {
        return false;
    }

    return status.active;
}

const DFCOStatus* getStatus() {
    return &status;
}

void configure(const DFCOConfig& new_config) {
    config = new_config;
    init();
}

void setEnabled(bool enabled) {
    config.enabled = enabled;
    Serial.printf("[DFCO] Decel fuel cut %s\n", enabled ? "ENABLED" : "DISABLED");
}

} // namespace DecelFuelCut
