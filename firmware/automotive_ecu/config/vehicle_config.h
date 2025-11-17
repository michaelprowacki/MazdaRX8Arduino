/**
 * @file vehicle_config.h
 * @brief Unified vehicle configuration for RX8 Arduino ECU replacement
 *
 * This file contains ALL configuration options for the automotive ECU.
 * Configure once, compile, flash - all features in one unified firmware.
 *
 * @author Unified Architecture (Phase 5)
 * @date 2025-11-16
 */

#pragma once

// ============================================================================
// VEHICLE TYPE - Choose ONE
// ============================================================================
#define VEHICLE_TYPE_ICE        1    // Internal combustion engine (rotary/piston)
#define VEHICLE_TYPE_EV         2    // Electric vehicle (motor controller)

#define VEHICLE_TYPE            VEHICLE_TYPE_ICE  // <-- SET THIS

// ============================================================================
// TRANSMISSION TYPE
// ============================================================================
#define TRANSMISSION_MANUAL     1
#define TRANSMISSION_AUTO       2

#define TRANSMISSION_TYPE       TRANSMISSION_MANUAL  // <-- SET THIS

// ============================================================================
// HARDWARE PLATFORM - Choose ONE
// ============================================================================
#define MCU_STM32F407           1    // STM32F407 (recommended for beginners)
#define MCU_TI_C2000            2    // TI C2000 F28379D (best for EV/motor control)
#define MCU_NXP_S32K148         3    // NXP S32K148 (automotive-grade, ISO 26262)
#define MCU_INFINEON_AURIX      4    // Infineon AURIX TC3xx (advanced automotive)
#define MCU_TI_HERCULES         5    // TI Hercules RM46/57 (safety-critical)

#define MCU_PLATFORM            MCU_STM32F407  // <-- SET THIS

// ============================================================================
// WHEEL CONFIGURATION
// ============================================================================
#define WHEEL_SIZE_FRONT        225  // mm (tire width)
#define WHEEL_SIZE_REAR         225  // mm

// Wheel speed sensor pulses per revolution
#define WHEEL_PULSES_PER_REV    48   // Typical for RX8

// ============================================================================
// CAN BUS CONFIGURATION
// ============================================================================
#define CAN_SPEED               500000  // 500 kbps (RX8 high-speed CAN)
#define CAN_UPDATE_RATE_MS      100     // Send CAN messages every 100ms

// ============================================================================
// CORE FEATURES (Safety-Critical)
// ============================================================================
#define ENABLE_CAN_BUS          1    // CAN bus emulation (ALWAYS enabled)
#define ENABLE_IMMOBILIZER      1    // Immobilizer bypass
#define ENABLE_ABS_DSC          1    // ABS/DSC/traction control emulation
#define ENABLE_POWER_STEERING   1    // Power steering enable signal
#define ENABLE_DIAGNOSTICS      1    // OBD-II diagnostic messages
#define ENABLE_SAFETY_MONITOR   1    // Watchdog, failsafe, error handling

// ============================================================================
// ICE-SPECIFIC FEATURES (Only if VEHICLE_TYPE == VEHICLE_TYPE_ICE)
// ============================================================================
#if VEHICLE_TYPE == VEHICLE_TYPE_ICE

#define ENABLE_THROTTLE_PEDAL   1    // Throttle pedal processing (analog input)
#define ENABLE_ENGINE_TEMP      1    // Engine temperature monitoring
#define ENABLE_RPM_CONTROL      1    // RPM signal generation

// Throttle pedal calibration (RX8-specific)
#define THROTTLE_VOLTAGE_MIN    1.7  // Volts
#define THROTTLE_VOLTAGE_MAX    4.0  // Volts
#define THROTTLE_ADC_BITS       12   // 12-bit ADC resolution

// Engine temperature
#define ENGINE_TEMP_NORMAL      145  // Degrees (CAN encoding)
#define ENGINE_TEMP_WARNING     180  // Warning threshold

#endif // VEHICLE_TYPE_ICE

// ============================================================================
// EV-SPECIFIC FEATURES (Only if VEHICLE_TYPE == VEHICLE_TYPE_EV)
// ============================================================================
#if VEHICLE_TYPE == VEHICLE_TYPE_EV

#define ENABLE_MOTOR_CONTROL    1    // Motor controller interface
#define ENABLE_REGEN_BRAKING    1    // Regenerative braking
#define ENABLE_BATTERY_MONITOR  1    // Battery state monitoring
#define ENABLE_CHARGER_CONTROL  0    // Charger control (optional)

