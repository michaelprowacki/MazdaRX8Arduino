/*
 * RX8_CAN_Messages.h
 *
 * Comprehensive CAN message decoder library for Mazda RX8
 * Supports all documented CAN messages from Series 1 RX8 (2004-2008)
 *
 * Usage:
 *   #include "RX8_CAN_Messages.h"
 *   RX8_CAN_Decoder decoder;
 *   decoder.decode0x201(canBuffer);
 *   int rpm = decoder.pcmStatus.engineRPM;
 *
 * Based on comprehensive reverse engineering from:
 * - Documentation/CAN_PID_Reference.md
 * - rnd-ash/rx8-reverse-engineering
 * - Community testing and validation
 *
 * Repository: https://github.com/michaelprowacki/MazdaRX8Arduino
 * License: MIT (see repository LICENSE)
 *
 * Last Updated: 2025-11-15
 */

#ifndef RX8_CAN_MESSAGES_H
#define RX8_CAN_MESSAGES_H

#include <Arduino.h>

// ============================================================================
// CAN MESSAGE IDS (Hexadecimal)
// ============================================================================

// PCM/ECU Transmitted Messages
#define CAN_ID_PCM_STATUS           0x201  // 513 decimal - RPM, Speed, Throttle
#define CAN_ID_PCM_SUPPLEMENT_1     0x203  // 515 decimal - Traction control
#define CAN_ID_DSC_ABS              0x212  // 530 decimal - DSC/ABS status
#define CAN_ID_PCM_SUPPLEMENT_2     0x215  // 533 decimal - PCM status supplement
#define CAN_ID_PCM_SUPPLEMENT_3     0x231  // 561 decimal - PCM status supplement
#define CAN_ID_PCM_SUPPLEMENT_4     0x240  // 576 decimal - PCM status supplement
#define CAN_ID_WARNING_LIGHTS       0x420  // 1056 decimal - MIL, temp, warnings
#define CAN_ID_ABS_DATA             0x620  // 1568 decimal - ABS system data
#define CAN_ID_ABS_CONFIG           0x630  // 1584 decimal - ABS config (AT/MT, wheel size)
#define CAN_ID_ABS_SUPPLEMENT       0x650  // 1616 decimal - ABS supplement
#define CAN_ID_IMMOBILIZER_RESPONSE 0x041  // 65 decimal - Immobilizer response

// Vehicle Transmitted Messages
#define CAN_ID_IMMOBILIZER_REQUEST  0x047  // 71 decimal - Immobilizer/keyless request
#define CAN_ID_ACCELEROMETER        0x075  // 117 decimal - 3-axis accelerometer
#define CAN_ID_STEERING_ANGLE       0x4BE  // 1214 decimal - Steering wheel angle
#define CAN_ID_WHEEL_SPEEDS_ABS     0x4B0  // 1200 decimal - Wheel speeds (for ABS/DSC)
#define CAN_ID_WHEEL_SPEEDS_DASH    0x4B1  // 1201 decimal - Wheel speeds (for dashboard)

// Additional Vehicle Messages
#define CAN_ID_BRAKE_STATUS         0x212  // Brake pedal and handbrake
#define CAN_ID_THROTTLE_PEDAL       0x201  // Included in PCM status

// ============================================================================
// DATA STRUCTURES
// ============================================================================

// 0x201 - PCM Status (RPM, Speed, Throttle)
struct PCM_Status {
    int engineRPM;           // Engine RPM (0-10000+)
    int vehicleSpeed;        // Vehicle speed in MPH (or KPH if configured)
    int throttlePosition;    // Throttle position percentage (0-100%)

    // Raw values
    uint16_t rawRPM;         // Raw encoded RPM value
    uint16_t rawSpeed;       // Raw encoded speed value
    uint8_t rawThrottle;     // Raw encoded throttle value
};

