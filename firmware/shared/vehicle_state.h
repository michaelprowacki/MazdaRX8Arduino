/**
 * @file vehicle_state.h
 * @brief Shared vehicle state structure between Tier 1 (automotive ECU) and Tier 2 (ESP32)
 *
 * This structure is transmitted over UART or CAN from the automotive ECU to ESP32.
 * Keep it small and efficient - transmitted 20-50 times per second.
 *
 * @author Unified Architecture (Phase 5)
 * @date 2025-11-16
 */

#pragma once

#include <stdint.h>

// ============================================================================
// VEHICLE STATE STRUCTURE
// ============================================================================
/**
 * Complete vehicle state transmitted from automotive ECU to UI controller
 *
 * Total size: 32 bytes (fits in single CAN frame with fragmentation or UART packet)
 */
typedef struct __attribute__((packed)) {
    // ========== Engine/Motor (6 bytes) ==========
    uint16_t rpm;                // Engine RPM (0-65535)
    uint16_t speed_kmh;          // Vehicle speed (km/h * 10, e.g., 1234 = 123.4 km/h)
    uint8_t  throttle_percent;   // Throttle position (0-100%)
    uint8_t  gear;               // Current gear (0=N, 1-6=gears, 0xFF=unknown)

    // ========== Temperatures (6 bytes) ==========
    int16_t  coolant_temp;       // Coolant temperature (°C * 10, e.g., 850 = 85.0°C)
    int16_t  oil_temp;           // Oil temperature (°C * 10)
    int16_t  intake_temp;        // Intake air temperature (°C * 10)

    // ========== Electrical (4 bytes) ==========
    uint16_t battery_voltage;    // Battery voltage (V * 100, e.g., 1380 = 13.80V)
    uint16_t alternator_load;    // Alternator load (%)

    // ========== Wheel Speeds (8 bytes) ==========
    uint16_t wheel_fl;           // Front left wheel speed (km/h * 100)
    uint16_t wheel_fr;           // Front right wheel speed
    uint16_t wheel_rl;           // Rear left wheel speed
    uint16_t wheel_rr;           // Rear right wheel speed

    // ========== Warning Flags (2 bytes) ==========
    union {
        uint16_t all_flags;      // All flags as single word
        struct {
            uint16_t check_engine    : 1;  // Check engine MIL
            uint16_t abs_warning     : 1;  // ABS warning light
            uint16_t oil_pressure    : 1;  // Oil pressure warning
            uint16_t coolant_level   : 1;  // Low coolant warning
            uint16_t battery_charge  : 1;  // Battery charge warning
            uint16_t brake_failure   : 1;  // Brake system failure
            uint16_t immobilizer     : 1;  // Immobilizer active
            uint16_t airbag          : 1;  // Airbag warning
            uint16_t tcs_active      : 1;  // Traction control active
            uint16_t abs_active      : 1;  // ABS active
            uint16_t handbrake       : 1;  // Handbrake engaged
            uint16_t seatbelt        : 1;  // Seatbelt not fastened
            uint16_t door_open       : 1;  // Door open
            uint16_t trunk_open      : 1;  // Trunk open
            uint16_t reserved1       : 1;  // Reserved
            uint16_t reserved2       : 1;  // Reserved
        } flags;
    } warnings;

    // ========== Fuel/Energy (2 bytes) ==========
    uint8_t  fuel_level_percent; // Fuel level (0-100%) or battery SOC for EV
    uint8_  reserved_fuel;       // Reserved

    // ========== Odometer (4 bytes) ==========
    uint32_t odometer_km;        // Odometer reading (kilometers)

    // Total: 32 bytes

} VehicleState;

// ============================================================================
// UART COMMUNICATION PROTOCOL
// ============================================================================
/**
 * UART packet format for transmitting vehicle state
 *
 * Format: [START_BYTE] [LENGTH] [VEHICLE_STATE] [CHECKSUM]
 *
 * START_BYTE: 0xAA
 * LENGTH: sizeof(VehicleState) = 32
 * CHECKSUM: XOR of all data bytes
 */
#define UART_START_BYTE     0xAA
#define UART_PACKET_SIZE    (1 + 1 + sizeof(VehicleState) + 1)  // Start + Len + Data + Checksum

/**
 * Calculate checksum for UART packet
 */
inline uint8_t calculateChecksum(const uint8_t* data, size_t len) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < len; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

/**
 * Pack vehicle state into UART packet
 */
inline void packVehicleState(const VehicleState* state, uint8_t* packet) {
    packet[0] = UART_START_BYTE;
    packet[1] = sizeof(VehicleState);

    // Copy state structure
    memcpy(&packet[2], state, sizeof(VehicleState));

    // Calculate and append checksum
    packet[2 + sizeof(VehicleState)] = calculateChecksum((const uint8_t*)state, sizeof(VehicleState));
}

/**
 * Unpack UART packet into vehicle state
 * Returns true if packet is valid
 */
inline bool unpackVehicleState(const uint8_t* packet, VehicleState* state) {
    // Validate start byte
    if (packet[0] != UART_START_BYTE) {
        return false;
    }

    // Validate length
    if (packet[1] != sizeof(VehicleState)) {
        return false;
    }

    // Validate checksum
    uint8_t calculated_checksum = calculateChecksum(&packet[2], sizeof(VehicleState));
    uint8_t received_checksum = packet[2 + sizeof(VehicleState)];
    if (calculated_checksum != received_checksum) {
        return false;
    }

    // Copy data
    memcpy(state, &packet[2], sizeof(VehicleState));
    return true;
}

// ============================================================================
// UI COMMANDS (ESP32 → Automotive ECU)
// ============================================================================
/**
 * Commands sent from UI controller back to automotive ECU
 * (Optional - for future features like remote start, diagnostics, etc.)
 */
typedef struct __attribute__((packed)) {
    uint8_t wiper_mode;          // 0=off, 1=auto, 2=low, 3=high
    uint8_t dimmer_level;        // Dashboard dimmer (0-100%)
    uint8_t reserved[6];         // Reserved for future use
} UICommands;

// Total: 8 bytes
