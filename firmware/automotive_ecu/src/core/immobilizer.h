/**
 * @file immobilizer.h
 * @brief Immobilizer bypass for RX8
 *
 * Handles the two-part handshake with the keyless control module (KCM)
 * to disable the immobilizer system when factory PCM is removed.
 *
 * CAN ID 0x047 (71 decimal): KCM → ECU (request)
 * CAN ID 0x041 (65 decimal): ECU → KCM (response)
 *
 * @author Ported from legacy RX8_CANBUS.ino
 * @date 2025-11-16
 */

#pragma once

#include <stdint.h>

namespace Immobilizer {

/**
 * Initialize immobilizer module
 */
void init();

/**
 * Process immobilizer handshake
 * Call this when CAN message 0x047 is received
 * @param data CAN message data (8 bytes)
 */
void processRequest(const uint8_t* data);

/**
 * Update immobilizer status
 * Call this in main loop
 */
void update();

/**
 * Check if immobilizer is unlocked
 * @return true if vehicle is unlocked and ready to drive
 */
bool isUnlocked();

/**
 * Get number of successful handshakes
 * @return Count of completed handshakes
 */
uint8_t getHandshakeCount();

} // namespace Immobilizer
