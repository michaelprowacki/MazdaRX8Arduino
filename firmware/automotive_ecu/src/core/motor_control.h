/**
 * @file motor_control.h
 * @brief Electric motor control for EV conversions
 *
 * Handles EV-specific functions:
 * - Motor RPM monitoring from inverter
 * - Motor temperature monitoring
 * - Motor controller communication
 * - Regen braking integration
 * - Precharge/contactor control
 *
 * @author Ported from legacy rx8can EV ECU
 * @date 2025-11-16
 */

#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <stdint.h>

namespace MotorControl {

/**
 * @brief Initialize motor control system
 *
 * Sets up:
 * - Motor controller communication (CAN)
 * - PWM outputs for precharge/contactors
 * - Default motor parameters
 */
void init();

/**
 * @brief Update motor control state
 *
 * Called every loop iteration to:
 * - Monitor motor status
 * - Update temperature readings
 * - Control precharge sequence
 *
 * @param throttle_percent Throttle position (0-100%)
 * @param vehicle_speed Vehicle speed (km/h * 10)
 */
void update(uint8_t throttle_percent, uint16_t vehicle_speed);

/**
 * @brief Process CAN message from motor controller/inverter
 *
 * Handles messages:
 * - ID 0x00A (10): Motor RPM
 * - ID 0x00F (15): Inverter temperature
 * - Motor controller specific messages (OpenInverter, custom)
 *
 * @param can_id CAN message ID
 * @param data Pointer to 8-byte CAN data
 */
void processCANMessage(uint32_t can_id, const uint8_t* data);

/**
 * @brief Get current motor RPM
 * @return Motor RPM (0-9000+)
 */
uint16_t getRPM();

/**
 * @brief Set motor RPM (for simulation/testing)
 * @param rpm Motor RPM
 */
void setRPM(uint16_t rpm);

/**
 * @brief Get motor/inverter temperature
 * @return Temperature in °C * 10
 */
int16_t getTemperature();

/**
 * @brief Set motor temperature (for simulation/testing)
 * @param temp Temperature in °C * 10
 */
void setTemperature(int16_t temp);

/**
 * @brief Check if motor is running
 * @return true if motor RPM > threshold
 */
bool isRunning();

/**
 * @brief Get precharge state
 * @return true if precharge active
 */
bool isPrechargeActive();

} // namespace MotorControl

#endif // MOTOR_CONTROL_H
