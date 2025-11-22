/**
 * @file throttle_pedal.cpp
 * @brief Throttle pedal processing for RX8
 *
 * Reads RX8 throttle pedal (1.7V - 4.0V range) and converts to:
 * - 0-100% for CAN bus reporting
 * - 0-960 PWM output for motor controller
 *
 * @author Ported from legacy RX8_CANBUS.ino
 * @date 2025-11-16
 */

#include "../../config/vehicle_config.h"

#if ENABLE_THROTTLE_PEDAL

#include "../hal/hal_interface.h"
#include <Arduino.h>

namespace ThrottlePedal {

// Calibration values
static uint16_t low_pedal = 0;
static uint16_t high_pedal = 803;  // 4.0V on 12-bit ADC
static float convert_throttle = 0.0f;

// Pin mapping (from config)
static const uint8_t INPUT_PIN = 0;   // Will be mapped by HAL
static const uint8_t OUTPUT_PIN = 1;  // Will be mapped by HAL

void init() {
    #if ENABLE_SERIAL_DEBUG
        Serial.println("[THROTTLE] Initializing throttle pedal...");
    #endif

    // Configure pins via HAL
    HAL_GPIO_SetMode(INPUT_PIN, HAL_GPIO_MODE_ANALOG);
    HAL_GPIO_SetMode(OUTPUT_PIN, HAL_GPIO_MODE_PWM);

    #if ENABLE_SERIAL_DEBUG
        Serial.println("[THROTTLE] Throttle pedal initialized");
    #endif
}

void calibrate() {
    #if ENABLE_SERIAL_DEBUG
        Serial.println("[THROTTLE] Calibrating pedal...");
        Serial.println("[THROTTLE] Ensure pedal is at rest!");
    #endif

    delay(500);  // Wait for pedal to settle

    // Read zero throttle position
    uint16_t zero_reading = HAL_ADC_Read(INPUT_PIN);
    low_pedal = (zero_reading > 40) ? (zero_reading - 40) : 0;

    // High pedal is fixed (4.0V)
    // Voltage range: 1.7V - 4.0V from RX8 pedal
    // ADC range (12-bit): ~341 (1.7V) to 803 (4.0V) assuming 5V reference
    high_pedal = 803;

    // Calculate conversion factor
    // Output range: 0 - 960 (0 - 4.5V for controller, 12-bit PWM)
    convert_throttle = 960.0f / (float)(high_pedal - low_pedal);

    #if ENABLE_SERIAL_DEBUG
        Serial.printf("[THROTTLE] Calibration complete:\n");
        Serial.printf("[THROTTLE]   Low: %d (%.2fV)\n", low_pedal, low_pedal * THROTTLE_VOLTAGE_MAX / 1023.0f);
        Serial.printf("[THROTTLE]   High: %d (%.2fV)\n", high_pedal, high_pedal * THROTTLE_VOLTAGE_MAX / 1023.0f);
        Serial.printf("[THROTTLE]   Conversion factor: %.4f\n", convert_throttle);
    #endif
}

uint8_t readPercent() {
    // Read analog value
    uint16_t raw = HAL_ADC_Read(INPUT_PIN);

    // Apply calibration
    int32_t calibrated = raw - low_pedal;
    if (calibrated < 0) calibrated = 0;

    // Convert to output range (0-960)
    int32_t output = (int32_t)(calibrated * convert_throttle);

    // Clamp to valid range
    if (output < 0) output = 0;
    if (output > 960) output = 960;

    // Convert to percentage (0-100)
    uint8_t percent = (output * 100) / 960;

    // Output PWM for motor controller (if needed)
    HAL_PWM_Write(OUTPUT_PIN, output / 4);  // Scale to 8-bit PWM

    return percent;
}

uint16_t readRaw() {
    return HAL_ADC_Read(INPUT_PIN);
}

} // namespace ThrottlePedal

#endif // ENABLE_THROTTLE_PEDAL
