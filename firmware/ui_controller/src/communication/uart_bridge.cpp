/**
 * @file uart_bridge.cpp
 * @brief UART communication bridge implementation
 *
 * @author Created for Phase 5 unified architecture
 * @date 2025-11-16
 */

#include "uart_bridge.h"
#include "../../config/features.h"
#include <Arduino.h>

#if ENABLE_UART_BRIDGE

namespace UARTBridge {

// UART instance
static HardwareSerial* uart = nullptr;

// Vehicle state
static VehicleState vehicle_state = {0};
static bool state_valid = false;
static uint32_t last_packet_time = 0;
static uint32_t packet_count = 0;
static uint32_t error_count = 0;
static uint32_t timeout_ms = 500;

// Packet buffer
static uint8_t packet_buffer[sizeof(VehicleState)];
static uint16_t buffer_index = 0;

// Forward declarations
static bool validatePacket(const VehicleState* state);
static uint8_t calculateChecksum(const uint8_t* data, uint16_t len);

// ============================================================================
// INITIALIZATION
// ============================================================================

void init(uint8_t uart_port, uint32_t baud_rate, int rx_pin, int tx_pin) {
    // Initialize UART
    if (uart_port == 1) {
        uart = &Serial1;
    } else if (uart_port == 2) {
        uart = &Serial2;
    } else {
        Serial.println("[UART BRIDGE] Invalid UART port!");
        return;
    }

    uart->begin(baud_rate, SERIAL_8N1, rx_pin, tx_pin);

    // Initialize state
    vehicle_state.header[0] = 'V';
    vehicle_state.header[1] = 'S';
    state_valid = false;
    last_packet_time = 0;
    packet_count = 0;
    error_count = 0;
    buffer_index = 0;

    Serial.printf("[UART BRIDGE] Initialized on UART%d @ %d baud\n", uart_port, baud_rate);
    Serial.printf("[UART BRIDGE] RX: %d, TX: %d\n", rx_pin, tx_pin);
}

// ============================================================================
// UPDATE (CALLED FROM FREERTOS TASK)
// ============================================================================

void update() {
    if (!uart) return;

    // Read available bytes
    while (uart->available()) {
        uint8_t byte = uart->read();

        // Check for packet start (header)
        if (buffer_index == 0) {
            if (byte == 'V') {
                packet_buffer[buffer_index++] = byte;
            }
        } else if (buffer_index == 1) {
            if (byte == 'S') {
                packet_buffer[buffer_index++] = byte;
            } else {
                // Invalid header, reset
                buffer_index = 0;
            }
        } else {
            // Accumulate packet data
            packet_buffer[buffer_index++] = byte;

            // Check if full packet received
            if (buffer_index >= sizeof(VehicleState)) {
                // Validate packet
                VehicleState* received_state = (VehicleState*)packet_buffer;

                if (validatePacket(received_state)) {
                    // Valid packet - update vehicle state
                    memcpy(&vehicle_state, received_state, sizeof(VehicleState));
                    state_valid = true;
                    last_packet_time = millis();
                    packet_count++;

                    #if ENABLE_SERIAL_DEBUG >= 2  // Verbose logging
                        Serial.printf("[UART BRIDGE] Packet received: RPM=%d, Speed=%.1f km/h\n",
                                     vehicle_state.rpm, vehicle_state.speed_kmh / 10.0f);
                    #endif
                } else {
                    // Invalid packet
                    error_count++;

                    #if ENABLE_SERIAL_DEBUG
                        Serial.println("[UART BRIDGE] Packet checksum error!");
                    #endif
                }

                // Reset buffer for next packet
                buffer_index = 0;
            }
        }
    }

    // Check for timeout
    if (state_valid && (millis() - last_packet_time > timeout_ms)) {
        state_valid = false;

        #if ENABLE_SERIAL_DEBUG
            Serial.println("[UART BRIDGE] WARNING: Vehicle state timeout!");
        #endif
    }
}

// ============================================================================
// GETTERS
// ============================================================================

const VehicleState* getVehicleState() {
    return &vehicle_state;
}

bool isValid() {
    return state_valid;
}

uint32_t getTimeSinceLastPacket() {
    return millis() - last_packet_time;
}

uint32_t getPacketCount() {
    return packet_count;
}

uint32_t getErrorCount() {
    return error_count;
}

void setTimeout(uint32_t new_timeout_ms) {
    timeout_ms = new_timeout_ms;
}

// ============================================================================
// COMMAND SENDING (OPTIONAL)
// ============================================================================

void sendCommand(uint8_t command, const uint8_t* data, uint8_t len) {
    if (!uart) return;

    // Send command packet
    uart->write('C');  // Command header
    uart->write('M');
    uart->write(command);
    uart->write(len);

    if (data && len > 0) {
        uart->write(data, len);
    }

    // Send checksum
    uint8_t checksum = command ^ len;
    for (uint8_t i = 0; i < len; i++) {
        checksum ^= data[i];
    }
    uart->write(checksum);

    #if ENABLE_SERIAL_DEBUG >= 2
        Serial.printf("[UART BRIDGE] Command sent: 0x%02X (len=%d)\n", command, len);
    #endif
}

// ============================================================================
// INTERNAL FUNCTIONS
// ============================================================================

static bool validatePacket(const VehicleState* state) {
    // Check header
    if (state->header[0] != 'V' || state->header[1] != 'S') {
        return false;
    }

    // Calculate checksum (XOR of all bytes except checksum itself)
    uint8_t calculated_checksum = calculateChecksum((const uint8_t*)state,
                                                     sizeof(VehicleState) - 1);

    // Verify checksum
    return (calculated_checksum == state->checksum);
}

static uint8_t calculateChecksum(const uint8_t* data, uint16_t len) {
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < len; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

} // namespace UARTBridge

#endif // ENABLE_UART_BRIDGE
