/*
 * Mazda RX8 Vehicle Profile for FOME
 *
 * Handles all RX8-specific body electronics:
 * - Dashboard CAN messages (gauges, warning lights)
 * - ABS/DSC communication
 * - Power steering enable
 * - Immobilizer bypass
 *
 * Based on MazdaRX8Arduino project by David Blackhurst
 */

#include "vehicle_profile.h"
#include <string.h>

class RX8Profile : public VehicleProfile {
public:
    void init() override {
        // Initialize default values
        m_coolantTemp = 145;  // Normal operating temp for RX8
        m_odometerByte = 0;
    }

    const char* getName() override {
        return "Mazda RX8 (S1)";
    }

    VehicleType getType() override {
        return VEHICLE_MAZDA_RX8;
    }

    void sendDashboardMessages() override {
        // Send all required CAN messages every 100ms

        // 0x201 - PCM Status (RPM, Speed, Throttle)
        send0x201();

        // 0x420 - MIL/Warning Lights/Temperature
        send0x420();

        // 0x620 - ABS System Data
        send0x620();

        // 0x630 - ABS Configuration
        send0x630();

        // 0x650 - ABS Supplement
        send0x650();

        // Additional PCM status messages
        send0x203();
        send0x215();
        send0x231();
        send0x240();
    }

    void handleCanRx(uint32_t canId, uint8_t* data, uint8_t length) override {
        switch (canId) {
            case 0x047:  // Immobilizer request
                handleImmobilizer(data);
                break;

            case 0x4B1:  // Wheel speed data
                handleWheelSpeed(data);
                break;
        }
    }

    void updateABSDSC() override {
        // Optional: Send 0x212 for DSC control
        uint8_t msg[7] = {0};

        // Byte 5: DSC/ABS status bits
        if (m_dscOff) msg[5] |= 0x01;
        if (m_absMIL) msg[5] |= 0x40;
        if (m_brakeFailMIL) msg[3] |= 0x40;
        if (m_etcActiveBL) msg[4] |= 0x20;
        if (m_etcDisabled) msg[5] |= 0x10;

        canTransmit(0x212, msg, 7);
    }

protected:
    // CAN transmit - to be implemented by platform layer
    void canTransmit(uint32_t id, uint8_t* data, uint8_t length) override;

private:
    // DSC/ABS state
    bool m_dscOff = false;
    bool m_absMIL = false;
    bool m_brakeFailMIL = false;
    bool m_etcActiveBL = false;
    bool m_etcDisabled = false;

    // Wheel speeds
    int m_wheelFL = 0;
    int m_wheelFR = 0;
    int m_wheelRL = 0;
    int m_wheelRR = 0;

    // 0x201 - PCM Status (most critical message)
    void send0x201() {
        uint8_t msg[8];

        // Bytes 0-1: Engine RPM (RPM * 3.85)
        int tempRPM = m_rpm * 3.85;
        msg[0] = (tempRPM >> 8) & 0xFF;
        msg[1] = tempRPM & 0xFF;

        // Bytes 2-3: Fixed 0xFF
        msg[2] = 0xFF;
        msg[3] = 0xFF;

        // Bytes 4-5: Vehicle Speed ((mph * 100) + 10000)
        int tempSpeed = (m_speed * 100) + 10000;
        msg[4] = (tempSpeed >> 8) & 0xFF;
        msg[5] = tempSpeed & 0xFF;

        // Byte 6: Throttle Pedal (0.5% increments, max 200)
        msg[6] = (m_tps * 2);
        if (msg[6] > 200) msg[6] = 200;

        // Byte 7: Fixed 0xFF
        msg[7] = 0xFF;

        canTransmit(0x201, msg, 8);
    }

