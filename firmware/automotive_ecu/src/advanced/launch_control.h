/**
 * @file launch_control.h
 * @brief Launch control system for optimal acceleration from standstill
 *
 * Features:
 * - RPM limiting during launch
 * - Ignition cut/retard for traction
 * - 2-step rev limiter (launch + main)
 * - Boost building at standstill
 * - Clutch switch integration
 * - Anti-wheelie control (RWD)
 *
 * @author Created for Phase 5+ advanced features
 * @date 2025-11-16
 */

#ifndef LAUNCH_CONTROL_H
#define LAUNCH_CONTROL_H

#include <stdint.h>

namespace LaunchControl {

/**
 * @brief Launch control configuration
 */
struct LaunchConfig {
    uint16_t launch_rpm;        // Launch RPM limit (e.g., 4000 RPM)
    uint16_t main_rpm_limit;    // Main rev limiter (e.g., 9000 RPM)
    uint8_t  ignition_cut_mode; // 0=soft cut, 1=hard cut, 2=retard
    uint8_t  launch_retard;     // Ignition retard during launch (degrees)
    bool     boost_build;       // Build boost at standstill
    uint16_t speed_threshold;   // Speed above which launch deactivates (km/h * 10)
    bool     require_clutch;    // Require clutch switch for activation
};

/**
 * @brief Launch control status
 */
struct LaunchStatus {
    bool armed;                 // Launch control armed (waiting for launch)
    bool active;                // Launch control active (limiting RPM)
    uint16_t current_rpm_limit; // Current RPM limit in effect
    uint8_t ignition_cuts;      // Number of ignition cuts (diagnostic)
    bool clutch_detected;       // Clutch switch state
};

/**
 * @brief Initialize launch control
 */
void init();

/**
 * @brief Update launch control
 *
 * @param rpm Current engine RPM
 * @param throttle Throttle position (0-100%)
 * @param speed Vehicle speed (km/h * 10)
 * @param clutch_pressed true if clutch pedal pressed
 * @return true if launch control is limiting RPM
 */
bool update(uint16_t rpm, uint8_t throttle, uint16_t speed, bool clutch_pressed);

/**
 * @brief Arm launch control
 *
 * Conditions to arm:
 * - Vehicle stationary (speed = 0)
 * - Clutch pressed (if required)
 * - Throttle > 90%
 */
void arm();

/**
 * @brief Disarm launch control
 */
void disarm();

/**
 * @brief Check if launch control should activate
 *
 * @param rpm Current RPM
 * @param throttle Throttle position
 * @param speed Vehicle speed
 * @param clutch_pressed Clutch state
 * @return true if should activate
 */
bool shouldActivate(uint16_t rpm, uint8_t throttle, uint16_t speed, bool clutch_pressed);

/**
 * @brief Apply RPM limiting
 *
 * @param rpm Current RPM
 * @return Ignition cut/retard command (0=normal, 1=cut, 2=retard)
 */
uint8_t applyRPMLimiting(uint16_t rpm);

/**
 * @brief Get launch control status
 * @return Status structure
 */
const LaunchStatus* getStatus();

/**
 * @brief Configure launch control
 * @param config Configuration structure
 */
void configure(const LaunchConfig& config);

/**
 * @brief Set launch RPM limit
 *
 * @param rpm Launch RPM limit (typical: 3000-5000 RPM)
 */
void setLaunchRPM(uint16_t rpm);

/**
 * @brief Enable/disable launch control
 *
 * @param enabled true to enable
 */
void setEnabled(bool enabled);

/**
 * @brief Get number of successful launches
 *
 * Diagnostic counter for number of times launch control was used
 *
 * @return Launch counter
 */
uint32_t getLaunchCount();

} // namespace LaunchControl

#endif // LAUNCH_CONTROL_H
