/*
 * Vehicle Profile System for FOME
 * Base class for vehicle-specific body electronics
 *
 * This enables FOME to control dashboard, ABS, power steering,
 * and immobilizer for different vehicle platforms.
 */

#ifndef VEHICLE_PROFILE_H
#define VEHICLE_PROFILE_H

#include <stdint.h>

// Vehicle type enumeration
enum VehicleType {
    VEHICLE_GENERIC = 0,
    VEHICLE_MAZDA_RX8 = 1,
    VEHICLE_MAZDA_MIATA_NC = 2,
    VEHICLE_BMW_E46 = 3,
    VEHICLE_NISSAN_350Z = 4,
    // Add more as needed
};

// Warning light flags
struct WarningLights {
    bool checkEngine;
    bool oilPressure;
    bool batteryCharge;
    bool coolantTemp;
    bool absWarning;
    bool dscOff;
    bool brakeFailure;
};

// Base class for vehicle profiles
class VehicleProfile {
public:
    virtual ~VehicleProfile() {}

    // Initialization
    virtual void init() = 0;

    // Main update functions (call from FOME main loop)
    virtual void sendDashboardMessages() = 0;
    virtual void handleCanRx(uint32_t canId, uint8_t* data, uint8_t length) = 0;

    // Optional overrides
    virtual void updateABSDSC() {}
    virtual void updatePowerSteering() {}

    // Set engine data from FOME sensors
    void setRPM(int rpm) { m_rpm = rpm; }
    void setVehicleSpeed(int speed) { m_speed = speed; }
    void setCoolantTemp(int temp) { m_coolantTemp = temp; }
    void setThrottlePosition(int tps) { m_tps = tps; }
    void setOilPressure(float kpa) { m_oilPressure = kpa; }
    void setBatteryVoltage(float volts) { m_batteryVoltage = volts; }
    void setWarnings(WarningLights warnings) { m_warnings = warnings; }

    // Get profile info
    virtual const char* getName() = 0;
    virtual VehicleType getType() = 0;

protected:
    // Engine data
    int m_rpm = 0;
    int m_speed = 0;
    int m_coolantTemp = 80;
    int m_tps = 0;
    float m_oilPressure = 300.0f;
    float m_batteryVoltage = 12.6f;

    // Warning lights
    WarningLights m_warnings = {};

    // Odometer
    uint8_t m_odometerByte = 0;

    // CAN transmit function (implement in platform layer)
    virtual void canTransmit(uint32_t id, uint8_t* data, uint8_t length) = 0;
};

// Factory function to create vehicle profile
VehicleProfile* createVehicleProfile(VehicleType type);

#endif // VEHICLE_PROFILE_H