// 0x420 - Warning Lights and Engine Temperature
struct Warning_Lights {
    int coolantTemperature;  // Coolant temperature (145 = normal)
    bool checkEngineMIL;     // Check Engine / MIL light
    bool checkEngineBL;      // Check Engine backlight
    bool lowCoolantMIL;      // Low coolant warning
    bool batteryChargeMIL;   // Battery charge warning
    bool oilPressureMIL;     // Oil pressure warning light
    bool oilPressureOK;      // Oil pressure OK status (1 = OK)

    // Additional warnings
    bool catalystTempHigh;   // Catalyst over-temperature
    bool engineOverheat;     // Engine overheat warning

    // Raw values
    uint8_t odometerByte;    // Odometer-related byte
    uint8_t byte5;           // Raw byte 5 (MIL bits)
    uint8_t byte6;           // Raw byte 6 (warning bits)
};

// 0x4B0 / 0x4B1 - Wheel Speeds
struct Wheel_Speeds {
    int frontLeft;           // Front left wheel speed (kph * 100)
    int frontRight;          // Front right wheel speed (kph * 100)
    int rearLeft;            // Rear left wheel speed (kph * 100)
    int rearRight;           // Rear right wheel speed (kph * 100)

    // Calculated values
    int averageFront;        // Average of front wheels
    int averageRear;         // Average of rear wheels
    int averageAll;          // Average of all four wheels

    // Status
    bool wheelSpeedMismatch; // Front wheels differ by >5 kph
};

// 0x4BE - Steering Angle
struct Steering_Angle {
    float steeringAngle;     // Steering wheel angle in degrees (-780 to +780)
    int16_t rawAngle;        // Raw 16-bit signed value
    bool centerCalibrated;   // True if steering is centered
};

// 0x075 - Accelerometer Data
struct Accelerometer {
    float lateralG;          // Lateral G-force (left/right)
    float longitudinalG;     // Longitudinal G-force (forward/back)
    float verticalG;         // Vertical G-force (up/down)

    // Raw values
    int16_t rawLateral;
    int16_t rawLongitudinal;
    int16_t rawVertical;
};

// 0x620 - ABS System Data
struct ABS_Data {
    uint8_t absStatus;       // ABS system status
    uint8_t dscStatus;       // DSC system status
    bool absWarning;         // ABS warning light
    bool brakeFailWarning;   // Brake failure warning

    // Raw bytes
    uint8_t byte[7];         // Full 7-byte message
};

// 0x630 - ABS Configuration
struct ABS_Config {
    bool isAutomatic;        // True if automatic transmission
    uint8_t wheelSizeParam1; // Wheel size parameter 1 (typically 106)
    uint8_t wheelSizeParam2; // Wheel size parameter 2 (typically 106)
    uint8_t transmissionType; // Full transmission type byte

    // Raw bytes
    uint8_t byte[8];         // Full 8-byte message
};

// 0x212 - DSC/ABS Status
struct DSC_Status {
    bool dscOff;             // DSC system off
    bool absMIL;             // ABS warning light
    bool brakeFailMIL;       // Brake failure warning
    bool etcActiveBL;        // Electronic throttle control active
    bool etcDisabled;        // Electronic throttle control disabled

    // Raw bytes
    uint8_t byte[7];         // Full 7-byte message
};

// ============================================================================
// MAIN DECODER CLASS
// ============================================================================

class RX8_CAN_Decoder {
public:
    // Data structures (public for easy access)
    PCM_Status pcmStatus;
    Warning_Lights warningLights;
    Wheel_Speeds wheelSpeeds;
    Steering_Angle steeringAngle;
    Accelerometer accelerometer;
    ABS_Data absData;
    ABS_Config absConfig;
    DSC_Status dscStatus;

    // Constructor
    RX8_CAN_Decoder() {
        reset();
    }

    // Reset all values to defaults
    void reset() {
        memset(&pcmStatus, 0, sizeof(pcmStatus));
        memset(&warningLights, 0, sizeof(warningLights));
        memset(&wheelSpeeds, 0, sizeof(wheelSpeeds));
        memset(&steeringAngle, 0, sizeof(steeringAngle));
        memset(&accelerometer, 0, sizeof(accelerometer));
        memset(&absData, 0, sizeof(absData));
        memset(&absConfig, 0, sizeof(absConfig));
        memset(&dscStatus, 0, sizeof(dscStatus));
    }

