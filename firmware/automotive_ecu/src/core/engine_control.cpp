/**
 * @file engine_control.cpp
 * @brief ICE engine control for RX8
 *
 * Handles engine-specific functions:
 * - RPM control/reporting
 * - Engine temperature
 * - Throttle pedal integration
 *
 * @author Ported from legacy RX8_CANBUS.ino
 * @date 2025-11-16
 */

#include "../../config/vehicle_config.h"

#if VEHICLE_TYPE == VEHICLE_TYPE_ICE

#include <Arduino.h>

namespace EngineControl {

// Engine state
static uint16_t engine_rpm = 1000;      // Current RPM
static int16_t engine_temp = ENGINE_TEMP_NORMAL * 10;  // Temperature (°C * 10)

void init() {
    engine_rpm = 1000;  // Idle RPM
    engine_temp = ENGINE_TEMP_NORMAL * 10;

    #if ENABLE_SERIAL_DEBUG
        Serial.println("[ENGINE] ICE engine control initialized");
        Serial.printf("[ENGINE] Target temp: %.1f°C\n", ENGINE_TEMP_NORMAL / 10.0f);
    #endif
}

void update(uint8_t throttle_percent, uint16_t vehicle_speed) {
    // Simple RPM calculation based on throttle and speed
    // In a real implementation, this would come from an actual RPM sensor

    // Base RPM: idle (1000) to redline (9000 for rotary)
    uint16_t base_rpm = 1000 + (throttle_percent * 80);  // 1000-9000 RPM range

    // Add speed-based component (simulating gear ratios)
    uint16_t speed_rpm = (vehicle_speed / 10) * 100;  // Rough approximation

    // Combine (weighted average)
    engine_rpm = (base_rpm * 7 + speed_rpm * 3) / 10;

    // Clamp to sane values
    if (engine_rpm < 800) engine_rpm = 800;    // Below idle
    if (engine_rpm > 9000) engine_rpm = 9000;  // Redline

    // Temperature gradually approaches normal
    int16_t target_temp = ENGINE_TEMP_NORMAL * 10;
    if (engine_temp < target_temp) {
        engine_temp += 1;  // Warm up slowly
    } else if (engine_temp > target_temp) {
        engine_temp -= 1;  // Cool down slowly
    }
}

uint16_t getRPM() {
    return engine_rpm;
}

void setRPM(uint16_t rpm) {
    engine_rpm = rpm;
}

int16_t getTemperature() {
    return engine_temp;
}

void setTemperature(int16_t temp) {
    engine_temp = temp;
}

} // namespace EngineControl

#endif // VEHICLE_TYPE_ICE
