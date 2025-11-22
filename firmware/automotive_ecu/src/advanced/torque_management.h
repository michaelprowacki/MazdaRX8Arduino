/**
 * @file torque_management.h
 * @brief Advanced torque management and limiting
 *
 * Features:
 * - Torque-based power limiting (not just RPM/boost)
 * - Gear-dependent torque limits
 * - Driveshaft RPM limiting (prevents driveline damage)
 * - Launch torque control
 * - Shift torque reduction
 * - Traction-based torque modulation
 * - Progressive torque delivery
 * - Torque curve shaping
 *
 * Use Cases:
 * 1. **Driveline Protection**
 *    - Limit torque in lower gears to prevent driveshaft/axle damage
 *    - Higher torque allowed in higher gears (lower wheel torque)
 *
 * 2. **Traction Management**
 *    - Reduce torque when wheel slip detected
 *    - Progressive torque application on launch
 *
 * 3. **Transmission Protection**
 *    - Reduce torque during shifts
 *    - Smooth torque delivery prevents transmission shock
 *
 * 4. **Performance Optimization**
 *    - Shape torque curve for better acceleration
 *    - Prevent bog or wheelspin
 *
 * Torque Control Methods:
 * - Ignition timing retard (primary method)
 * - Boost pressure reduction (turbo applications)
 * - Throttle position limiting (drive-by-wire)
 * - Fuel delivery reduction (less common)
 *
 * Example Torque Limits (RX8 with turbo):
 * - 1st gear: 250 lb-ft (prevent wheelspin)
 * - 2nd gear: 300 lb-ft
 * - 3rd gear: 350 lb-ft
 * - 4th-6th: 400 lb-ft (maximum)
 *
 * @author Created for Phase 5++ Haltech-style features
 * @date 2025-11-16
 */

#ifndef TORQUE_MANAGEMENT_H
#define TORQUE_MANAGEMENT_H

#include <stdint.h>

namespace TorqueManagement {

/**
 * @brief Torque limit mode
 */
enum TorqueMode {
    MODE_DISABLED,          // No torque management
    MODE_DRIVELINE,         // Driveline protection (gear-based limits)
    MODE_TRACTION,          // Traction-based limiting
    MODE_SHIFT,             // Shift torque reduction
    MODE_PROGRESSIVE        // Progressive torque delivery
};

/**
 * @brief Torque management configuration
 */
struct TorqueConfig {
    TorqueMode mode;            // Operating mode

    // Gear-based torque limits (lb-ft)
    uint16_t torque_limit_gear[7]; // Per gear (0 = neutral, 1-6 = gears)

    // Driveshaft RPM limiting
    uint16_t max_driveshaft_rpm;   // Maximum driveshaft RPM
    float    final_drive_ratio;    // Final drive ratio (e.g., 4.44 for RX8)

    // Progressive delivery
    uint16_t ramp_rate;         // Torque ramp rate (lb-ft/sec)
    uint16_t min_torque;        // Minimum torque (lb-ft)
    uint16_t max_torque;        // Maximum torque (lb-ft)

    // Shift torque reduction
    uint16_t shift_reduction;   // Torque reduction during shift (%)
    uint16_t shift_duration;    // Shift duration (ms)

    // Traction control integration
    bool integrate_traction_control; // Use TC slip detection
    uint16_t slip_torque_reduction;  // Torque reduction per 1% slip (%)

    // Control method
    uint8_t timing_authority;   // Max timing retard for torque control (degrees)
    uint8_t boost_authority;    // Max boost reduction for torque control (PSI)
};

/**
 * @brief Torque management status
 */
struct TorqueStatus {
    bool active;                // Torque limiting active
    TorqueMode current_mode;    // Current operating mode

    uint16_t target_torque;     // Target torque (lb-ft)
    uint16_t current_torque;    // Current torque estimate (lb-ft)
    uint16_t torque_limit;      // Active torque limit (lb-ft)
    uint16_t torque_reduction;  // Current reduction (%)

    // Driveshaft
    uint16_t driveshaft_rpm;    // Calculated driveshaft RPM