    // ========================================================================
    // DECODING FUNCTIONS
    // ========================================================================

    // Decode 0x201 - PCM Status (RPM, Speed, Throttle)
    void decode0x201(uint8_t buf[]) {
        // Bytes 0-1: Engine RPM (RPM * 3.85)
        pcmStatus.rawRPM = (buf[0] << 8) | buf[1];
        pcmStatus.engineRPM = (int)(pcmStatus.rawRPM / 3.85);

        // Bytes 2-3: Fixed 0xFF 0xFF (no data)

        // Bytes 4-5: Vehicle Speed ((mph * 100) + 10000)
        pcmStatus.rawSpeed = (buf[4] << 8) | buf[5];
        pcmStatus.vehicleSpeed = (pcmStatus.rawSpeed - 10000) / 100;

        // Byte 6: Throttle Pedal (0.5% increments, 0-200 = 0-100%)
        pcmStatus.rawThrottle = buf[6];
        pcmStatus.throttlePosition = pcmStatus.rawThrottle / 2;

        // Byte 7: Fixed 0xFF (no data)
    }

    // Decode 0x420 - Warning Lights and Temperature
    void decode0x420(uint8_t buf[]) {
        // Byte 0: Engine/Coolant Temperature (145 = normal operating temp)
        warningLights.coolantTemperature = buf[0];

        // Byte 1: Odometer-related
        warningLights.odometerByte = buf[1];

        // Byte 2-3: Unknown

        // Byte 4: Oil pressure (1 = OK)
        warningLights.oilPressureOK = (buf[4] == 1);

        // Byte 5: MIL lights
        //   Bit 6 = Check Engine MIL
        //   Bit 7 = Check Engine Backlight
        warningLights.byte5 = buf[5];
        warningLights.checkEngineMIL = (buf[5] & 0x40) ? true : false;
        warningLights.checkEngineBL = (buf[5] & 0x80) ? true : false;

        // Byte 6: Additional warnings
        //   Bit 0 = Catalyst over-temp
        //   Bit 1 = Low coolant
        //   Bit 5 = Engine overheat
        //   Bit 6 = Battery charge warning
        //   Bit 7 = Oil pressure warning
        warningLights.byte6 = buf[6];
        warningLights.catalystTempHigh = (buf[6] & 0x01) ? true : false;
        warningLights.lowCoolantMIL = (buf[6] & 0x02) ? true : false;
        warningLights.engineOverheat = (buf[6] & 0x20) ? true : false;
        warningLights.batteryChargeMIL = (buf[6] & 0x40) ? true : false;
        warningLights.oilPressureMIL = (buf[6] & 0x80) ? true : false;
    }

    // Decode 0x4B0 or 0x4B1 - Wheel Speeds (both use same format)
    void decode0x4B0(uint8_t buf[]) { decodeWheelSpeeds(buf); }
    void decode0x4B1(uint8_t buf[]) { decodeWheelSpeeds(buf); }

    void decodeWheelSpeeds(uint8_t buf[]) {
        // All values encoded as (kph * 100) + 10000
        wheelSpeeds.frontLeft = ((buf[0] << 8) | buf[1]) - 10000;
        wheelSpeeds.frontRight = ((buf[2] << 8) | buf[3]) - 10000;
        wheelSpeeds.rearLeft = ((buf[4] << 8) | buf[5]) - 10000;
        wheelSpeeds.rearRight = ((buf[6] << 8) | buf[7]) - 10000;

        // Calculate averages
        wheelSpeeds.averageFront = (wheelSpeeds.frontLeft + wheelSpeeds.frontRight) / 2;
        wheelSpeeds.averageRear = (wheelSpeeds.rearLeft + wheelSpeeds.rearRight) / 2;
        wheelSpeeds.averageAll = (wheelSpeeds.frontLeft + wheelSpeeds.frontRight +
                                  wheelSpeeds.rearLeft + wheelSpeeds.rearRight) / 4;

        // Check for wheel speed mismatch (difference > 500 = 5 kph)
        int frontDiff = abs(wheelSpeeds.frontLeft - wheelSpeeds.frontRight);
        wheelSpeeds.wheelSpeedMismatch = (frontDiff > 500);
    }