    // 0x420 - MIL/Warning Lights
    void send0x420() {
        uint8_t msg[7] = {0};

        // Byte 0: Engine temperature
        msg[0] = m_coolantTemp;

        // Byte 1: Odometer increment
        msg[1] = m_odometerByte++;

        // Byte 4: Oil pressure status (1 = OK)
        msg[4] = (m_oilPressure > 100.0f) ? 0x01 : 0x00;

        // Byte 5: Check engine light
        if (m_warnings.checkEngine) {
            msg[5] |= 0x40;  // Bit 6 = MIL
        }

        // Byte 6: Other warning lights
        if (m_warnings.coolantTemp) msg[6] |= 0x02;    // Bit 1 = Low water
        if (m_warnings.batteryCharge) msg[6] |= 0x40;  // Bit 6 = Battery
        if (m_warnings.oilPressure) msg[6] |= 0x80;    // Bit 7 = Oil

        canTransmit(0x420, msg, 7);
    }

    // 0x620 - ABS System Data
    void send0x620() {
        // Critical for ABS light
        // Byte 6 varies by car (2, 3, or 4) - may need adjustment
        uint8_t msg[7] = {0, 0, 0, 0, 16, 0, 4};
        canTransmit(0x620, msg, 7);
    }

    // 0x630 - ABS Configuration
    void send0x630() {
        // Byte 0: AT/MT indicator
        // Bytes 6-7: Wheel size parameters
        uint8_t msg[8] = {8, 0, 0, 0, 0, 0, 106, 106};
        canTransmit(0x630, msg, 8);
    }

    // 0x650 - ABS Supplement
    void send0x650() {
        uint8_t msg[1] = {0};
        canTransmit(0x650, msg, 1);
    }

    // 0x203 - Traction Control Data
    void send0x203() {
        uint8_t msg[7] = {19, 0, 0, 0, 0, 0, 0};
        canTransmit(0x203, msg, 7);
    }

    // 0x215 - PCM Supplement 1
    void send0x215() {
        uint8_t msg[8] = {2, 0, 0, 0, 0, 0, 0, 0};
        canTransmit(0x215, msg, 8);
    }

    // 0x231 - PCM Supplement 2
    void send0x231() {
        uint8_t msg[5] = {15, 0, 82, 0, 0};
        canTransmit(0x231, msg, 5);
    }

    // 0x240 - PCM Supplement 3
    void send0x240() {
        uint8_t msg[8] = {4, 0, 40, 0, 2, 55, 6, 129};
        canTransmit(0x240, msg, 8);
    }

    // Handle immobilizer handshake
    void handleImmobilizer(uint8_t* data) {
        // Two-part handshake on CAN ID 0x047
        if (data[1] == 127 && data[2] == 2) {
            uint8_t response[8] = {7, 12, 48, 242, 23, 0, 0, 0};
            canTransmit(0x041, response, 8);
        }
        if (data[1] == 92 && data[2] == 244) {
            uint8_t response[8] = {129, 127, 0, 0, 0, 0, 0, 0};
            canTransmit(0x041, response, 8);
        }
    }

    // Handle wheel speed data
    void handleWheelSpeed(uint8_t* data) {
        // Decode wheel speeds from 0x4B1
        m_wheelFL = (data[0] * 256) + data[1] - 10000;
        m_wheelFR = (data[2] * 256) + data[3] - 10000;
        m_wheelRL = (data[4] * 256) + data[5] - 10000;
        m_wheelRR = (data[6] * 256) + data[7] - 10000;

        // Calculate vehicle speed from front wheels
        // Check for wheel speed mismatch
        if (abs(m_wheelFL - m_wheelFR) > 500) {
            // More than 5 kph difference - warn
            m_warnings.checkEngine = true;
            m_speed = 0;
        } else {
            m_speed = ((m_wheelFL + m_wheelFR) / 2) / 100;
        }
    }
};

// Platform-specific CAN transmit implementation
// This needs to be linked with FOME's CAN driver
extern "C" void fomeCan Transmit(uint32_t id, uint8_t* data, uint8_t length);

void RX8Profile::canTransmit(uint32_t id, uint8_t* data, uint8_t length) {
    fomeCanTransmit(id, data, length);
}

// Factory function
VehicleProfile* createVehicleProfile(VehicleType type) {
    switch (type) {
        case VEHICLE_MAZDA_RX8:
            return new RX8Profile();
        // Add other profiles here
        default:
            return nullptr;
    }
}
