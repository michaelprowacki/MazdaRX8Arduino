/**
 * @file main.cpp
 * @brief Unified Automotive ECU Firmware - ALL critical features in ONE codebase
 *
 * This replaces ALL separate ECU modules with a single unified firmware:
 * - ECU_Module (ICE engine control)
 * - EV_ECU_Module (EV motor control)
 * - Immobilizer bypass
 * - ABS/DSC emulation
 * - All CAN bus messages
 *
 * Configure via config/vehicle_config.h
 *
 * @author Unified Architecture (Phase 5)
 * @date 2025-11-16
 */

#include <Arduino.h>
#include "config/vehicle_config.h"

// HAL (Hardware Abstraction Layer) - platform-specific
#include "hal/hal_interface.h"

// Core modules - ALL included, enabled/disabled via config
#include "core/can_controller.h"

#if ENABLE_SAFETY_MONITOR
    #include "core/safety_monitor.h"
#endif

#if VEHICLE_TYPE == VEHICLE_TYPE_ICE
    #include "core/engine_control.h"
#elif VEHICLE_TYPE == VEHICLE_TYPE_EV
    #include "core/motor_control.h"
#endif

#if ENABLE_IMMOBILIZER
    #include "core/immobilizer.h"
#endif

#if ENABLE_ABS_DSC
    #include "core/abs_dsc.h"
#endif

#if ENABLE_DIAGNOSTICS
    #include "core/diagnostics.h"
#endif

#if ENABLE_UART_BRIDGE
    #include "communication/uart_bridge.h"
#endif

// Peripherals
#include "peripherals/wheel_speed.h"

#if ENABLE_THROTTLE_PEDAL
    #include "peripherals/throttle_pedal.h"
#endif

#if ENABLE_POWER_STEERING
    #include "peripherals/power_steering.h"
#endif

// Shared CAN protocol library
#include "../shared/RX8_CAN_Protocol/can_messages.h"

// ============================================================================
// GLOBAL STATE
// ============================================================================
struct VehicleState {
    // Engine/Motor
    uint16_t rpm;
    uint16_t speed_kmh;          // km/h * 10 (e.g., 1234 = 123.4 km/h)
    uint8_t  throttle_percent;   // 0-100%

    // Temperatures
    int16_t  coolant_temp;       // °C * 10
    int16_t  oil_temp;           // °C * 10

    // Electrical
    uint16_t battery_voltage;    // V * 100 (e.g., 1380 = 13.80V)

    // Wheel speeds
    uint16_t wheel_fl;           // Front left (km/h * 100)
    uint16_t wheel_fr;           // Front right
    uint16_t wheel_rl;           // Rear left
    uint16_t wheel_rr;           // Rear right

    // Warning flags
    struct {
        uint8_t check_engine    : 1;
        uint8_t abs_warning     : 1;
        uint8_t oil_pressure    : 1;
        uint8_t coolant_level   : 1;
        uint8_t battery_charge  : 1;
        uint8_t brake_failure   : 1;
        uint8_t immobilizer     : 1;
        uint8_t reserved        : 1;
    } warnings;