    // Decode 0x4BE - Steering Angle
    void decode0x4BE(uint8_t buf[]) {
        // Bytes 0-1: Signed 16-bit steering angle
        // Range: -3120 to +3120 (maps to -780° to +780°)
        // Formula: angle_degrees = raw_value / 4
        steeringAngle.rawAngle = (int16_t)((buf[0] << 8) | buf[1]);
        steeringAngle.steeringAngle = steeringAngle.rawAngle / 4.0;

        // Check if centered (within ±5 degrees)
        steeringAngle.centerCalibrated = (abs(steeringAngle.steeringAngle) < 5.0);
    }

    // Decode 0x075 - Accelerometer Data
    void decode0x075(uint8_t buf[]) {
        // Bytes 0-1: Lateral G (left/right)
        // Bytes 2-3: Longitudinal G (forward/back)
        // Bytes 4-5: Vertical G (up/down)
        // All are signed 16-bit values
        // Conversion factor varies - typically divide by ~1000 for G-force

        accelerometer.rawLateral = (int16_t)((buf[0] << 8) | buf[1]);
        accelerometer.rawLongitudinal = (int16_t)((buf[2] << 8) | buf[3]);
        accelerometer.rawVertical = (int16_t)((buf[4] << 8) | buf[5]);

        // Convert to G-force (approximate conversion)
        // Note: Exact conversion factor may need calibration
        accelerometer.lateralG = accelerometer.rawLateral / 1000.0;
        accelerometer.longitudinalG = accelerometer.rawLongitudinal / 1000.0;
        accelerometer.verticalG = accelerometer.rawVertical / 1000.0;
    }

    // Decode 0x620 - ABS System Data
    void decode0x620(uint8_t buf[]) {
        // Store full message
        memcpy(absData.byte, buf, 7);

        // Byte 4: ABS/DSC status
        absData.absStatus = buf[4];

        // Warning lights (bit positions vary by vehicle)
        // These are typical but may need adjustment
        absData.absWarning = (buf[4] & 0x10) ? true : false;
        absData.brakeFailWarning = (buf[5] & 0x01) ? true : false;
    }

    // Decode 0x630 - ABS Configuration
    void decode0x630(uint8_t buf[]) {
        // Store full message
        memcpy(absConfig.byte, buf, 8);

        // Byte 0: Transmission type
        //   8 = typical value for RX8
        //   Varies by AT/MT and specific configuration
        absConfig.transmissionType = buf[0];
        absConfig.isAutomatic = (buf[0] != 0); // Simplified - actual logic more complex

        // Bytes 6-7: Wheel size parameters
        //   Typically both 106 for RX8
        absConfig.wheelSizeParam1 = buf[6];
        absConfig.wheelSizeParam2 = buf[7];
    }

    // Decode 0x212 - DSC/ABS Status
    void decode0x212(uint8_t buf[]) {
        // Store full message
        memcpy(dscStatus.byte, buf, 7);

        // Decode DSC/ABS warning lights
        // Note: Bit positions may vary by vehicle configuration
        dscStatus.dscOff = (buf[0] & 0x01) ? true : false;
        dscStatus.absMIL = (buf[1] & 0x10) ? true : false;
        dscStatus.brakeFailMIL = (buf[2] & 0x01) ? true : false;
        dscStatus.etcActiveBL = (buf[3] & 0x80) ? true : false;
        dscStatus.etcDisabled = (buf[4] & 0x01) ? true : false;
    }

    // ========================================================================
    // HELPER FUNCTIONS
    // ========================================================================

    // Convert speed from MPH to KPH
    int speedMPHtoKPH(int mph) {
        return (int)(mph * 1.60934);
    }

    // Convert speed from KPH to MPH
    int speedKPHtoMPH(int kph) {
        return (int)(kph / 1.60934);
    }

