/*
 * FOME Integration Example
 *
 * Shows how to integrate vehicle profiles and XCP with FOME ECU.
 * This is a simplified example demonstrating the architecture.
 */

#include "../vehicle_profiles/vehicle_profile.h"
#include "../xcp/xcp_protocol.h"
#include "../platform/platform_abstraction.h"

// Configuration
#define VEHICLE_TYPE VEHICLE_MAZDA_RX8
#define BODY_CAN_INTERVAL_MS 100
#define XCP_CAN_RX_ID 0x554
#define XCP_CAN_TX_ID 0x555

// Global instances
VehicleProfile* vehicleProfile = nullptr;
extern XcpSlave xcpSlave;

// Simulated sensor data (in real FOME, read from actual sensors)
struct SensorData {
    int rpm;
    int speed;
    int coolantTemp;
    int throttle;
    float oilPressure;
    float batteryVoltage;
} sensors;

// Timing
uint32_t lastBodyCanTime = 0;

// -----------------------------------------
// Initialization
// -----------------------------------------

void fomeIntegrationInit() {
    // Initialize platform
    platform_serial_init(115200);
    platform_serial_println("FOME Integration Starting...");

    // Initialize CAN bus at 500kbps
    if (!platform_can_init(500000)) {
        platform_serial_println("ERROR: CAN init failed!");
        return;
    }
    platform_serial_println("CAN bus initialized");

    // Create vehicle profile
    vehicleProfile = createVehicleProfile(VEHICLE_TYPE);
    if (vehicleProfile) {
        vehicleProfile->init();
        platform_serial_print("Vehicle profile: ");
        platform_serial_println(vehicleProfile->getName());
    } else {
        platform_serial_println("ERROR: Failed to create vehicle profile!");
    }

    // Initialize XCP slave
    xcpSlave.init(XCP_CAN_RX_ID, XCP_CAN_TX_ID);
    platform_serial_println("XCP slave initialized");

    // Initialize watchdog (2 second timeout)
    platform_watchdog_init(2000);

    platform_serial_println("FOME Integration Ready");
}

// -----------------------------------------
// Main Loop Tasks
// -----------------------------------------

void processCanRx() {
    CanMessage msg;

    while (platform_can_receive(&msg)) {
        // Handle XCP commands
        if (msg.id == XCP_CAN_RX_ID) {
            xcpSlave.processCommand(msg.data, msg.length);
        }
        // Handle vehicle-specific messages
        else if (vehicleProfile) {
            vehicleProfile->handleCanRx(msg.id, msg.data, msg.length);
        }
    }
}

void updateSensors() {
    // In real FOME, read from actual sensors
    // This is placeholder code for demonstration

    // Example: Read throttle from ADC
    uint16_t throttleRaw = platform_adc_read(0);  // A0
    sensors.throttle = (throttleRaw * 100) / 4095;

    // Example: Simulated values
    sensors.rpm = 3000;
    sensors.speed = 60;
    sensors.coolantTemp = 85;
    sensors.oilPressure = 400.0f;
    sensors.batteryVoltage = 14.2f;
}

void sendBodyCanMessages() {
    if (!vehicleProfile) return;

    uint32_t now = platform_millis();
    if (now - lastBodyCanTime < BODY_CAN_INTERVAL_MS) {
        return;
    }
    lastBodyCanTime = now;

    // Update vehicle profile with current sensor data
    vehicleProfile->setRPM(sensors.rpm);
    vehicleProfile->setVehicleSpeed(sensors.speed);
    vehicleProfile->setCoolantTemp(sensors.coolantTemp);
    vehicleProfile->setThrottle(sensors.throttle);
    vehicleProfile->setOilPressure(sensors.oilPressure);
    vehicleProfile->setBatteryVoltage(sensors.batteryVoltage);

    // Set warning lights based on conditions
    WarningLights warnings = {0};
    warnings.checkEngine = false;
    warnings.coolantTemp = (sensors.coolantTemp > 110);
    warnings.oilPressure = (sensors.oilPressure < 100);
    warnings.batteryCharge = (sensors.batteryVoltage < 11.5f);
    vehicleProfile->setWarnings(warnings);

    // Send all body CAN messages
    vehicleProfile->sendDashboardMessages();
}

void sendXcpDaqData() {
    // Send DAQ data on 10ms event channel
    static uint32_t lastDaqTime = 0;
    uint32_t now = platform_millis();

    if (now - lastDaqTime >= 10) {
        lastDaqTime = now;
        xcpSlave.sendDaqData(0);  // Event channel 0
    }
}

// -----------------------------------------
// Main Entry Points
// -----------------------------------------

// Call this from FOME's setup()
void setup() {
    fomeIntegrationInit();
}

// Call this from FOME's main loop
void loop() {
    // Feed watchdog
    platform_watchdog_feed();

    // Process incoming CAN messages
    processCanRx();

    // Update sensor readings
    updateSensors();

    // Send body electronics CAN messages
    sendBodyCanMessages();

    // Send XCP DAQ data
    sendXcpDaqData();
}

// -----------------------------------------
// Arduino Main (for standalone testing)
// -----------------------------------------

#ifdef PLATFORM_ARDUINO
int main() {
    init();  // Arduino core init

    setup();

    while (1) {
        loop();
    }

    return 0;
}
#endif

// -----------------------------------------
// FOME Integration Callbacks
// -----------------------------------------

// Called by FOME when engine data updates
extern "C" void onEngineUpdate(int rpm, int coolantTemp, int tps, int map) {
    sensors.rpm = rpm;
    sensors.coolantTemp = coolantTemp;
    sensors.throttle = tps;
    // Update other values as needed
}

// Called by FOME on slow callback (10Hz)
extern "C" void onSlowCallback() {
    sendBodyCanMessages();
}

// Called by FOME on fast callback (1kHz)
extern "C" void onFastCallback() {
    processCanRx();
    sendXcpDaqData();
}
