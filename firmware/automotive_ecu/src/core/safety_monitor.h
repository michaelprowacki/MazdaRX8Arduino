/**
 * @file safety_monitor.h
 * @brief Safety monitoring and watchdog for automotive ECU
 *
 * Critical safety features:
 * - Hardware watchdog timer (must kick < 1000ms)
 * - CAN bus timeout detection
 * - Failsafe mode activation
 * - Temperature monitoring
 * - Voltage monitoring
 *
 * **SAFETY CRITICAL**: This module MUST be called every loop iteration
 * or the system will reset via hardware watchdog.
 *
 * @author Created for unified automotive ECU
 * @date 2025-11-16
 */

#ifndef SAFETY_MONITOR_H
#define SAFETY_MONITOR_H

#include <stdint.h>

namespace SafetyMonitor {

/**
 * @brief Safety state enumeration
 */
enum class SafetyState {
    NORMAL,      // Normal operation
    WARNING,     // Warning condition detected
    FAILSAFE     // Failsafe mode active
};

/**
 * @brief Initialize safety monitoring system
 *
 * Sets up:
 * - Hardware watchdog timer (platform-specific timeout)
 * - CAN timeout detection
 * - Voltage/temperature limits
 * - Failsafe default state
 */
void init();

/**
 * @brief Kick the watchdog timer
 *
 * **MUST** be called every loop iteration (< 1000ms on most platforms).
 * Failure to call will result in hardware reset.
 *
 * Also performs safety checks:
 * - CAN bus timeout
 * - Temperature limits
 * - Voltage limits
 */
void kick();

/**
 * @brief Update safety monitoring
 *
 * Called periodically to check:
 * - CAN bus activity
 * - System temperatures
 * - Battery voltage
 * - Critical sensor health
 *
 * @param can_rx_active true if CAN messages received recently
 */
void update(bool can_rx_active);

/**
 * @brief Get current safety state
 * @return Current safety state (NORMAL, WARNING, or FAILSAFE)
 */
SafetyState getState();

/**
 * @brief Enter failsafe mode
 *
 * Failsafe actions:
 * - Close throttle to 0%
 * - Set RPM to 0 (or idle)
 * - Activate all warning lights
 * - Disable motor control (EV)
 * - Log error condition
 *
 * @param reason Reason for entering failsafe (for logging)
 */
void enterFailsafe(const char* reason);

/**
 * @brief Exit failsafe mode
 *
 * Allows recovery from failsafe after conditions clear.
 * Requires manual reset or timeout period.
 */
void exitFailsafe();

/**
 * @brief Check if CAN bus timeout has occurred
 *
 * @param timeout_ms Timeout period in milliseconds (default: 500ms)
 * @return true if no CAN RX for timeout_ms
 */
bool isCANTimeout(uint32_t timeout_ms = 500);

/**
 * @brief Update last CAN RX timestamp
 *
 * Call this when CAN messages are received to reset timeout.
 */
void updateCANActivity();

/**
 * @brief Get time since last CAN RX
 * @return Milliseconds since last CAN activity
 */
uint32_t getTimeSinceCANActivity();

/**
 * @brief Check if temperature is within safe limits
 *
 * @param temp Temperature in °C * 10
 * @return true if temperature is safe
 */
bool isTemperatureSafe(int16_t temp);

/**
 * @brief Check if voltage is within safe limits
 *
 * @param voltage Voltage in V * 100
 * @return true if voltage is safe
 */
bool isVoltageSafe(uint16_t voltage);

/**
 * @brief Get failsafe reason (for diagnostics)
 * @return Pointer to failsafe reason string
 */
const char* getFailsafeReason();

} // namespace SafetyMonitor

#endif // SAFETY_MONITOR_H