    // Convert temperature from raw value to Celsius (approximate)
    int tempToCelsius(int rawTemp) {
        // RX8 temp sensor: 145 = ~90°C normal operating temp
        // Approximate conversion: Celsius = (raw - 55) * 0.75
        return (int)((rawTemp - 55) * 0.75);
    }

    // Convert temperature from raw value to Fahrenheit
    int tempToFahrenheit(int rawTemp) {
        int celsius = tempToCelsius(rawTemp);
        return (int)(celsius * 9.0 / 5.0 + 32);
    }

    // Check if coolant temperature is in normal range
    bool isCoolantTempNormal() {
        return (warningLights.coolantTemperature >= 140 &&
                warningLights.coolantTemperature <= 150);
    }

    // Check if any warning lights are active
    bool hasActiveWarnings() {
        return (warningLights.checkEngineMIL ||
                warningLights.lowCoolantMIL ||
                warningLights.batteryChargeMIL ||
                warningLights.oilPressureMIL ||
                warningLights.engineOverheat ||
                warningLights.catalystTempHigh);
    }

    // Get wheel speed in MPH (from kph*100 encoding)
    int getWheelSpeedMPH(int encodedSpeed) {
        int kph = encodedSpeed / 100;  // Convert from kph*100 to kph
        return speedKPHtoMPH(kph);
    }

    // Print all decoded values (for debugging)
    void printAll(Stream &stream = Serial) {
        stream.println("=== RX8 CAN Data ===");

        stream.print("RPM: "); stream.print(pcmStatus.engineRPM);
        stream.print(" | Speed: "); stream.print(pcmStatus.vehicleSpeed);
        stream.print(" | Throttle: "); stream.print(pcmStatus.throttlePosition);
        stream.println("%");

        stream.print("Coolant: "); stream.print(warningLights.coolantTemperature);
        stream.print(" ("); stream.print(tempToCelsius(warningLights.coolantTemperature));
        stream.println("°C)");

        stream.print("Warnings: ");
        if(hasActiveWarnings()) {
            if(warningLights.checkEngineMIL) stream.print("CEL ");
            if(warningLights.lowCoolantMIL) stream.print("LOW_COOLANT ");
            if(warningLights.batteryChargeMIL) stream.print("BATTERY ");
            if(warningLights.oilPressureMIL) stream.print("OIL ");
            if(warningLights.engineOverheat) stream.print("OVERHEAT ");
            stream.println();
        } else {
            stream.println("None");
        }

        stream.print("Wheel Speeds - FL: "); stream.print(wheelSpeeds.frontLeft / 100);
        stream.print(" FR: "); stream.print(wheelSpeeds.frontRight / 100);
        stream.print(" RL: "); stream.print(wheelSpeeds.rearLeft / 100);
        stream.print(" RR: "); stream.println(wheelSpeeds.rearRight / 100);

        stream.print("Steering Angle: "); stream.print(steeringAngle.steeringAngle);
        stream.println("°");

        stream.print("Accelerometer - Lat: "); stream.print(accelerometer.lateralG);
        stream.print("G | Long: "); stream.print(accelerometer.longitudinalG);
        stream.print("G | Vert: "); stream.print(accelerometer.verticalG);
        stream.println("G");

        stream.println("===================");
    }
};

// ============================================================================
// ENCODING FUNCTIONS (For ECU/Transmitting Modules)
// ============================================================================

class RX8_CAN_Encoder {
public:
    // Encode 0x201 - PCM Status
    static void encode0x201(uint8_t buf[], int rpm, int speed, int throttle) {
        // Bytes 0-1: RPM * 3.85
        uint16_t encodedRPM = (uint16_t)(rpm * 3.85);
        buf[0] = (encodedRPM >> 8) & 0xFF;
        buf[1] = encodedRPM & 0xFF;

        // Bytes 2-3: Fixed
        buf[2] = 0xFF;
        buf[3] = 0xFF;

        // Bytes 4-5: (Speed * 100) + 10000
        uint16_t encodedSpeed = (speed * 100) + 10000;
        buf[4] = (encodedSpeed >> 8) & 0xFF;
        buf[5] = encodedSpeed & 0xFF;

        // Byte 6: Throttle * 2 (0.5% increments)
        buf[6] = throttle * 2;

        // Byte 7: Fixed
        buf[7] = 0xFF;
    }

