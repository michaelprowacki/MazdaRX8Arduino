/**
 * @file uart_bridge.h
 * @brief UART communication bridge between automotive ECU and ESP32
 *
 * Receives vehicle state data from automotive ECU via UART:
 * - RPM, speed, throttle
 * - Temperatures, pressures
 * - Warning flags
 * - Wheel speeds
 *
 * Protocol: Binary packet with checksum
 * Baud rate: 115200 (configurable)
 *
 * @author Created for Phase 5 unified architecture
 * @date 2025-11-16
 */

#ifndef UART_BRIDGE_H
#define UART_BRIDGE_H

#include <stdint.h>

namespace UARTBridge {

/**
 * @brief Vehicle state structure (must match automotive ECU definition)
 */
struct VehicleState {
    // Header
    uint8_t  header[2];          // "VS" (0x56 0x53)

    // Engine/Motor
    uint16_t rpm;
    uint16_t speed_kmh;          // km/h * 10
    uint8_t  throttle_percent;

    // Temperatures (°C * 10)
    int16_t  coolant_temp;
    int16_t  oil_temp;
    int16_t  intake_temp;

    // Pressures
    uint16_t oil_pressure;       // PSI
    int16_t  boost_pressure;     // PSI (can be negative for vacuum)

    // Electrical
    uint16_t battery_voltage;    // V * 100

    // Wheel speeds (km/h * 100)
    uint16_t wheel_fl;
    uint16_t wheel_fr;
    uint16_t wheel_rl;
    uint16_t wheel_rr;

    // Warning flags (bitfield)
    struct {
        uint8_t check_engine    : 1;
        uint8_t abs_warning     : 1;
        uint8_t oil_pressure    : 1;
        uint8_t coolant_level   : 1;
        uint8_t battery_charge  : 1;
        uint8_t brake_failure   : 1;
        uint8_t immobilizer     : 1;
        uint8_t reserved        : 1;
    } warnings;

    // Fuel (if ICE)
    uint8_t  fuel_level;         // 0-100%

    // Odometer
    uint32_t odometer;           // km * 10

    // Checksum
    uint8_t  checksum;           // XOR of all bytes

} __attribute__((packed));

/**
 * @brief Initialize UART bridge
 *
 * @param uart_port UART port number (1, 2, etc.)
 * @param baud_rate Baud rate (default: 115200)
 * @param rx_pin RX pin (default: 16)
 * @param tx_pin TX pin (default: 17)
 */
void init(uint8_t uart_port = 2,
          uint32_t baud_rate = 115200,
          int rx_pin = 16,
          int tx_pin = 17);

/**
 * @brief Update UART bridge (call from FreeRTOS task)
 *
 * Reads incoming UART data and updates vehicle state.
 */
void update();

/**
 * @brief Get current vehicle state
 * @return Pointer to current vehicle state
 */
const VehicleState* getVehicleState();

/**
 * @brief Check if vehicle state is valid
 *
 * Returns false if:
 * - No data received recently (timeout)
 * - Checksum error
 * - Invalid header
 *
 * @return true if vehicle state is valid
 */
bool isValid();

/**
 * @brief Get time since last valid packet
 * @return Milliseconds since last valid packet
 */
uint32_t getTimeSinceLastPacket();

/**
 * @brief Get packet receive count
 * @return Total number of valid packets received
 */
uint32_t getPacketCount();

/**
 * @brief Get error count
 * @return Total number of packet errors (checksum, timeout)
 */
uint32_t getErrorCount();

/**
 * @brief Send command to automotive ECU (optional)
 *
 * For future expansion: ESP32 can send commands to ECU
 * (e.g., request diagnostic data, adjust settings)
 *
 * @param command Command byte
 * @param data Optional data payload
 * @param len Data length
 */
void sendCommand(uint8_t command, const uint8_t* data = nullptr, uint8_t len = 0);

/**
 * @brief Set timeout for vehicle state validity
 *
 * If no valid packet received within timeout, state is marked invalid.
 *
 * @param timeout_ms Timeout in milliseconds (default: 500ms)
 */
void setTimeout(uint32_t timeout_ms);

} // namespace UARTBridge

#endif // UART_BRIDGE_H
