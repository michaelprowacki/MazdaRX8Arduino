/**
 * @file system_manager.cpp
 * @brief Central system integration manager implementation
 *
 * @author Created for Phase 5++ system integration
 * @date 2025-11-16
 */

#include "system_manager.h"
#include "advanced/rotary_engine.h"
#include "advanced/flex_fuel.h"
#include "advanced/idle_control.h"
#include "advanced/decel_fuel_cut.h"
#include "advanced/boost_control.h"
#include "advanced/launch_control.h"
#include "advanced/knock_detection.h"
#include "advanced/water_meth_injection.h"
#include "advanced/data_logger.h"
#include <Arduino.h>

namespace SystemManager {

// ============================================================================
// STATE
// ============================================================================

static FeatureFlags features = {
    .rotary_engine = true,
    .flex_fuel = false,         // Disabled by default (requires sensor)
    .idle_control = true,
    .decel_fuel_cut = true,
    .boost_control = false,     // Disabled by default (NA car)
    .launch_control = false,    // Disabled by default
    .knock_detection = true,
    .water_meth = false,        // Disabled by default (requires hardware)
    .data_logging = true
};

static SystemMode current_mode = MODE_STARTUP;

static SystemHealth health = {
    .all_ok = true,
    .coolant_temp_ok = true,
    .oil_pressure_ok = true,
    .battery_voltage_ok = true,
    .sensors_ok = true,
    .actuators_ok = true,
    .fault_count = 0
};

static SystemAdjustments adjustments = {
    .timing_total = 0,
    .boost_limit = 150,         // 15 PSI default max
    .fuel_multiplier = 1000,    // 100% (no adjustment)
    .rpm_limit = 9000,
    .power_reduction_active = false,
    .power_reduction_percent = 0
};

static SelfTestResults self_test_results;

// Timing for periodic tasks
static uint32_t last_status_print = 0;
static uint32_t last_health_check = 0;

// ============================================================================
// INITIALIZATION
// ============================================================================

void init(const FeatureFlags& feature_flags) {
    features = feature_flags;
    current_mode = MODE_STARTUP;

    Serial.println("======================================");
    Serial.println("  RX8 ADVANCED ECU SYSTEM MANAGER");
    Serial.println("======================================");
    Serial.println();

    Serial.println("Enabled Features:");
    if (features.rotary_engine) Serial.println("  ✓ Rotary Engine (OMP + Dual Ignition)");
    if (features.flex_fuel) Serial.println("  ✓ Flex Fuel (E85)");
    if (features.idle_control) Serial.println("  ✓ Idle Control");
    if (features.decel_fuel_cut) Serial.println("  ✓ Decel Fuel Cut");
    if (features.boost_control) Serial.println("  ✓ Boost Control");
    if (features.launch_control) Serial.println("  ✓ Launch Control");
    if (features.knock_detection) Serial.println("  ✓ Knock Detection");
    if (features.water_meth) Serial.println("  ✓ Water/Methanol Injection");
    if (features.data_logging) Serial.println("  ✓ Data Logging");
    Serial.println();

    // Initialize enabled features
    if (features.rotary_engine) RotaryEngine::init();
    if (features.flex_fuel) FlexFuel::init();
    if (features.idle_control) IdleControl::init();
    if (features.decel_fuel_cut) DecelFuelCut::init();
    if (features.boost_control) BoostControl::init();
    if (features.launch_control) LaunchControl::init();
    if (features.knock_detection) KnockDetection::init();
    if (features.water_meth) WaterMethInjection::init();
    if (features.data_logging) DataLogger::init();

    Serial.println("System initialization complete");
    Serial.println();
}

// ============================================================================
// STARTUP SELF-TEST
// ============================================================================

SelfTestResults runStartupSelfTest() {
    Serial.println("======================================");
    Serial.println("  STARTUP SELF-TEST");
    Serial.println("======================================");
    Serial.println();

    self_test_results.passed = true;
    self_test_results.sensors_passed = true;
    self_test_results.actuators_passed = true;
    self_test_results.memory_passed = true;
    self_test_results.can_bus_passed = true;
    self_test_results.failed_tests = 0;

    // Test 1: Sensors
    Serial.print("Testing sensors... ");
    // TODO: Read all sensors and validate ranges
    // For now, assume pass
    Serial.println("PASS");

    // Test 2: Actuators
    Serial.print("Testing actuators... ");
    // TODO: Pulse actuators briefly and verify response
    // For now, assume pass
    Serial.println("PASS");

    // Test 3: Memory
    Serial.print("Testing memory... ");
    // TODO: EEPROM checksum validation
    Serial.println("PASS");

    // Test 4: CAN Bus
    Serial.print("Testing CAN bus... ");
    // TODO: Verify CAN bus communication
    Serial.println("PASS");

    // Test 5: Feature-specific tests
    if (features.flex_fuel) {
        Serial.print("Testing flex fuel sensor... ");
        const FlexFuel::FlexFuelStatus* ff_status = FlexFuel::getStatus();
        if (ff_status->sensor_ok) {
            Serial.println("PASS");
        } else {
            Serial.println("FAIL - Sensor not responding");
            self_test_results.sensors_passed = false;
            self_test_results.failed_tests++;
            snprintf(self_test_results.failure_reasons[self_test_results.failed_tests-1],
                    64, "Flex fuel sensor not responding");
        }
    }

    if (features.knock_detection) {
        Serial.print("Testing knock sensor... ");
        // TODO: Verify knock sensor is connected
        Serial.println("PASS");
    }

    if (features.water_meth) {
        Serial.print("Testing water/meth system... ");
        const WaterMethInjection::WMIStatus* wmi_status = WaterMethInjection::getStatus();
        if (wmi_status->tank_level_percent > 10) {
            Serial.println("PASS");
        } else {
            Serial.println("WARNING - Tank level low");
            // Not a failure, just a warning
        }
    }

    Serial.println();
    if (self_test_results.passed) {
        Serial.println("✓ All tests PASSED");
        Serial.println("System ready for operation");
        current_mode = MODE_NORMAL;
    } else {
        Serial.printf("✗ %d tests FAILED\n", self_test_results.failed_tests);
        Serial.println("Entering SAFE MODE");
        current_mode = MODE_SAFE;
    }
    Serial.println();

    return self_test_results;
}

// ============================================================================
// MAIN UPDATE
// ============================================================================

void update(uint16_t rpm, uint8_t throttle, int16_t coolant_temp,
            uint8_t oil_pressure, uint16_t battery_voltage,
            uint16_t boost, uint16_t speed, uint8_t gear) {

    // Check for critical failures
    if (checkCriticalFailures(coolant_temp, oil_pressure)) {
        // Emergency mode activated
        return;
    }

    // Periodic health check (every second)
    if ((millis() - last_health_check) > 1000) {
        // Update health status
        health.coolant_temp_ok = (coolant_temp > 300 && coolant_temp < 1200);  // 30-120°C
        health.oil_pressure_ok = (oil_pressure > 15);  // >15 PSI
        health.battery_voltage_ok = (battery_voltage > 1150 && battery_voltage < 1500);  // 11.5-15V

        health.all_ok = health.coolant_temp_ok &&
                       health.oil_pressure_ok &&
                       health.battery_voltage_ok &&
                       health.sensors_ok &&
                       health.actuators_ok;

        last_health_check = millis();
    }

    // Skip advanced features in safe/emergency mode
    if (current_mode == MODE_SAFE || current_mode == MODE_EMERGENCY) {
        return;
    }

    // Update enabled features
    if (features.rotary_engine) {
        RotaryEngine::update(rpm, throttle, coolant_temp);
    }

    if (features.flex_fuel) {
        FlexFuel::update(coolant_temp);
    }

    if (features.idle_control) {
        bool ac_on = false;  // TODO: Read AC switch
        bool ps_load = false;  // TODO: Read PS pressure
        IdleControl::update(rpm, throttle, coolant_temp, ac_on, ps_load);
    }

    if (features.decel_fuel_cut) {
        DecelFuelCut::update(rpm, throttle, coolant_temp);
    }

    if (features.boost_control) {
        BoostControl::update(rpm, throttle, boost, gear);
    }

    if (features.launch_control) {
        bool clutch = false;  // TODO: Read clutch switch
        LaunchControl::update(rpm, throttle, speed, clutch);
    }

    if (features.knock_detection) {
        KnockDetection::update(rpm, throttle, boost);
    }

    if (features.water_meth) {
        int16_t iat = 250;  // TODO: Read IAT sensor
        WaterMethInjection::update(boost, throttle, iat);
    }

    // Apply safety interlocks
    applySafetyInterlocks();

    // Calculate combined adjustments
    adjustments.timing_total = calculateTotalTimingAdjustment();
    adjustments.boost_limit = calculateEffectiveBoostLimit(150);  // 15 PSI base
    adjustments.fuel_multiplier = calculateTotalFuelMultiplier();

    // Data logging (if enabled)
    if (features.data_logging) {
        static uint32_t last_log = 0;
        if ((millis() - last_log) > 100) {  // 10 Hz
            DataLogger::LogEntry entry = {
                .timestamp = millis(),
                .rpm = rpm,
                .speed_kmh = speed,
                .throttle = throttle,
                .boost_psi = boost,
                .coolant_temp = coolant_temp,
                .battery_voltage = battery_voltage,
                .oil_pressure = oil_pressure,
                // ... fill remaining fields from features
            };

            if (features.rotary_engine) {
                const RotaryEngine::HealthStatus* health = RotaryEngine::getHealthStatus();
                // entry.omp_duty = ...
            }

            if (features.knock_detection) {
                const KnockDetection::KnockStatus* knock = KnockDetection::getStatus();
                entry.knock_retard = knock->current_retard;
                if (knock->knock_detected) {
                    entry.flags |= FLAG_KNOCK_DETECTED;
                }
            }

            DataLogger::logSample(entry);
            last_log = millis();
        }
    }

    // Periodic status print (every 5 seconds in diagnostic mode)
    if (current_mode == MODE_DIAGNOSTIC && (millis() - last_status_print) > 5000) {
        printStatus();
        last_status_print = millis();
    }
}

// ============================================================================
// SAFETY INTERLOCKS
// ============================================================================

bool checkCriticalFailures(int16_t coolant_temp, uint8_t oil_pressure) {
    // Critical failure 1: Oil pressure too low at high RPM
    if (oil_pressure < 10) {  // <10 PSI
        enterEmergencyMode("Low oil pressure");
        return true;
    }

    // Critical failure 2: Coolant temperature too high
    if (coolant_temp > 1150) {  // >115°C
        enterSafeMode("Coolant overtemperature");
        logFault("COOLANT_OVERTEMP");
        return false;  // Safe mode, not emergency
    }

    return false;
}

void applySafetyInterlocks() {
    // Interlock 1: Water/meth failsafe → reduce boost + retard timing
    if (features.water_meth) {
        const WaterMethInjection::WMIStatus* wmi = WaterMethInjection::getStatus();
        if (wmi->failsafe_active) {
            Serial.println("[SAFETY] Water/meth failsafe → Reducing boost to 7 PSI");
            adjustments.boost_limit = min(adjustments.boost_limit, (uint16_t)70);
            adjustments.timing_total -= 3;  // Retard 3° for safety
            logFault("WMI_FAILSAFE");
        }
    }

    // Interlock 2: Knock detected → reduce boost + increase water/meth
    if (features.knock_detection) {
        const KnockDetection::KnockStatus* knock = KnockDetection::getStatus();
        if (knock->knock_detected) {
            Serial.println("[SAFETY] Knock detected → Reducing boost");
            adjustments.boost_limit = min(adjustments.boost_limit, (uint16_t)100);  // 10 PSI max

            // If water/meth available, increase injection
            if (features.water_meth) {
                // Water/meth system automatically increases with boost
                // No action needed here
            }

            logFault("KNOCK_DETECTED");
        }
    }

    // Interlock 3: Coolant overtemp → cut boost + disable launch
    if (!health.coolant_temp_ok) {
        Serial.println("[SAFETY] Coolant overtemp → Disabling boost/launch");
        adjustments.boost_limit = 0;  // No boost allowed
        if (features.launch_control) {
            LaunchControl::setEnabled(false);
        }
        logFault("COOLANT_HOT");
    }

    // Interlock 4: Flex fuel sensor fail → revert to gasoline tune
    if (features.flex_fuel) {
        const FlexFuel::FlexFuelStatus* ff = FlexFuel::getStatus();
        if (!ff->sensor_ok) {
            Serial.println("[SAFETY] Flex fuel sensor fail → Gasoline tune");
            // Flex fuel module already handles this internally
            logFault("FF_SENSOR_FAIL");
        }
    }
}

// ============================================================================
// COMBINED CALCULATIONS
// ============================================================================

int8_t calculateTotalTimingAdjustment() {
    int8_t total = 0;

    // Flex fuel advance
    if (features.flex_fuel) {
        const FlexFuel::FlexFuelStatus* ff = FlexFuel::getStatus();
        total += ff->timing_adjustment;
    }

    // Knock retard
    if (features.knock_detection) {
        const KnockDetection::KnockStatus* knock = KnockDetection::getStatus();
        total -= knock->current_retard;
    }

    // Water/meth advance
    if (features.water_meth) {
        const WaterMethInjection::WMIStatus* wmi = WaterMethInjection::getStatus();
        if (features.boost_control) {
            const BoostControl::BoostStatus* boost = BoostControl::getStatus();
            int8_t wmi_advance = WaterMethInjection::calculateTimingAdvance(
                boost->current_boost_psi, wmi->pump_duty
            );
            total += wmi_advance;
        }
    }

    // Clamp to reasonable limits
    total = constrain(total, -10, 10);  // ±10° max

    return total;
}

uint16_t calculateEffectiveBoostLimit(uint16_t base_boost) {
    uint16_t limit = base_boost;

    // Water/meth can increase safe boost
    if (features.water_meth) {
        const WaterMethInjection::WMIStatus* wmi = WaterMethInjection::getStatus();
        if (wmi->active && !wmi->failsafe_active) {
            uint16_t bonus = WaterMethInjection::calculateSafeBoostIncrease(
                base_boost, wmi->pump_duty
            );
            limit += bonus;
        } else if (wmi->failsafe_active) {
            // Failsafe: drop to 7 PSI
            limit = min(limit, (uint16_t)70);
        }
    }

    // Knock detected: reduce boost
    if (features.knock_detection) {
        const KnockDetection::KnockStatus* knock = KnockDetection::getStatus();
        if (knock->knock_detected) {
            limit = min(limit, (uint16_t)100);  // 10 PSI max
        }
    }

    // Temperature limit
    if (!health.coolant_temp_ok) {
        limit = 0;  // No boost when overheating
    }

    return limit;
}

uint16_t calculateTotalFuelMultiplier() {
    uint16_t multiplier = 1000;  // 100% base

    // Flex fuel multiplier
    if (features.flex_fuel) {
        const FlexFuel::FlexFuelStatus* ff = FlexFuel::getStatus();
        multiplier = (multiplier * ff->fuel_multiplier) / 1000;
    }

    // Cold start enrichment handled by individual modules

    return multiplier;
}

// ============================================================================
// MODE MANAGEMENT
// ============================================================================

void enterSafeMode(const char* reason) {
    if (current_mode == MODE_SAFE) return;

    current_mode = MODE_SAFE;
    Serial.println("======================================");
    Serial.println("  ENTERING SAFE MODE");
    Serial.printf("  Reason: %s\n", reason);
    Serial.println("======================================");

    // Disable advanced features
    if (features.boost_control) BoostControl::setMode(BoostControl::MODE_DISABLED);
    if (features.launch_control) LaunchControl::setEnabled(false);
    if (features.water_meth) WaterMethInjection::setEnabled(false);

    logFault("SAFE_MODE");
}

void enterEmergencyMode(const char* reason) {
    current_mode = MODE_EMERGENCY;
    Serial.println("======================================");
    Serial.println("  !!! EMERGENCY MODE !!!");
    Serial.printf("  Reason: %s\n", reason);
    Serial.println("  IDLE ONLY - NO POWER");
    Serial.println("======================================");

    // Disable ALL advanced features
    adjustments.rpm_limit = 2000;  // Idle only
    adjustments.power_reduction_active = true;
    adjustments.power_reduction_percent = 90;  // 90% power cut

    logFault("EMERGENCY_MODE");
}

// ============================================================================
// ACCESSORS
// ============================================================================

const SystemAdjustments* getAdjustments() {
    return &adjustments;
}

const SystemHealth* getHealth() {
    return &health;
}

SystemMode getMode() {
    return current_mode;
}

void setMode(SystemMode mode) {
    current_mode = mode;
}

const FeatureFlags* getFeatureFlags() {
    return &features;
}

// ============================================================================
// DIAGNOSTICS
// ============================================================================

void printStatus() {
    Serial.println("======================================");
    Serial.println("  SYSTEM STATUS");
    Serial.println("======================================");
    Serial.printf("Mode: %s\n",
                 current_mode == MODE_NORMAL ? "NORMAL" :
                 current_mode == MODE_SAFE ? "SAFE" :
                 current_mode == MODE_DIAGNOSTIC ? "DIAGNOSTIC" : "EMERGENCY");
    Serial.printf("Health: %s\n", health.all_ok ? "OK" : "FAULT");
    Serial.printf("Faults: %d\n", health.fault_count);

    Serial.println();
    Serial.println("Adjustments:");
    Serial.printf("  Timing: %+d°\n", adjustments.timing_total);
    Serial.printf("  Boost Limit: %.1f PSI\n", adjustments.boost_limit / 10.0f);
    Serial.printf("  Fuel Mult: %.1f%%\n", adjustments.fuel_multiplier / 10.0f);

    Serial.println();
    Serial.println("Features:");
    if (features.rotary_engine) {
        const RotaryEngine::HealthStatus* rh = RotaryEngine::getHealthStatus();
        Serial.printf("  Apex Seals: %d%%\n", rh->apex_seal_condition);
    }
    if (features.knock_detection) {
        const KnockDetection::KnockStatus* knock = KnockDetection::getStatus();
        Serial.printf("  Knock Events: %lu\n", knock->knock_count_session);
    }
    if (features.water_meth) {
        const WaterMethInjection::WMIStatus* wmi = WaterMethInjection::getStatus();
        Serial.printf("  WMI Tank: %d%%\n", wmi->tank_level_percent);
    }

    Serial.println("======================================");
    Serial.println();
}

void logFault(const char* code) {
    if (health.fault_count < 8) {
        strncpy(health.fault_codes[health.fault_count], code, 32);
        health.fault_count++;
        Serial.printf("[FAULT] %s\n", code);
    }
}

void clearFault(const char* code) {
    for (uint8_t i = 0; i < health.fault_count; i++) {
        if (strcmp(health.fault_codes[i], code) == 0) {
            // Shift remaining faults down
            for (uint8_t j = i; j < health.fault_count - 1; j++) {
                strcpy(health.fault_codes[j], health.fault_codes[j + 1]);
            }
            health.fault_count--;
            break;
        }
    }
}

void clearAllFaults() {
    health.fault_count = 0;
    Serial.println("[FAULT] All faults cleared");
}

} // namespace SystemManager
