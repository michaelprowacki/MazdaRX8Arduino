/**
 * @file boost_control.h
 * @brief Turbo boost control for turbocharged 13B-MSP engines
 *
 * Features:
 * - Electronic wastegate control
 * - Boost-by-gear functionality
 * - Anti-lag system (ALS)
 * - Overboost protection
 * - Closed-loop boost control
 * - Boost ramping/spool control
 *
 * @author Created for Phase 5+ advanced features
 * @date 2025-11-16
 */

#ifndef BOOST_CONTROL_H
#define BOOST_CONTROL_H

#include <stdint.h>

namespace BoostControl {

/**
 * @brief Boost control configuration
 */
struct BoostConfig {
    uint16_t target_boost_psi[6];  // Target boost per gear (PSI * 10)
    uint16_t max_boost_psi;         // Absolute max boost (safety limit)
    uint8_t  boost_ramp_rate;       // How fast to build boost (0-100)
    bool     antilag_enabled;       // Anti-lag system enabled
    uint8_t  wastegate_preload;     // Wastegate spring preload (0-100%)
    uint16_t spool_rpm_threshold;   // Min RPM for boost control
};

/**
 * @brief Boost control status
 */
struct BoostStatus {
    uint16_t current_boost_psi;     // Current boost pressure (PSI * 10)
    uint16_t target_boost_psi;      // Current target boost
    uint8_t  wastegate_duty;        // Wastegate duty cycle (0-100%)
    bool     overboost_detected;    // Overboost protection active
    bool     antilag_active;        // Anti-lag currently firing
    uint8_t  current_gear;          // Current transmission gear
};

/**
 * @brief Initialize boost control system
 */
void init();

/**
 * @brief Update boost control
 *
 * @param rpm Current engine RPM
 * @param throttle Throttle position (0-100%)
 * @param current_boost Current boost pressure (PSI * 10)
 * @param gear Current gear (1-6, 0 = neutral)
 * @return Wastegate duty cycle (0-100%)
 */
uint8_t update(uint16_t rpm, uint8_t throttle, uint16_t current_boost, uint8_t gear);

/**
 * @brief Calculate wastegate duty cycle (closed-loop control)
 *
 * @param target_boost Target boost pressure
 * @param current_boost Current boost pressure
 * @param rpm Current RPM
 * @return Wastegate duty cycle (0-100%)
 */
uint8_t calculateWastegateDuty(uint16_t target_boost, uint16_t current_boost, uint16_t rpm);

/**
 * @brief Anti-lag system control
 *
 * Retards ignition and injects fuel on deceleration to keep turbo spooled
 *
 * @param rpm Current RPM
 * @param throttle Throttle position
 * @param boost Current boost
 * @return true if anti-lag should be active
 */
bool controlAntiLag(uint16_t rpm, uint8_t throttle, uint16_t boost);

/**
 * @brief Check for overboost condition
 *
 * @param boost Current boost pressure
 * @return true if overboost protection should activate
 */
bool checkOverboost(uint16_t boost);

/**
 * @brief Get current boost control status
 * @return Boost status structure
 */
const BoostStatus* getStatus();

/**
 * @brief Configure boost control
 * @param config Boost configuration
 */
void configure(const BoostConfig& config);

/**
 * @brief Set target boost for specific gear
 *
 * @param gear Gear number (1-6)
 * @param boost_psi Target boost in PSI * 10
 */
void setGearBoost(uint8_t gear, uint16_t boost_psi);

/**
 * @brief Enable/disable anti-lag system
 *
 * WARNING: Anti-lag increases exhaust temperatures significantly!
 *
 * @param enabled true to enable anti-lag
 */
void setAntiLagEnabled(bool enabled);

} // namespace BoostControl

#endif // BOOST_CONTROL_H
