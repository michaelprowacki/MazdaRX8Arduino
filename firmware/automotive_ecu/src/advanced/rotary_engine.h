/**
 * @file rotary_engine.h
 * @brief Advanced rotary engine management for 13B-MSP (RENESIS)
 *
 * Features specific to Mazda's 13B-MSP rotary engine:
 * - Oil Metering Pump (OMP) control
 * - Dual ignition system (leading/trailing spark)
 * - Apex seal health monitoring
 * - Compression monitoring
 * - High RPM protection (9000+ RPM)
 * - Oil consumption tracking
 * - Coolant seal monitoring
 *
 * @author Created for Phase 5+ advanced features
 * @date 2025-11-16
 */

#ifndef ROTARY_ENGINE_H
#define ROTARY_ENGINE_H

#include <stdint.h>

namespace RotaryEngine {

/**
 * @brief Rotary engine health status
 */
struct HealthStatus {
    uint8_t  apex_seal_condition;      // 0-100% (estimated health)
    uint8_t  compression_rotor1;       // 0-100% (relative to new)
    uint8_t  compression_rotor2;       // 0-100% (relative to new)
    uint16_t oil_consumption_ml_100km; // Oil usage rate
    bool     coolant_seal_ok;          // Coolant seal integrity
    bool     oil_metering_ok;          // OMP functioning
    uint32_t hours_since_rebuild;      // Engine operating hours
};

/**
 * @brief Oil Metering Pump (OMP) configuration
 */
struct OMPConfig {
    uint8_t idle_rate;       // Oil injection at idle (0-100%)
    uint8_t cruise_rate;     // Oil injection at cruise (0-100%)
    uint8_t wot_rate;        // Oil injection at WOT (0-100%)
    uint16_t rpm_threshold;  // RPM for increased injection
    bool premix_mode;        // true = disable OMP (premix fuel)
};

/**
 * @brief Ignition configuration (dual spark)
 */
struct IgnitionConfig {
    // Leading spark (primary)
    uint8_t leading_advance_idle;    // Degrees BTDC at idle
    uint8_t leading_advance_cruise;  // Degrees BTDC at cruise
    uint8_t leading_advance_wot;     // Degrees BTDC at WOT

    // Trailing spark (secondary, typically retarded)
    uint8_t trailing_offset;         // Offset from leading (degrees)
    bool split_mode;                 // Split timing under load
};

/**
 * @brief Initialize rotary engine management
 */
void init();

/**
 * @brief Update rotary engine control
 *
 * @param rpm Current engine RPM
 * @param throttle Throttle position (0-100%)
 * @param coolant_temp Coolant temperature (°C * 10)
 */
void update(uint16_t rpm, uint8_t throttle, int16_t coolant_temp);

/**
 * @brief Control Oil Metering Pump (OMP)
 *
 * Critical for rotary engines - injects 2-stroke oil into combustion chamber
 * to lubricate apex seals.
 *
 * @param rpm Current RPM
 * @param throttle Throttle position (0-100%)
 * @return OMP duty cycle (0-100%)
 */
uint8_t controlOMP(uint16_t rpm, uint8_t throttle);

/**
 * @brief Calculate ignition timing (leading spark)
 *
 * @param rpm Current RPM
 * @param throttle Throttle position
 * @param knock_detected true if knock detected
 * @return Ignition advance in degrees BTDC
 */
uint8_t calculateLeadingTiming(uint16_t rpm, uint8_t throttle, bool knock_detected);

/**
 * @brief Calculate trailing spark timing
 *
 * @param leading_timing Leading spark timing
 * @param rpm Current RPM
 * @return Trailing timing in degrees BTDC
 */
uint8_t calculateTrailingTiming(uint8_t leading_timing, uint16_t rpm);

/**
 * @brief Monitor apex seal health
 *
 * Uses compression analysis, oil consumption, and blow-by detection
 *
 * @return Health status structure
 */
HealthStatus monitorApexSeals();

/**
 * @brief Check for coolant seal failure (rotary-specific issue)
 *
 * Coolant seal failure causes coolant to enter combustion chamber
 *
 * @return true if seal appears healthy
 */
bool checkCoolantSeal();

/**
 * @brief Estimate oil consumption rate
 *
 * @param distance_km Distance traveled since last oil check
 * @param oil_added_ml Oil added (ml)
 * @return Consumption rate (ml/100km)
 */
uint16_t estimateOilConsumption(uint32_t distance_km, uint16_t oil_added_ml);

/**
 * @brief Get engine health status
 * @return Current health status
 */
const HealthStatus* getHealthStatus();

/**
 * @brief Configure OMP settings
 * @param config OMP configuration
 */
void configureOMP(const OMPConfig& config);

/**
 * @brief Configure ignition timing
 * @param config Ignition configuration
 */
void configureIgnition(const IgnitionConfig& config);

/**
 * @brief Get recommended rebuild interval
 *
 * Based on usage pattern and health monitoring
 *
 * @return Estimated hours until rebuild recommended
 */
uint32_t getRecommendedRebuildHours();

/**
 * @brief Record engine operating hours
 *
 * Should be called periodically while engine running
 *
 * @param hours Hours to add (typically small increments)
 */
void recordOperatingHours(float hours);

} // namespace RotaryEngine

#endif // ROTARY_ENGINE_H
