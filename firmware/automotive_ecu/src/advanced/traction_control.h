/**
 * @file traction_control.h
 * @brief Active traction control system with wheel-speed based power reduction
 *
 * Features:
 * - Individual wheel speed monitoring (all 4 wheels)
 * - Slip ratio calculation (driven vs non-driven wheels)
 * - Progressive power reduction (ignition cut + timing retard)
 * - Adjustable slip threshold
 * - Speed-sensitive intervention
 * - Launch mode integration
 * - Manual on/off switch
 *
 * How it works:
 * 1. Monitor all 4 wheel speeds from CAN (0x4B1)
 * 2. Calculate slip ratio (rear vs front wheels)
 * 3. If slip > threshold → reduce power progressively
 * 4. Power reduction methods:
 *    - Soft: Timing retard only
 *    - Medium: Timing retard + partial ignition cut
 *    - Hard: Aggressive ignition cut
 *
 * Benefits for RWD RX8:
 * - Prevents wheelspin in 1st/2nd gear
 * - Faster acceleration (more grip = more speed)
 * - Safer in wet conditions
 * - Protects drivetrain from shock loads
 *
 * @author Created for Phase 5++ Haltech-style features
 * @date 2025-11-16
 */

#ifndef TRACTION_CONTROL_H
#define TRACTION_CONTROL_H

#include <stdint.h>

namespace TractionControl {

/**
 * @brief Traction control mode
 */
enum TCMode {
    TC_OFF,             // Traction control disabled
    TC_SOFT,            // Gentle intervention (timing retard only)
    TC_MEDIUM,          // Moderate intervention (timing + partial cut)
    TC_HARD             // Aggressive intervention (full ignition cut)
};

/**
 * @brief Wheel speeds structure
 */
struct WheelSpeeds {
    uint16_t front_left;    // km/h * 10
    uint16_t front_right;   // km/h * 10
    uint16_t rear_left;     // km/h * 10
    uint16_t rear_right;    // km/h * 10
};

/**
 * @brief Traction control configuration
 */
struct TCConfig {
    TCMode mode;                // Intervention mode
    uint8_t slip_threshold;     // Slip threshold (% * 10, e.g., 100 = 10%)
    uint8_t max_power_reduction; // Max power reduction (0-100%)
    uint16_t min_speed;         // Min speed for TC (km/h * 10)
    uint16_t max_speed;         // Max speed for TC (km/h * 10, 0=no limit)
    bool launch_mode_active;    // Enhanced TC during launch
    uint8_t intervention_rate;  // How fast to apply power reduction (0-100)
    uint8_t recovery_rate;      // How fast to restore power (0-100)
};

/**
 * @brief Traction control status
 */
struct TCStatus {
    bool active;                // TC currently intervening
    bool slip_detected;         // Slip above threshold
    uint16_t slip_ratio;        // Current slip ratio (% * 10)
    uint8_t power_reduction;    // Current power reduction (0-100%)
    uint8_t ignition_cuts;      // Ignition events cut (diagnostic)
    int8_t timing_retard;       // Timing retard applied (degrees)
    uint16_t intervention_count; // Number of interventions this session
    WheelSpeeds wheel_speeds;   // Current wheel speeds
};

/**
 * @brief Initialize traction control
 */
void init();

/**
 * @brief Update traction control
 *
 * @param wheel_speeds Current wheel speeds from CAN
 * @param throttle Throttle position (0-100%)
 * @param rpm Current RPM
 * @param gear Current gear (0-6)
 * @return Power reduction amount (0-100%)
 */
uint8_t update(const WheelSpeeds& wheel_speeds, uint8_t throttle,
               uint16_t rpm, uint8_t gear);

/**
 * @brief Calculate slip ratio
 *
 * For RWD: Compares rear wheels (driven) to front wheels (non-driven)
 * Slip ratio = (rear_speed - front_speed) / front_speed * 100
 *
 * @param wheel_speeds Current wheel speeds
 * @return Slip ratio (% * 10, e.g., 100 = 10% slip)
 */
uint16_t calculateSlipRatio(const WheelSpeeds& wheel_speeds);

/**
 * @brief Calculate power reduction based on slip
 *
 * Progressive power reduction:
 * - 0-5% slip: No intervention
 * - 5-10% slip: Light reduction (10-30%)
 * - 10-15% slip: Medium reduction (30-60%)
 * - 15%+ slip: Heavy reduction (60-100%)
 *
 * @param slip_ratio Current slip ratio (% * 10)
 * @return Power reduction (0-100%)
 */
uint8_t calculatePowerReduction(uint16_t slip_ratio);

/**
 * @brief Apply power reduction
 *
 * Methods:
 * - SOFT: Timing retard only (-0 to -10°)
 * - MEDIUM: Timing retard + cut every other ignition event
 * - HARD: Cut all ignition events when slip detected
 *
 * @param power_reduction Power reduction amount (0-100%)
 * @param mode TC mode
 * @return Ignition cut command (0=normal, 1=cut, 2=retard)
 */
uint8_t applyPowerReduction(uint8_t power_reduction, TCMode mode);

/**
 * @brief Calculate timing retard for traction control
 *
 * @param power_reduction Power reduction amount (0-100%)
 * @return Timing retard (degrees, 0-10°)
 */
int8_t calculateTimingRetard(uint8_t power_reduction);

/**
 * @brief Check if traction control should be active
 *
 * Conditions:
 * - Speed in valid range
 * - Throttle above threshold
 * - Not disabled by user
 *
 * @param speed Vehicle speed (km/h * 10)
 * @param throttle Throttle position (0-100%)
 * @return true if TC should be active
 */
bool shouldBeActive(uint16_t speed, uint8_t throttle);

/**
 * @brief Get traction control status
 * @return Status structure
 */
const TCStatus* getStatus();

/**
 * @brief Configure traction control
 * @param config Configuration structure
 */
void configure(const TCConfig& config);

/**
 * @brief Set traction control mode
 *
 * @param mode TC mode
 */
void setMode(TCMode mode);

/**
 * @brief Set slip threshold
 *
 * @param threshold Slip threshold (% * 10, e.g., 100 = 10%)
 */
void setSlipThreshold(uint8_t threshold);

/**
 * @brief Enable/disable launch mode
 *
 * Launch mode uses more aggressive TC settings for better launches
 *
 * @param enabled true to enable
 */
void setLaunchMode(bool enabled);

/**
 * @brief Reset intervention counter
 */
void resetCounters();

/**
 * @brief Create preset configurations
 */
TCConfig createStreetConfig();      // Conservative for street use
TCConfig createTrackConfig();       // Aggressive for track use
TCConfig createDragConfig();        // Optimized for drag racing

} // namespace TractionControl

#endif // TRACTION_CONTROL_H