    // Safety
    bool immobilizer_unlocked;
    bool failsafe_active;
    uint32_t last_can_rx_time;

} g_vehicle_state = {0};

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================
void printBanner();
void printConfig();
void updateVehicleState();
void transmitCANMessages();

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    // Initialize serial console (if enabled)
    #if ENABLE_SERIAL_DEBUG
        Serial.begin(SERIAL_BAUDRATE);
        delay(1000);  // Wait for serial connection
        printBanner();
        printConfig();
    #endif

    // Initialize Hardware Abstraction Layer
    HAL_Init();

    // Initialize safety monitor (watchdog)
    #if ENABLE_SAFETY_MONITOR
        SafetyMonitor::init();
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[SAFETY] Watchdog initialized");
        #endif
    #endif

    // Initialize CAN controller (ALWAYS required)
    CANController::init(CAN_SPEED);
    #if ENABLE_SERIAL_DEBUG
        Serial.println("[CAN] Controller initialized @ 500 kbps");
    #endif

    // Initialize vehicle control system
    #if VEHICLE_TYPE == VEHICLE_TYPE_ICE
        EngineControl::init();
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[ENGINE] ICE control initialized");
        #endif
    #elif VEHICLE_TYPE == VEHICLE_TYPE_EV
        MotorControl::init();
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[MOTOR] EV control initialized");
        #endif
    #endif

    // Initialize immobilizer
    #if ENABLE_IMMOBILIZER
        Immobilizer::init();
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[IMMOB] Immobilizer bypass initialized");
        #endif
    #endif

    // Initialize ABS/DSC
    #if ENABLE_ABS_DSC
        ABSDSC::init();
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[ABS] ABS/DSC emulation initialized");
        #endif
    #endif

    // Initialize diagnostics
    #if ENABLE_DIAGNOSTICS
        Diagnostics::init();
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[DIAG] OBD-II diagnostics initialized");
        #endif
    #endif

    // Initialize wheel speed sensors
    WheelSpeed::init();
    #if ENABLE_SERIAL_DEBUG
        Serial.println("[WHEEL] Wheel speed sensors initialized");
    #endif

    // Initialize throttle pedal (ICE only)
    #if ENABLE_THROTTLE_PEDAL
        ThrottlePedal::init();
        ThrottlePedal::calibrate();  // Auto-calibrate at startup
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[THROTTLE] Throttle pedal calibrated");
        #endif
    #endif

    // Initialize power steering
    #if ENABLE_POWER_STEERING
        PowerSteering::enable();
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[PS] Power steering enabled");
        #endif
    #endif

    // Initialize UART bridge to ESP32 (if enabled)
    #if ENABLE_UART_BRIDGE
        UARTBridge::init(UART_BAUDRATE);
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[UART] Bridge to ESP32 initialized");
        #endif
    #endif

    #if ENABLE_SERIAL_DEBUG
        Serial.println("\n=== AUTOMOTIVE ECU READY ===\n");
    #endif

    // Set default vehicle state
    g_vehicle_state.warnings.check_engine = DEFAULT_CHECK_ENGINE;
    g_vehicle_state.warnings.oil_pressure = DEFAULT_OIL_PRESSURE;
    g_vehicle_state.immobilizer_unlocked = false;
    g_vehicle_state.failsafe_active = false;
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
    static uint32_t last_can_tx = 0;
    static uint32_t last_uart_tx = 0;
    static uint32_t last_debug = 0;
    uint32_t now = millis();

    // Kick watchdog (CRITICAL!)
    #if ENABLE_SAFETY_MONITOR
        SafetyMonitor::kick();
    #endif

    // Process incoming CAN messages
    CANController::process();

    // Handle immobilizer (must respond quickly!)
    #if ENABLE_IMMOBILIZER
        Immobilizer::update();
        if (Immobilizer::isUnlocked()) {
            g_vehicle_state.immobilizer_unlocked = true;
        }
    #endif

    // Only proceed if immobilizer is unlocked (safety!)
    #if ENABLE_IMMOBILIZER
        if (!g_vehicle_state.immobilizer_unlocked) {
            // Stay in safe state until unlocked
            delay(10);
            return;
        }
    #endif

    // Update vehicle state from sensors
    updateVehicleState();

    // Update vehicle control
    #if VEHICLE_TYPE == VEHICLE_TYPE_ICE
        EngineControl::update(g_vehicle_state.throttle_percent,
                              g_vehicle_state.speed_kmh);
        g_vehicle_state.rpm = EngineControl::getRPM();
    #elif VEHICLE_TYPE == VEHICLE_TYPE_EV
        MotorControl::update(g_vehicle_state.throttle_percent,
                             g_vehicle_state.speed_kmh);
        g_vehicle_state.rpm = MotorControl::getRPM();
    #endif

    // Update ABS/DSC
    #if ENABLE_ABS_DSC
        ABSDSC::update(g_vehicle_state.speed_kmh);
    #endif

    // Transmit CAN messages (100ms interval)
    if (now - last_can_tx >= CAN_UPDATE_RATE_MS) {
        transmitCANMessages();
        last_can_tx = now;
    }

    // Send vehicle state to ESP32 via UART (50ms interval)
    #if ENABLE_UART_BRIDGE
        if (now - last_uart_tx >= UART_UPDATE_RATE_MS) {
            UARTBridge::sendVehicleState(g_vehicle_state);
            last_uart_tx = now;
        }
    #endif

    // Debug output (1000ms interval)
    #if ENABLE_SERIAL_DEBUG
        if (now - last_debug >= 1000) {
            Serial.printf("RPM: %d | Speed: %.1f km/h | Throttle: %d%% | Coolant: %.1f°C\n",
                         g_vehicle_state.rpm,
                         g_vehicle_state.speed_kmh / 10.0f,
                         g_vehicle_state.throttle_percent,
                         g_vehicle_state.coolant_temp / 10.0f);
            last_debug = now;
        }
    #endif
}

// ============================================================================
// UPDATE VEHICLE STATE FROM SENSORS
// ============================================================================
void updateVehicleState() {
    // Read wheel speeds
    WheelSpeed::read(&g_vehicle_state.wheel_fl,
                     &g_vehicle_state.wheel_fr,
                     &g_vehicle_state.wheel_rl,
                     &g_vehicle_state.wheel_rr);

    // Calculate vehicle speed (average of front wheels)
    uint16_t avg_front = (g_vehicle_state.wheel_fl + g_vehicle_state.wheel_fr) / 2;
    g_vehicle_state.speed_kmh = avg_front / 10;  // Convert to km/h * 10

    // Read throttle pedal
    #if ENABLE_THROTTLE_PEDAL
        g_vehicle_state.throttle_percent = ThrottlePedal::readPercent();
    #else
        g_vehicle_state.throttle_percent = 0;  // EV gets throttle from motor controller
    #endif

    // Read temperatures (from CAN or sensors)
    #if VEHICLE_TYPE == VEHICLE_TYPE_ICE
        g_vehicle_state.coolant_temp = ENGINE_TEMP_NORMAL * 10;  // TODO: Read from sensor
    #elif VEHICLE_TYPE == VEHICLE_TYPE_EV
        g_vehicle_state.coolant_temp = 0;  // EV may not have coolant temp
    #endif

    // Read battery voltage
    g_vehicle_state.battery_voltage = HAL_ReadBatteryVoltage();  // V * 100
}

