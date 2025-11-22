/*
 * Generic Vehicle Profile for FOME
 *
 * Base implementation that sends minimal CAN messages.
 * Useful as a starting point for new vehicle support.
 */

#include "vehicle_profile.h"
#include <string.h>

class GenericProfile : public VehicleProfile {
public:
    void init() override {
        m_coolantTemp = 90;  // Default operating temp
    }

    const char* getName() override {
        return "Generic Vehicle";
    }

    VehicleType getType() override {
        return VEHICLE_GENERIC;
    }

    void sendDashboardMessages() override {
        // Send basic OBD-II style messages
        sendEngineData();
        sendVehicleData();
    }

    void handleCanRx(uint32_t canId, uint8_t* data, uint8_t length) override {
        // Generic handler - override for specific vehicles
        (void)canId;
        (void)data;
        (void)length;
    }

    void updateABSDSC() override {
        // No ABS/DSC control in generic profile
    }

protected:
    void canTransmit(uint32_t id, uint8_t* data, uint8_t length) override;

private:
    void sendEngineData() {
        // Standard OBD-II style engine data broadcast
        // CAN ID 0x7E8 (ECU response to tester)
        uint8_t msg[8] = {0};

        // SAE J1979 format
        msg[0] = 0x06;  // Length
        msg[1] = 0x41;  // Mode 01 response
        msg[2] = 0x00;  // PID 00 - Supported PIDs

        // Indicate we support PIDs 01, 04, 05, 0C, 0D
        msg[3] = 0x98;  // PIDs 1-8
        msg[4] = 0x18;  // PIDs 9-16
        msg[5] = 0x80;  // PIDs 17-24
        msg[6] = 0x00;  // PIDs 25-32

        canTransmit(0x7E8, msg, 8);
    }

    void sendVehicleData() {
        // Broadcast current values as OBD-II responses

        // RPM (PID 0x0C)
        {
            uint8_t msg[8] = {0};
            msg[0] = 0x04;
            msg[1] = 0x41;
            msg[2] = 0x0C;
            // RPM = (A*256 + B) / 4
            uint16_t rpmEncoded = m_rpm * 4;
            msg[3] = (rpmEncoded >> 8) & 0xFF;
            msg[4] = rpmEncoded & 0xFF;
            canTransmit(0x7E8, msg, 8);
        }

        // Coolant temp (PID 0x05)
        {
            uint8_t msg[8] = {0};
            msg[0] = 0x03;
            msg[1] = 0x41;
            msg[2] = 0x05;
            // Temp = A - 40
            msg[3] = m_coolantTemp + 40;
            canTransmit(0x7E8, msg, 8);
        }

        // Vehicle speed (PID 0x0D)
        {
            uint8_t msg[8] = {0};
            msg[0] = 0x03;
            msg[1] = 0x41;
            msg[2] = 0x0D;
            msg[3] = m_speed;  // Direct km/h
            canTransmit(0x7E8, msg, 8);
        }

        // Throttle position (PID 0x11)
        {
            uint8_t msg[8] = {0};
            msg[0] = 0x03;
            msg[1] = 0x41;
            msg[2] = 0x11;
            // Throttle = A * 100 / 255
            msg[3] = (m_throttle * 255) / 100;
            canTransmit(0x7E8, msg, 8);
        }
    }
};

// Factory function implementation
VehicleProfile* createVehicleProfile(VehicleType type) {
    switch (type) {
        case VEHICLE_GENERIC:
            return new GenericProfile();

        case VEHICLE_MAZDA_RX8:
            // Return RX8Profile from rx8_profile.cpp
            extern VehicleProfile* createRX8Profile();
            return createRX8Profile();

        default:
            return new GenericProfile();
    }
}

// Platform-specific canTransmit stub - implement in platform layer
void GenericProfile::canTransmit(uint32_t id, uint8_t* data, uint8_t length) {
    // Platform implementation will override this
    (void)id;
    (void)data;
    (void)length;
}
