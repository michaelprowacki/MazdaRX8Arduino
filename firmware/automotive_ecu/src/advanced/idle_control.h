/**
 * @file idle_control.h
 * @brief Closed-loop idle control via IAC (Idle Air Control) valve
 *
 * Features:
 * - PID-based closed-loop control
 * - Target RPM based on coolant temp
 * - AC compressor compensation
 * - Power steering load compensation
 * - Idle-up for cold engine
 * - Dash pot (throttle closing damping)
 *
 * @author Created for Phase 5+ Haltech-style features
 * @date 2025-11-16
 */

#ifndef IDLE_CONTROL_H
#define IDLE_CONTROL_H

#include <stdint.h>

namespace IdleControl {

/**
 * @brief Idle control mode
 */
enum ControlMode {
    MODE_DISABLED,          // No idle control
    MODE_OPEN_LOOP,         // Fixed IAC position (no feedback)
    MODE_CLOSED_LOOP        // PID feedback control
};

/**
 * @brief Idle control configuration
 */
struct IdleConfig {
    ControlMode mode;           // Control mode
    uint16_t target_rpm_warm;   // Target idle RPM (warm engine)
    uint16_t target_rpm_cold;   // Target idle RPM (cold engine)
    uint16_t warm_temp;         // Warm engine temp threshold (°C * 10)
    uint8_t  ac_rpm_adder;      // RPM increase with AC on
    uint8_t  ps_rpm_adder;      // RPM increase with power steering load
    uint8_t  dashpot_delay;     // Dashpot delay (seconds * 10)
    float    pid_kp;            // PID proportional gain
    float    pid_ki;            // PID integral gain
    float    pid_kd;            // PID derivative gain
};

/**
 * @brief Idle control status
 */
struct IdleStatus {
    uint16_t target_rpm;        // Current target RPM
    uint16_t actual_rpm;        // Actual engine RPM
    int16_t  rpm_error;         // Error (target - actual)
    uint8_t  iac_position;      // IAC valve position (0-100%)
    bool     ac_active;         // AC compressor active
    bool     ps_load;           // Power steering load detected
    bool     dashpot_active;    // Dashpot active (throttle closing)
    bool     idle_active;       // Engine at idle (throttle closed)
};

/**
 * @brief Initialize idle control
 */
void init();

/**
 * @brief Update idle control
 *
 * Call this every loop (10-50 ms typical)
 *
 * @param rpm Current engine RPM
 * @param throttle Throttle position (0-100%)
 * @param coolant_temp Coolant temperature (°C * 10)
 * @param ac_on AC compressor active
 * @param ps_load Power steering load detected
 * @return IAC valve position (0-100%)
 */
uint8_t update(uint16_t rpm, uint8_t throttle, int16_t coolant_temp,
               bool ac_on, bool ps_load);

/**
 * @brief Calculate target idle RPM
 *
 * Based on coolant temp with warm-up curve
 *
 * @param coolant_temp Coolant temperature (°C * 10)
 * @return Target RPM
 */
uint16_t calculateTargetRPM(int16_t coolant_temp);

/**
 * @brief PID controller for IAC position
 *
 * @param error RPM error (target - actual)
 * @param dt Time delta (seconds)
 * @return IAC adjustment (delta position)
 */
int16_t calculatePID(int16_t error, float dt);

/**
 * @brief Dashpot control (smooth throttle closing)
 *
 * Prevents stalling when throttle suddenly closed
 *
 * @param throttle Current throttle position
 * @param rpm Current RPM
 * @return Additional IAC opening for dashpot (%)
 */
uint8_t calculateDashpot(uint8_t throttle, uint16_t rpm);

/**
 * @brief Detect idle condition
 *
 * @param throttle Throttle position (0-100%)
 * @param rpm Current RPM
 * @return true if at idle
 */
bool isIdle(uint8_t throttle, uint16_t rpm);

/**
 * @brief Get idle control status
 * @return Status structure
 */
const IdleStatus* getStatus();

/**
 * @brief Configure idle control
 * @param config Configuration structure
 */
void configure(const IdleConfig& config);

/**
 * @brief Set control mode
 *
 * @param mode Control mode
 */
void setMode(ControlMode mode);

/**
 * @brief Set target idle RPM
 *
 * @param rpm Target RPM (warm engine)
 */
void setTargetRPM(uint16_t rpm);

/**
 * @brief Manual IAC position (open-loop mode)
 *
 * @param position IAC position (0-100%)
 */
void setIACPosition(uint8_t position);

/**
 * @brief Reset PID integrator
 *
 * Call when entering/exiting idle
 */
void resetPID();

} // namespace IdleControl

#endif // IDLE_CONTROL_H
