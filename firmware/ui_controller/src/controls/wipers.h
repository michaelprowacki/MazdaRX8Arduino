/**
 * @file wipers.h
 * @brief Speed-sensitive wiper control
 *
 * Controls windshield wipers based on vehicle speed:
 * - Intermittent mode at low speed (< 30 km/h)
 * - Continuous mode at high speed (> 30 km/h)
 * - Adjustable sensitivity
 *
 * Ported from legacy Wipers_Module (now deprecated)
 * to unified ESP32 UI controller.
 *
 * @author Ported for Phase 5 unified architecture
 * @date 2025-11-16
 */

#ifndef WIPERS_H
#define WIPERS_H

#include <stdint.h>

namespace Wipers {

/**
 * @brief Wiper mode enumeration
 */
enum Mode {
    MODE_OFF,           // Wipers off
    MODE_INTERMITTENT,  // Intermittent (speed-sensitive)
    MODE_LOW,           // Continuous low speed
    MODE_HIGH,          // Continuous high speed
    MODE_AUTO           // Automatic (rain sensor, if available)
};

/**
 * @brief Initialize wiper control
 *
 * Sets up:
 * - GPIO for wiper relay control
 * - Default sensitivity
 */
void init();

/**
 * @brief Update wiper control (call from FreeRTOS task)
 *
 * @param vehicle_speed Current vehicle speed (km/h)
 */
void update(uint16_t vehicle_speed);

/**
 * @brief Set wiper mode
 * @param mode Wiper mode
 */
void setMode(Mode mode);

/**
 * @brief Get current wiper mode
 * @return Current mode
 */
Mode getMode();

/**
 * @brief Set intermittent sensitivity
 *
 * Controls delay between wipes in intermittent mode:
 * - 0 = slowest (5s delay)
 * - 100 = fastest (1s delay)
 *
 * @param sensitivity Sensitivity percentage (0-100)
 */
void setSensitivity(uint8_t sensitivity);

/**
 * @brief Get current sensitivity
 * @return Sensitivity percentage (0-100)
 */
uint8_t getSensitivity();

/**
 * @brief Manual wiper activation (single wipe)
 */
void singleWipe();

/**
 * @brief Enable/disable speed-sensitive mode
 *
 * When enabled, intermittent delay adjusts based on speed:
 * - Low speed: longer delay
 * - High speed: shorter delay
 *
 * @param enabled true to enable speed-sensitive mode
 */
void setSpeedSensitive(bool enabled);

/**
 * @brief Check if wipers are currently active
 * @return true if wipers are running
 */
bool isActive();

/**
 * @brief Get time until next wipe (intermittent mode only)
 * @return Milliseconds until next wipe, or 0 if not in intermittent mode
 */
uint32_t getTimeUntilNextWipe();

} // namespace Wipers

#endif // WIPERS_H
