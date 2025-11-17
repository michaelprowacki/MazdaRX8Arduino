/**
 * @file abs_dsc.h
 * @brief ABS/DSC system emulation for RX8
 *
 * Emulates ABS (Anti-lock Braking System) and DSC (Dynamic Stability Control)
 * when factory modules are removed or bypassed.
 *
 * CAN Messages:
 * - 0x620 (1568): ABS system data
 * - 0x630 (1584): ABS configuration (AT/MT, wheel size)
 * - 0x650 (1616): ABS supplement
 * - 0x212 (530): DSC status (optional, usually disabled)
 *
 * @author Ported from legacy RX8_CANBUS.ino
 * @date 2025-11-16
 */

#ifndef ABS_DSC_H
#define ABS_DSC_H

#include <stdint.h>

namespace ABSDSC {

/**
 * @brief Initialize ABS/DSC emulation
 *
 * Sets up default states for:
 * - ABS system status
 * - DSC system status
 * - Transmission type (AT/MT)
 * - Wheel size parameters
 */
void init();

/**
 * @brief Update ABS/DSC state
 *
 * Called periodically to update system status based on:
 * - Vehicle speed
 * - Wheel speeds
 * - Brake pressure
 * - Stability events
 *
 * @param vehicle_speed Current vehicle speed (km/h * 10)
 */
void update(uint16_t vehicle_speed);

/**
 * @brief Get ABS warning light state
 * @return true if ABS light should be on
 */
bool getABSWarning();

/**
 * @brief Set ABS warning light state
 * @param state true to turn on ABS light
 */
void setABSWarning(bool state);

/**
 * @brief Get DSC off indicator state
 * @return true if DSC is off
 */
bool getDSCOff();

/**
 * @brief Set DSC off state
 * @param state true to indicate DSC is off
 */
void setDSCOff(bool state);

/**
 * @brief Get brake failure warning state
 * @return true if brake failure warning active
 */
bool getBrakeFailWarning();

/**
 * @brief Set brake failure warning
 * @param state true to activate brake failure warning
 */
void setBrakeFailWarning(bool state);

/**
 * @brief Get Electronic Throttle Control (ETC) active state
 * @return true if ETC is actively intervening
 */
bool getETCActive();

/**
 * @brief Set ETC active state
 * @param state true if ETC is active
 */
void setETCActive(bool state);

/**
 * @brief Get ETC disabled state
 * @return true if ETC is disabled
 */
bool getETCDisabled();

/**
 * @brief Set ETC disabled state
 * @param state true to disable ETC
 */
void setETCDisabled(bool state);

/**
 * @brief Transmit ABS/DSC CAN messages
 *
 * Sends:
 * - 0x620: ABS system data
 * - 0x630: ABS configuration
 * - 0x650: ABS supplement
 * - 0x212: DSC status (if enabled in config)
 */
void transmitCANMessages();

} // namespace ABSDSC

#endif // ABS_DSC_H
