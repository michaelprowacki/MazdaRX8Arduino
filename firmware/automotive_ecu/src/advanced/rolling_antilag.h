/**
 * @file rolling_antilag.h
 * @brief Rolling anti-lag system for turbo boost retention
 *
 * Features:
 * - Anti-lag while moving (not just stationary)
 * - Gear-dependent activation
 * - Speed-based enable/disable
 * - TPS-based activation windows
 * - RPM-based activation windows
 * - Ignition timing retard
 * - Fuel cut control
 * - Boost retention between shifts
 *
 * How It Works:
 * Rolling anti-lag keeps the turbo spooled during lift-throttle conditions
 * (e.g., between gear shifts) by retarding ignition timing and cutting fuel
 * to maintain exhaust gas velocity and keep the turbine spinning.
 *
 * Unlike stationary anti-lag (2-step launch control), rolling anti-lag:
 * - Works while vehicle is moving
 * - Activates during throttle lift-off (between shifts)
 * - Deactivates when throttle reapplied
 * - More aggressive than normal engine braking
 * - Keeps boost pressure up for next gear
 *
 * Typical Use Case:
 * 1. Driver at WOT in 2nd gear, boost at 15 PSI
 * 2. Driver lifts throttle to shift to 3rd
 * 3. Anti-lag activates: retards timing 15°, cuts every other cylinder
 * 4. Exhaust flow maintained, turbo stays spooled
 * 5. Driver completes shift, reapplies throttle
 * 6. Boost still at 12 PSI (instead of 0 PSI normally)
 * 7. Instant power delivery
 *
 * Safety:
 * - Only activates above minimum speed (prevents idle activation)
 * - Only activates in specific gears (typically 2-5)
 * - Requires recent WOT (prevents activation during normal driving)
 * - Deactivates if conditions unsafe
 * - Coolant temperature check
 * - Boost pressure monitoring
 *
 * @author Created for Phase 5++ Haltech-style features
 * @date 2025-11-16
 */

#ifndef ROLLING_ANTILAG_H
#define ROLLING_ANTILAG_H

#include <stdint.h>

namespace RollingAntilag {

/**
 * @brief Rolling anti-lag configuration
 */
struct AntilagConfig {
    bool enabled;               // System enabled

    // Activation conditions
    uint16_t min_rpm;           // Minimum RPM to activate
    uint16_t max_rpm;           // Maximum RPM to activate
    uint16_t min_speed;         // Minimum speed (km/h * 10)
    uint16_t max_speed;         // Maximum speed (km/h * 10, 0 = no limit)
    uint8_t  min_gear;          // Minimum gear (1-6)
    uint8_t  max_gear;          // Maximum gear (1-6)

    // Throttle conditions
    uint8_t  tps_activate;      // TPS below this to activate (%)
    uint8_t  tps_deactivate;    // TPS above this to deactivate (%)
    uint16_t wot_memory_time;   // Time to remember WOT (ms)

    // Control parameters
    int8_t   timing_retard;     // Ignition timing retard (degrees)
    uint8_t  fuel_cut_ratio;    // Fuel cut ratio (0-100%, 50 = cut every other)
    uint16_t boost_target;      // Target boost to maintain (PSI * 10)

    // Safety
    uint16_t max_coolant_temp;  // Max coolant temp to operate (°C * 10)
    uint16_t max_boost;         // Max boost allowed (PSI * 10)
    uint16_t max_duration;      // Max continuous duration (ms)
};

/**
 * @brief Rolling anti-lag status
 */
struct AntilagStatus {
    bool active;                // Anti-lag currently active
    bool conditions_met;        // Activation conditions met
    bool wot_recently;          // WOT within memory time

    uint32_t activation_time;   // Time anti-lag activated (ms)
    uint32_t duration;          // Current activation duration (ms)
    uint32_t last_wot_time;     // Last time WOT detected (ms)

    uint16_t current_boost;     // Current boost pressure (PSI * 10)
    uint8_t  current_gear;      // Current gear
    uint16_t current_speed;     // Current speed (km/h * 10)
    uint16_t current_rpm;       // Current RPM
    uint8_t  current_tps;       // Current TPS (%)

    uint32_t total_activations; // Total number of activations
    uint32_t total_time;        // Total time active (ms)
};

/**
 * @brief Initialize rolling anti-lag
 *
 * @param config Configuration
 */
void init(const AntilagConfig& config);

/**
 * @brief Update rolling anti-lag
 *
 * Call this every loop iteration
 *
 * @param rpm Current RPM
 * @param throttle Throttle position (0-100%)
 * @param speed Vehicle speed (km/h * 10)
 * @param gear Current gear (1-6, 0 = neutral)
 * @param boost Boost pressure (PSI * 10)
 * @param coolant_temp Coolant temp (°C * 10)
 */
void update(uint16_t rpm, uint8_t throttle, uint16_t speed,
            uint8_t gear, uint16_t boost, int16_t coolant_temp);

/**
 * @brief Check if anti-lag should be active
 *
 * @param rpm Current RPM
 * @param throttle Throttle position
 * @param speed Vehicle speed
 * @param gear Current gear
 * @param boost Boost pressure
 * @param coolant_temp Coolant temp
 * @return true if should activate
 */
bool shouldActivate(uint16_t rpm, uint8_t throttle, uint16_t speed,
                   uint8_t gear, uint16_t boost, int16_t coolant_temp);

/**
 * @brief Get timing adjustment for anti-lag
 *
 * @return Timing retard (degrees, negative value)
 */
int8_t getTimingAdjustment();

/**
 * @brief Get fuel cut pattern
 *
 * Returns which cylinders to cut
 *
 * @param cylinder Cylinder number (0-based)
 * @return true if cylinder should be cut
 */
bool shouldCutCylinder(uint8_t cylinder);

/**
 * @brief Enable/disable rolling anti-lag
 *
 * @param enabled Enable/disable
 */
void setEnabled(bool enabled);

/**
 * @brief Get anti-lag status
 *
 * @return Status structure
 */
const AntilagStatus* getStatus();

/**
 * @brief Emergency deactivation
 */
void emergencyDeactivate();

// ============================================================================
// PRESET CONFIGURATIONS
// ============================================================================

/**
 * @brief Create street configuration
 *
 * Conservative settings for street use
 * - Moderate timing retard (10°)
 * - Light fuel cut (25%)
 * - Active in gears 2-4 only
 *
 * @return Config structure
 */
AntilagConfig createStreetConfig();

/**
 * @brief Create track configuration
 *
 * Aggressive settings for track use
 * - Heavy timing retard (15°)
 * - Moderate fuel cut (50%)
 * - Active in gears 2-5
 *
 * @return Config structure
 */
AntilagConfig createTrackConfig();

/**
 * @brief Create drag racing configuration
 *
 * Maximum aggression for drag racing
 * - Extreme timing retard (20°)
 * - Heavy fuel cut (75%)
 * - Active in gears 1-4 (1/4 mile)
 *
 * @return Config structure
 */
AntilagConfig createDragConfig();

} // namespace RollingAntilag

#endif // ROLLING_ANTILAG_H