    // Control outputs
    int8_t  timing_adjustment;  // Timing retard for torque control (degrees)
    uint16_t boost_adjustment;  // Boost reduction for torque control (PSI * 10)

    // Shift detection
    bool shifting;              // Shift in progress
    uint32_t shift_start_time;  // Shift start time (ms)

    // Statistics
    uint32_t limit_events;      // Number of times limit reached
    uint32_t total_limit_time;  // Total time limiting (ms)
};

/**
 * @brief Initialize torque management
 *
 * @param config Configuration
 */
void init(const TorqueConfig& config);

/**
 * @brief Update torque management
 *
 * Call this every loop iteration
 *
 * @param rpm Engine RPM
 * @param throttle Throttle position (0-100%)
 * @param gear Current gear (0-6)
 * @param boost Boost pressure (PSI * 10)
 * @param wheel_slip Wheel slip ratio (% * 10, from traction control)
 */
void update(uint16_t rpm, uint8_t throttle, uint8_t gear,
            uint16_t boost, uint16_t wheel_slip);

/**
 * @brief Estimate current engine torque
 *
 * Uses RPM, boost, throttle to estimate torque
 *
 * @param rpm Engine RPM
 * @param boost Boost pressure
 * @param throttle Throttle position
 * @return Estimated torque (lb-ft)
 */
uint16_t estimateTorque(uint16_t rpm, uint16_t boost, uint8_t throttle);

/**
 * @brief Calculate driveshaft RPM
 *
 * @param engine_rpm Engine RPM
 * @param gear Current gear
 * @return Driveshaft RPM
 */
uint16_t calculateDriveshaftRPM(uint16_t engine_rpm, uint8_t gear);

/**
 * @brief Calculate torque limit for current conditions
 *
 * @param gear Current gear
 * @param driveshaft_rpm Driveshaft RPM
 * @param wheel_slip Wheel slip
 * @return Torque limit (lb-ft)
 */
uint16_t calculateTorqueLimit(uint8_t gear, uint16_t driveshaft_rpm,
                              uint16_t wheel_slip);

/**
 * @brief Calculate timing retard to achieve torque reduction
 *
 * @param torque_reduction Desired reduction (%)
 * @return Timing retard (degrees)
 */
int8_t calculateTimingRetard(uint16_t torque_reduction);

/**
 * @brief Calculate boost reduction to achieve torque reduction
 *
 * @param torque_reduction Desired reduction (%)
 * @param current_boost Current boost pressure
 * @return Boost reduction (PSI * 10)
 */
uint16_t calculateBoostReduction(uint16_t torque_reduction, uint16_t current_boost);

/**
 * @brief Detect gear shift
 *
 * Detects shift by monitoring RPM drop
 *
 * @param rpm Current RPM
 * @param gear Current gear
 * @return true if shift detected
 */
bool detectShift(uint16_t rpm, uint8_t gear);

/**
 * @brief Activate shift torque reduction
 */
void activateShiftReduction();

/**
 * @brief Deactivate shift torque reduction
 */
void deactivateShiftReduction();

/**
 * @brief Set torque mode
 *
 * @param mode Torque mode
 */
void setMode(TorqueMode mode);

/**
 * @brief Get torque status
 *
 * @return Status structure
 */
const TorqueStatus* getStatus();

// ============================================================================
// PRESET CONFIGURATIONS
// ============================================================================

/**
 * @brief Create driveline protection config
 *
 * Limits torque in lower gears to protect driveline
 *
 * @return Config structure
 */
TorqueConfig createDrivelineProtectionConfig();

/**
 * @brief Create traction optimization config
 *
 * Optimizes torque delivery for traction
 *
 * @return Config structure
 */
TorqueConfig createTractionOptimizationConfig();

/**
 * @brief Create drag racing config
 *
 * Optimizes for straight-line acceleration
 *
 * @return Config structure
 */
TorqueConfig createDragRacingConfig();

} // namespace TorqueManagement

#endif // TORQUE_MANAGEMENT_H
