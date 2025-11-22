/**
 * @file can_controller.h
 * @brief CAN bus controller for RX8 ECU replacement
 *
 * Handles all CAN bus communication for the RX8 vehicle.
 * Platform-independent interface, platform-specific implementation in HAL.
 *
 * @author Ported from legacy RX8_CANBUS.ino
 * @date 2025-11-16
 */

#pragma once

#include <stdint.h>

namespace CANController {

/**
 * Initialize CAN bus controller
 * @param speed CAN bus speed in bps (typically 500000 for RX8)
 * @return true if successful, false on error
 */
bool init(uint32_t speed);

/**
 * Process incoming CAN messages (non-blocking)
 * Call this frequently in main loop
 */
void process();

/**
 * Transmit a CAN message
 * @param id CAN message ID
 * @param data Pointer to data buffer (up to 8 bytes)
 * @param len Data length (0-8 bytes)
 * @return true if transmitted successfully
 */
bool transmit(uint32_t id, const uint8_t* data, uint8_t len);

/**
 * Check if a CAN message is available
 * @return true if message available
 */
bool messageAvailable();

/**
 * Read a CAN message
 * @param id Pointer to store message ID
 * @param len Pointer to store message length
 * @param data Pointer to buffer for data (at least 8 bytes)
 * @return true if message read successfully
 */
bool readMessage(uint32_t* id, uint8_t* len, uint8_t* data);

/**
 * Get last received message timestamp
 * @return Timestamp in milliseconds
 */
uint32_t getLastRxTime();

} // namespace CANController