// ============================================================================
// TRANSMIT ALL CAN MESSAGES
// ============================================================================
void transmitCANMessages() {
    // 0x201 - PCM Status (RPM, Speed, Throttle)
    uint8_t msg_201[8];
    RX8_CAN_Encoder::encode0x201(msg_201,
                                  g_vehicle_state.rpm,
                                  g_vehicle_state.speed_kmh / 10,  // Convert to mph
                                  g_vehicle_state.throttle_percent);
    CANController::transmit(0x201, msg_201, 8);

    // 0x420 - MIL/Warning Lights
    uint8_t msg_420[7];
    RX8_CAN_Encoder::encode0x420(msg_420,
                                  g_vehicle_state.coolant_temp / 10,
                                  g_vehicle_state.warnings.check_engine,
                                  g_vehicle_state.warnings.coolant_level,
                                  g_vehicle_state.warnings.battery_charge,
                                  g_vehicle_state.warnings.oil_pressure);
    CANController::transmit(0x420, msg_420, 7);

    // 0x203 - Traction Control
    uint8_t msg_203[7] = {0,0,0,0,0,0,0};
    CANController::transmit(0x203, msg_203, 7);

    // 0x215, 0x231, 0x240 - PCM Status Supplements
    uint8_t msg_215[8] = {0,0,0,0,0,0,0,0};
    uint8_t msg_231[5] = {0,0,0,0,0};
    uint8_t msg_240[8] = {0,0,0,0,0,0,0,0};
    CANController::transmit(0x215, msg_215, 8);
    CANController::transmit(0x231, msg_231, 5);
    CANController::transmit(0x240, msg_240, 8);

    // ABS/DSC messages (if enabled)
    #if ENABLE_ABS_DSC
        ABSDSC::transmitCANMessages();
    #endif
}

// ============================================================================
// PRINT BANNER
// ============================================================================
void printBanner() {
    Serial.println("\n");
    Serial.println("╔══════════════════════════════════════════════════════════════╗");
    Serial.println("║     RX8 UNIFIED AUTOMOTIVE ECU - Phase 5 Architecture       ║");
    Serial.println("║     ALL critical features in ONE firmware                    ║");
    Serial.println("╚══════════════════════════════════════════════════════════════╝");
    Serial.println();
}

// ============================================================================
// PRINT CONFIGURATION
// ============================================================================
void printConfig() {
    Serial.println("Configuration:");
    Serial.printf("  Vehicle Type:  %s\n", CONFIG_STRING_VEHICLE);
    Serial.printf("  Transmission:  %s\n", CONFIG_STRING_TRANS);
    Serial.printf("  MCU Platform:  %s\n", CONFIG_STRING_MCU);
    Serial.printf("  CAN Speed:     %d kbps\n", CAN_SPEED / 1000);
    Serial.println();

    Serial.println("Enabled Features:");
    Serial.printf("  [%c] CAN Bus Emulation\n", ENABLE_CAN_BUS ? 'X' : ' ');
    Serial.printf("  [%c] Immobilizer Bypass\n", ENABLE_IMMOBILIZER ? 'X' : ' ');
    Serial.printf("  [%c] ABS/DSC Emulation\n", ENABLE_ABS_DSC ? 'X' : ' ');
    Serial.printf("  [%c] Power Steering\n", ENABLE_POWER_STEERING ? 'X' : ' ');
    Serial.printf("  [%c] OBD-II Diagnostics\n", ENABLE_DIAGNOSTICS ? 'X' : ' ');
    Serial.printf("  [%c] Safety Monitor\n", ENABLE_SAFETY_MONITOR ? 'X' : ' ');

    #if VEHICLE_TYPE == VEHICLE_TYPE_ICE
        Serial.printf("  [%c] Throttle Pedal\n", ENABLE_THROTTLE_PEDAL ? 'X' : ' ');
    #elif VEHICLE_TYPE == VEHICLE_TYPE_EV
        Serial.printf("  [%c] Motor Control\n", ENABLE_MOTOR_CONTROL ? 'X' : ' ');
        Serial.printf("  [%c] Regen Braking\n", ENABLE_REGEN_BRAKING ? 'X' : ' ');
    #endif

    Serial.println();
}
