/**
 * @file motor_control.cpp
 * @brief Electric motor control for EV conversions
 *
 * Handles motor-specific functions:
 * - Motor RPM from inverter CAN messages
 * - Motor/inverter temperature monitoring
 * - Precharge relay control
 * - Motor controller integration
 *
 * @author Ported from legacy rx8can EV ECU
 * @date 2025-11-16
 */

#include "../../config/vehicle_config.h"

#if VEHICLE_TYPE == VEHICLE_TYPE_EV

#include "motor_control.h"
#include "../hal/hal_interface.h"
#include <Arduino.h>

namespace MotorControl {

// Motor state
static uint16_t motor_rpm = 0;          // Current motor RPM
static int16_t motor_temp = 0;          // Motor/inverter temperature (°C * 10)
static uint32_t idle_counter = 0;       // Counter for idle detection
static bool precharge_active = false;   // Precharge relay state

// Pin configuration (mapped by HAL)
static const uint8_t PRECHARGE_PIN = 0;  // PWM output for precharge/contactor

void init() {
    motor_rpm = 0;
    motor_temp = 0;
    idle_counter = 0;
    precharge_active = false;

    // Configure precharge/contactor PWM output
    HAL_GPIO_SetMode(PRECHARGE_PIN, HAL_GPIO_MODE_PWM);

    #if ENABLE_SERIAL_DEBUG
        Serial.println("[MOTOR] EV motor control initialized");
        Serial.println("[MOTOR] Listening for inverter CAN messages:");
        Serial.println("[MOTOR]   - ID 0x00A (10): Motor RPM");
        Serial.println("[MOTOR]   - ID 0x00F (15): Inverter temperature");
    #endif

    // Prime precharge/oil pump on startup
    delay(2000);
    HAL_PWM_Write(PRECHARGE_PIN, 254);  // Full power
    precharge_active = true;

    #if ENABLE_SERIAL_DEBUG
        Serial.println("[MOTOR] Precharge sequence started");
    #endif
}

void processCANMessage(uint32_t can_id, const uint8_t* data) {
    switch (can_id) {
        case 0x00A:  // Motor RPM from inverter (ID 10)
        {
            uint16_t rpm_raw = (data[1] << 8) | data[0];

            // Clamp to safe range
            if (rpm_raw <= 9000) {
                motor_rpm = rpm_raw;
            }

            #if ENABLE_SERIAL_DEBUG >= 2  // Verbose logging
                Serial.printf("[MOTOR] RPM update: %d\n", motor_rpm);
            #endif
            break;
        }

        case 0x00F:  // Inverter temperature (ID 15)
        {
            // Map inverter temp from 0-254 to engine temp range (88-230)
            // This corresponds to roughly 40°C to 115°C
            int16_t temp_mapped = map(data[0], 0, 254, 88, 230);
            motor_temp = temp_mapped;

            #if ENABLE_SERIAL_DEBUG
                Serial.printf("[MOTOR] Temperature: %d (raw: %d)\n", motor_temp, data[0]);
            #endif
            break;
        }

        // Add support for other motor controller protocols here
        // (OpenInverter, Tesla, custom controllers, etc.)

        default:
            // Unknown motor controller message
            break;
    }
}

void update(uint8_t throttle_percent, uint16_t vehicle_speed) {
    // Idle detection: if motor RPM is low, increment counter
    if (motor_rpm <= 100) {
        idle_counter++;
    } else {
        idle_counter = 0;
    }

    // Precharge/pump control based on idle time
    if (idle_counter >= 3000) {
        // Motor idle for extended period - turn off precharge
        HAL_PWM_Write(PRECHARGE_PIN, 0);
        precharge_active = false;

        #if ENABLE_SERIAL_DEBUG
            if (idle_counter == 3000) {  // Log only once
                Serial.println("[MOTOR] Idle timeout - precharge off");
            }
        #endif
    } else {
        // Motor active - keep precharge on
        HAL_PWM_Write(PRECHARGE_PIN, 254);
        precharge_active = true;
    }

    // Prevent counter overflow
    if (idle_counter >= 4000) {
        idle_counter = 4000;
    }

    // Future: Add regen braking logic here
    #if ENABLE_REGEN_BRAKING
        // Calculate regen braking based on:
        // - Throttle position (off-throttle)
        // - Vehicle speed (enough kinetic energy)
        // - Battery SOC (not full)
        // - Brake pressure sensor (if available)
    #endif
}

uint16_t getRPM() {
    return motor_rpm;
}

void setRPM(uint16_t rpm) {
    motor_rpm = rpm;
}

int16_t getTemperature() {
    return motor_temp;
}

void setTemperature(int16_t temp) {
    motor_temp = temp;
}

bool isRunning() {
    return motor_rpm > 100;  // Consider running if > 100 RPM
}

bool isPrechargeActive() {
    return precharge_active;
}

} // namespace MotorControl

#endif // VEHICLE_TYPE_EV