// Motor controller type
#define MOTOR_CTRL_OPEN_INV     1    // OpenInverter
#define MOTOR_CTRL_CUSTOM       2    // Custom controller

#define MOTOR_CONTROLLER        MOTOR_CTRL_OPEN_INV

// Battery configuration
#define BATTERY_NOMINAL_VOLTAGE 360  // Volts (e.g., 360V pack)
#define BATTERY_CAPACITY_KWH    40   // kWh

#endif // VEHICLE_TYPE_EV

// ============================================================================
// WARNING LIGHTS CONFIGURATION
// ============================================================================
#define DEFAULT_CHECK_ENGINE    0    // 0 = off, 1 = on at startup
#define DEFAULT_ABS_LIGHT       0    // 0 = off, 1 = on
#define DEFAULT_OIL_PRESSURE    1    // 1 = OK, 0 = warning

// ============================================================================
// COMMUNICATION WITH UI CONTROLLER (ESP32)
// ============================================================================
#define ENABLE_UART_BRIDGE      1    // UART communication to ESP32
#define UART_BAUDRATE           115200
#define UART_UPDATE_RATE_MS     50   // Send vehicle state every 50ms

// Alternatively, use CAN for ESP32 communication
#define ENABLE_CAN_BRIDGE       0    // CAN communication to ESP32 (requires transceiver)

// ============================================================================
// PIN MAPPINGS (Platform-specific - see hal/<platform>/pin_mapping.h)
// ============================================================================
// Pins are defined in hardware abstraction layer for each MCU platform

// ============================================================================
// SAFETY CONFIGURATION
// ============================================================================
#define WATCHDOG_TIMEOUT_MS     1000  // Watchdog timeout (must kick every 1s)
#define FAILSAFE_TIMEOUT_MS     500   // Enter failsafe if no CAN RX for 500ms

// Failsafe behavior
#define FAILSAFE_THROTTLE       0     // Close throttle
#define FAILSAFE_RPM            0     // Zero RPM
#define FAILSAFE_LIGHTS         1     // Turn on all warning lights

// ============================================================================
// DEBUG/DEVELOPMENT OPTIONS
// ============================================================================
#define ENABLE_SERIAL_DEBUG     1    // Serial console output (disable in production)
#define SERIAL_BAUDRATE         115200

// Bench testing mode (no real sensors)
#define BENCH_TEST_MODE         0    // 1 = use simulated inputs

// ============================================================================
// VALIDATION
// ============================================================================
#if VEHICLE_TYPE != VEHICLE_TYPE_ICE && VEHICLE_TYPE != VEHICLE_TYPE_EV
    #error "VEHICLE_TYPE must be VEHICLE_TYPE_ICE or VEHICLE_TYPE_EV"
#endif

#if MCU_PLATFORM < MCU_STM32F407 || MCU_PLATFORM > MCU_TI_HERCULES
    #error "Invalid MCU_PLATFORM selected"
#endif

#if VEHICLE_TYPE == VEHICLE_TYPE_ICE && ENABLE_MOTOR_CONTROL
    #warning "ENABLE_MOTOR_CONTROL is ignored for ICE vehicles"
#endif

#if VEHICLE_TYPE == VEHICLE_TYPE_EV && ENABLE_THROTTLE_PEDAL
    #warning "ENABLE_THROTTLE_PEDAL is typically not used for EV (motor controller handles this)"
#endif

// ============================================================================
// CONFIGURATION SUMMARY (Printed at startup)
// ============================================================================
#define CONFIG_STRING_VEHICLE \
    (VEHICLE_TYPE == VEHICLE_TYPE_ICE ? "ICE" : "EV")

#define CONFIG_STRING_TRANS \
    (TRANSMISSION_TYPE == TRANSMISSION_MANUAL ? "Manual" : "Auto")

#define CONFIG_STRING_MCU \
    (MCU_PLATFORM == MCU_STM32F407 ? "STM32F407" : \
     MCU_PLATFORM == MCU_TI_C2000 ? "TI C2000" : \
     MCU_PLATFORM == MCU_NXP_S32K148 ? "NXP S32K148" : \
     MCU_PLATFORM == MCU_INFINEON_AURIX ? "Infineon AURIX" : \
     MCU_PLATFORM == MCU_TI_HERCULES ? "TI Hercules" : "Unknown")
