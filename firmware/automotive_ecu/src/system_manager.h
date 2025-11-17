/**
 * @file system_manager.h
 * @brief Central system integration manager for all advanced features
 *
 * Coordinates:
 * - Rotary engine management (OMP, dual ignition, apex seal monitoring)
 * - Flex fuel (E85) support
 * - Idle control (IAC)
 * - Decel fuel cut (DFCO)
 * - Boost control + anti-lag
 * - Launch control
 * - Knock detection
 * - Water/methanol injection
 * - Data logging
 *
 * Features:
 * - Cross-feature safety interlocks
 * - Startup self-test sequence
 * - Failsafe coordination
 * - Centralized status monitoring
 * - Feature enable/disable matrix
 * - Diagnostic mode
 * - Emergency safe mode
 *
 * Safety Interlocks:
 * - If water/meth fails → reduce boost AND retard timing
 * - If knock detected → reduce boost, retard timing, increase water/meth
 * - If coolant overtemp → cut boost, disable launch, reduce power
 * - If oil pressure low → safe mode (idle only)
 * - If flex fuel sensor fails → revert to gasoline tune
 *
 * @author Created for Phase 5++ system integration
 * @date 2025-11-16
 */

#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include <stdint.h>

namespace SystemManager {

/**
 * @brief Feature enable/disable flags
 */
struct FeatureFlags {
    bool rotary_engine;         // OMP, dual ignition, apex seal monitoring
    bool flex_fuel;             // E85 support
    bool idle_control;          // Closed-loop idle
    bool decel_fuel_cut;        // DFCO
    bool boost_control;         // Turbo boost control
    bool launch_control;        // 2-step launch
    bool knock_detection;       // Knock sensor
    bool water_meth;            // Water/methanol injection
    bool data_logging;          // Black box logging
};

/**
 * @brief System operating mode
 */
enum SystemMode {
    MODE_STARTUP,           // Startup self-test in progress
    MODE_NORMAL,            // Normal operation
    MODE_SAFE,              // Safe mode (advanced features disabled)
    MODE_DIAGNOSTIC,        // Diagnostic mode (verbose logging)
    MODE_EMERGENCY          // Emergency mode (idle only)
};

/**
 * @brief System health status
 */
struct SystemHealth {
    bool all_ok;                    // Overall system health
    bool coolant_temp_ok;           // Coolant temp in range
    bool oil_pressure_ok;           // Oil pressure adequate
    bool battery_voltage_ok;        // Battery voltage in range
    bool sensors_ok;                // All sensors reading valid
    bool actuators_ok;              // All actuators responding
    uint8_t fault_count;            // Number of active faults
    char fault_codes[8][32];        // Fault descriptions
};

/**
 * @brief Cross-feature adjustments
 */
struct SystemAdjustments {
    int8_t  timing_total;           // Total timing adjustment (degrees)
    uint16_t boost_limit;           // Effective boost limit (PSI * 10)
    uint16_t fuel_multiplier;       // Total fuel adjustment (% * 10)
    uint16_t rpm_limit;             // Effective RPM limit
    bool    power_reduction_active; // Power reduction in effect
    uint8_t power_reduction_percent; // Power reduction amount (%)
};

/**
 * @brief Startup self-test results
 */
struct SelfTestResults {
    bool passed;                    // Overall pass/fail
    bool sensors_passed;            // Sensor validation
    bool actuators_passed;          // Actuator validation
    bool memory_passed;             // Memory check
    bool can_bus_passed;            // CAN bus check
    uint8_t failed_tests;           // Number of failed tests
    char failure_reasons[8][64];    // Failure descriptions
};

/**
 * @brief Initialize system manager
 *
 * @param features Feature enable flags
 */
void init(const FeatureFlags& features);

/**
 * @brief Run startup self-test
 *
 * Tests all sensors, actuators, and subsystems
 *
 * @return Self-test results
 */
SelfTestResults runStartupSelfTest();

/**
 * @brief Update system manager (main loop)
 *
 * Call this every loop iteration. Coordinates all subsystems.
 *
 * @param rpm Current RPM
 * @param throttle Throttle position (0-100%)
 * @param coolant_temp Coolant temp (°C * 10)
 * @param oil_pressure Oil pressure (PSI)
 * @param battery_voltage Battery voltage (V * 100)
 * @param boost Boost pressure (PSI * 10)
 * @param speed Vehicle speed (km/h * 10)
 * @param gear Current gear (0-6)
 */
void update(uint16_t rpm, uint8_t throttle, int16_t coolant_temp,
            uint8_t oil_pressure, uint16_t battery_voltage,
            uint16_t boost, uint16_t speed, uint8_t gear);

/**
 * @brief Get cross-feature adjustments
 *
 * Returns combined adjustments from all active features
 *
 * @return System adjustments
 */
const SystemAdjustments* getAdjustments();

/**
 * @brief Get system health status
 *
 * @return Health status
 */
const SystemHealth* getHealth();

/**
 * @brief Get current system mode
 *
 * @return Operating mode
 */
SystemMode getMode();

/**
 * @brief Set system mode
 *
 * @param mode Desired mode
 */
void setMode(SystemMode mode);

/**
 * @brief Check for critical failures
 *
 * Activates emergency mode if critical failure detected
 *
 * @param coolant_temp Coolant temp
 * @param oil_pressure Oil pressure
 * @return true if critical failure
 */
bool checkCriticalFailures(int16_t coolant_temp, uint8_t oil_pressure);

/**
 * @brief Apply safety interlocks
 *
 * Coordinates safety across all features:
 * - Water/meth failure → reduce boost + retard timing
 * - Knock detected → reduce boost + increase water/meth
 * - Overtemp → cut boost + disable launch
 * - Low oil → emergency mode
 */
void applySafetyInterlocks();

/**
 * @brief Calculate combined timing adjustment
 *
 * Sums adjustments from:
 * - Flex fuel (+0-5°)
 * - Knock detection (-0-10°)
 * - Water/meth (+0-5°)
 * - Safe mode (retard if needed)
 *
 * @return Total timing adjustment (degrees)
 */
int8_t calculateTotalTimingAdjustment();

/**
 * @brief Calculate effective boost limit
 *
 * Takes minimum of:
 * - Base boost target
 * - Water/meth failsafe limit
 * - Knock-based limit
 * - Temperature-based limit
 *
 * @param base_boost Base boost target
 * @return Effective boost limit (PSI * 10)
 */
uint16_t calculateEffectiveBoostLimit(uint16_t base_boost);

/**
 * @brief Calculate combined fuel multiplier
 *
 * Multiplies adjustments from:
 * - Flex fuel (1.0-1.3x for E85)
 * - Idle control enrichment
 * - Cold start enrichment
 *
 * @return Total fuel multiplier (% * 10)
 */
uint16_t calculateTotalFuelMultiplier();

/**
 * @brief Enter safe mode
 *
 * Disables all advanced features, runs on conservative base tune
 */
void enterSafeMode(const char* reason);

/**
 * @brief Enter emergency mode
 *
 * Idle-only operation, all power features disabled
 */
void enterEmergencyMode(const char* reason);

/**
 * @brief Get diagnostic information
 *
 * Returns detailed status of all subsystems
 *
 * @param buffer Buffer to write diagnostic info
 * @param max_len Maximum buffer length
 */
void getDiagnosticInfo(char* buffer, size_t max_len);

/**
 * @brief Enable/disable specific feature
 *
 * @param feature Feature name
 * @param enabled Enable/disable
 */
void setFeatureEnabled(const char* feature, bool enabled);

/**
 * @brief Get feature enable status
 *
 * @return Feature flags
 */
const FeatureFlags* getFeatureFlags();

/**
 * @brief Print system status to serial
 *
 * Human-readable status display
 */
void printStatus();

/**
 * @brief Log fault code
 *
 * @param code Fault code string
 */
void logFault(const char* code);

/**
 * @brief Clear fault code
 *
 * @param code Fault code to clear
 */
void clearFault(const char* code);

/**
 * @brief Clear all faults
 */
void clearAllFaults();

} // namespace SystemManager

#endif // SYSTEM_MANAGER_H