    // Encode 0x420 - Warning Lights
    static void encode0x420(uint8_t buf[], int temp, bool cel, bool lowCoolant,
                           bool batteryCharge, bool oilPressure) {
        buf[0] = temp;  // Coolant temperature
        buf[1] = 0;     // Odometer byte
        buf[2] = 0;
        buf[3] = 0;
        buf[4] = oilPressure ? 0 : 1;  // 1 = OK

        // Byte 5: MIL bits
        buf[5] = 0;
        if(cel) buf[5] |= 0x40;  // Check Engine MIL

        // Byte 6: Warning bits
        buf[6] = 0;
        if(lowCoolant) buf[6] |= 0x02;
        if(batteryCharge) buf[6] |= 0x40;
        if(oilPressure) buf[6] |= 0x80;
    }

    // Encode wheel speeds
    static void encodeWheelSpeeds(uint8_t buf[], int fl, int fr, int rl, int rr) {
        // All values are (kph * 100) + 10000
        uint16_t encFL = fl + 10000;
        uint16_t encFR = fr + 10000;
        uint16_t encRL = rl + 10000;
        uint16_t encRR = rr + 10000;

        buf[0] = (encFL >> 8) & 0xFF;
        buf[1] = encFL & 0xFF;
        buf[2] = (encFR >> 8) & 0xFF;
        buf[3] = encFR & 0xFF;
        buf[4] = (encRL >> 8) & 0xFF;
        buf[5] = encRL & 0xFF;
        buf[6] = (encRR >> 8) & 0xFF;
        buf[7] = encRR & 0xFF;
    }

    // Encode 0x212 - DSC/ABS Status
    static void encode0x212(uint8_t buf[], bool dscOff, bool absMIL,
                           bool brakeFailMIL, bool etcActiveBL, bool etcDisabled) {
        // Initialize to zeros
        memset(buf, 0, 7);

        // Encode warning bits
        // Note: Bit positions validated from ECU_Module code
        if(dscOff) buf[3] |= 0x04;         // Byte 3, bit 2
        if(absMIL) buf[4] |= 0x08;         // Byte 4, bit 3
        if(brakeFailMIL) buf[4] |= 0x40;   // Byte 4, bit 6
        if(etcActiveBL) buf[5] |= 0x20;    // Byte 5, bit 5
        if(etcDisabled) buf[5] |= 0x10;    // Byte 5, bit 4
    }

    // ========================================================================
    // STATIC MESSAGE ENCODERS (for fixed PCM status messages)
    // ========================================================================

    // Encode 0x203 - Traction Control Data (typically fixed)
    static void encode0x203(uint8_t buf[]) {
        // Standard RX8 traction control message
        // From ECU_Module: {19,19,19,19,175,3,19}
        buf[0] = 19;
        buf[1] = 19;
        buf[2] = 19;
        buf[3] = 19;
        buf[4] = 175;
        buf[5] = 3;
        buf[6] = 19;
    }

    // Encode 0x215 - PCM Status Supplement 1 (typically fixed)
    static void encode0x215(uint8_t buf[]) {
        // Standard RX8 PCM supplement message
        // From ECU_Module: {2,45,2,45,2,42,6,129}
        buf[0] = 2;
        buf[1] = 45;
        buf[2] = 2;
        buf[3] = 45;
        buf[4] = 2;
        buf[5] = 42;
        buf[6] = 6;
        buf[7] = 129;
    }

    // Encode 0x231 - PCM Status Supplement 2 (typically fixed)
    static void encode0x231(uint8_t buf[]) {
        // Standard RX8 PCM supplement message
        // From ECU_Module: {15,0,255,255,0}
        buf[0] = 15;
        buf[1] = 0;
        buf[2] = 255;
        buf[3] = 255;
        buf[4] = 0;
    }

    // Encode 0x240 - PCM Status Supplement 3 (typically fixed)
    static void encode0x240(uint8_t buf[]) {
        // Standard RX8 PCM supplement message
        // From ECU_Module: {4,0,40,0,2,55,6,129}
        buf[0] = 4;
        buf[1] = 0;
        buf[2] = 40;
        buf[3] = 0;
        buf[4] = 2;
        buf[5] = 55;
        buf[6] = 6;
        buf[7] = 129;
    }

    // Encode 0x620 - ABS System Data
    // Byte 6 (index 6) varies by vehicle: 2, 3, or 4
    static void encode0x620(uint8_t buf[], uint8_t variant = 4) {
        // Standard RX8 ABS data message
        // From ECU_Module: {0,0,0,0,16,0,4}
        buf[0] = 0;
        buf[1] = 0;
        buf[2] = 0;
        buf[3] = 0;
        buf[4] = 16;
        buf[5] = 0;
        buf[6] = variant;  // Vehicle-specific: 2, 3, or 4
    }

    // Encode 0x630 - ABS Configuration
    static void encode0x630(uint8_t buf[], uint8_t transmissionType = 8,
                           uint8_t wheelSize1 = 106, uint8_t wheelSize2 = 106) {
        // Standard RX8 ABS config message
        // From ECU_Module: {8,0,0,0,0,0,106,106}
        buf[0] = transmissionType;  // 8 = typical RX8 value
        buf[1] = 0;
        buf[2] = 0;
        buf[3] = 0;
        buf[4] = 0;
        buf[5] = 0;
        buf[6] = wheelSize1;  // Typically 106
        buf[7] = wheelSize2;  // Typically 106
    }

    // Encode 0x650 - ABS Supplement (single byte)
    static void encode0x650(uint8_t buf[]) {
        // Standard RX8 ABS supplement message
        // From ECU_Module: {0}
        buf[0] = 0;
    }

    // ========================================================================
    // IMMOBILIZER RESPONSE ENCODERS
    // ========================================================================

    // Encode 0x041 - Immobilizer Response Type A
    // Response to 0x047 with buf[1]=127, buf[2]=2
    static void encode0x041_ResponseA(uint8_t buf[]) {
        // From ECU_Module: {7,12,48,242,23,0,0,0}
        buf[0] = 7;
        buf[1] = 12;
        buf[2] = 48;
        buf[3] = 242;
        buf[4] = 23;
        buf[5] = 0;
        buf[6] = 0;
        buf[7] = 0;
    }

    // Encode 0x041 - Immobilizer Response Type B
    // Response to 0x047 with buf[1]=92, buf[2]=244
    static void encode0x041_ResponseB(uint8_t buf[]) {
        // From ECU_Module: {129,127,0,0,0,0,0,0}
        buf[0] = 129;
        buf[1] = 127;
        buf[2] = 0;
        buf[3] = 0;
        buf[4] = 0;
        buf[5] = 0;
        buf[6] = 0;
        buf[7] = 0;
    }

    // ========================================================================
    // CONVENIENCE FUNCTIONS
    // ========================================================================

    // Initialize all ECU messages to default values
    // Call this once at startup to get standard RX8 ECU messages
    static void initializeECUMessages(uint8_t send203[7], uint8_t send215[8],
                                     uint8_t send231[5], uint8_t send240[8],
                                     uint8_t send620[7], uint8_t send630[8],
                                     uint8_t send650[1]) {
        encode0x203(send203);
        encode0x215(send215);
        encode0x231(send231);
        encode0x240(send240);
        encode0x620(send620);
        encode0x630(send630);
        encode0x650(send650);
    }

    // Update dynamic PCM status (call this every cycle with current values)
    static void updatePCMStatus(uint8_t send201[8], uint8_t send420[7],
                               int rpm, int speed, int throttle,
                               int temp, bool cel, bool lowCoolant,
                               bool batteryCharge, bool oilPressure) {
        encode0x201(send201, rpm, speed, throttle);
        encode0x420(send420, temp, cel, lowCoolant, batteryCharge, oilPressure);
    }
};

#endif // RX8_CAN_MESSAGES_H
