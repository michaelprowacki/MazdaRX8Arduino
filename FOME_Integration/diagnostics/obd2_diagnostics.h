/*
 * OBD-II Diagnostics
 *
 * Standard OBD-II PID support for diagnostics and scan tools
 * Supports Mode 01 (current data), Mode 03 (DTCs), Mode 04 (clear DTCs)
 */

#ifndef OBD2_DIAGNOSTICS_H
#define OBD2_DIAGNOSTICS_H

#include <stdint.h>
#include <stdbool.h>

// OBD-II CAN IDs
#define OBD2_REQUEST_ID     0x7DF  // Broadcast request
#define OBD2_RESPONSE_ID    0x7E8  // ECU response (ECU #1)
#define OBD2_RESPONSE_ID2   0x7E9  // ECU response (ECU #2)

// OBD-II Service modes
#define OBD2_MODE_CURRENT_DATA      0x01
#define OBD2_MODE_FREEZE_FRAME      0x02
#define OBD2_MODE_READ_DTC          0x03
#define OBD2_MODE_CLEAR_DTC         0x04
#define OBD2_MODE_O2_TEST           0x05
#define OBD2_MODE_TEST_RESULTS      0x06
#define OBD2_MODE_PENDING_DTC       0x07
#define OBD2_MODE_CONTROL           0x08
#define OBD2_MODE_VEHICLE_INFO      0x09
#define OBD2_MODE_PERMANENT_DTC     0x0A

// Common Mode 01 PIDs
#define OBD2_PID_SUPPORTED_01_20    0x00
#define OBD2_PID_MIL_STATUS         0x01
#define OBD2_PID_FREEZE_DTC         0x02
#define OBD2_PID_FUEL_STATUS        0x03
#define OBD2_PID_ENGINE_LOAD        0x04
#define OBD2_PID_COOLANT_TEMP       0x05
#define OBD2_PID_SHORT_FUEL_TRIM_1  0x06
#define OBD2_PID_LONG_FUEL_TRIM_1   0x07
#define OBD2_PID_SHORT_FUEL_TRIM_2  0x08
#define OBD2_PID_LONG_FUEL_TRIM_2   0x09
#define OBD2_PID_FUEL_PRESSURE      0x0A
#define OBD2_PID_INTAKE_MAP         0x0B
#define OBD2_PID_ENGINE_RPM         0x0C
#define OBD2_PID_VEHICLE_SPEED      0x0D
#define OBD2_PID_TIMING_ADVANCE     0x0E
#define OBD2_PID_INTAKE_TEMP        0x0F
#define OBD2_PID_MAF_RATE           0x10
#define OBD2_PID_THROTTLE_POS       0x11
#define OBD2_PID_O2_SENSOR_STATUS   0x13
#define OBD2_PID_O2_SENSOR_1        0x14
#define OBD2_PID_RUNTIME            0x1F
#define OBD2_PID_SUPPORTED_21_40    0x20
#define OBD2_PID_DISTANCE_MIL       0x21
#define OBD2_PID_FUEL_RAIL_PRES     0x22
#define OBD2_PID_FUEL_RAIL_PRES_D   0x23
#define OBD2_PID_COMMANDED_EGR      0x2C
#define OBD2_PID_EGR_ERROR          0x2D
#define OBD2_PID_FUEL_LEVEL         0x2F
#define OBD2_PID_WARMUPS_SINCE_CLR  0x30
#define OBD2_PID_DISTANCE_SINCE_CLR 0x31
#define OBD2_PID_BARO_PRESSURE      0x33
#define OBD2_PID_CATALYST_TEMP_1_1  0x3C
#define OBD2_PID_SUPPORTED_41_60    0x40
#define OBD2_PID_CONTROL_MODULE_V   0x42
#define OBD2_PID_ABS_ENGINE_LOAD    0x43
#define OBD2_PID_COMMANDED_AFR      0x44
#define OBD2_PID_REL_THROTTLE_POS   0x45
#define OBD2_PID_AMBIENT_TEMP       0x46
#define OBD2_PID_ABS_THROTTLE_B     0x47
#define OBD2_PID_ABS_THROTTLE_C     0x48
#define OBD2_PID_ACCEL_PEDAL_D      0x49
#define OBD2_PID_ACCEL_PEDAL_E      0x4A
#define OBD2_PID_COMMANDED_THROTTLE 0x4C
#define OBD2_PID_ENGINE_RUN_TIME    0x4D
#define OBD2_PID_FUEL_TYPE          0x51
#define OBD2_PID_ETHANOL_PERCENT    0x52
#define OBD2_PID_ENGINE_OIL_TEMP    0x5C
#define OBD2_PID_FUEL_INJECT_TIME   0x5D
#define OBD2_PID_FUEL_RATE          0x5E
#define OBD2_PID_SUPPORTED_61_80    0x60

// DTC structure
#define MAX_DTCS 16

typedef struct {
    uint16_t code;      // DTC code (e.g., P0301 = 0x0301)
    bool active;        // Currently active
    bool pending;       // Pending (not confirmed)
    bool permanent;     // Permanent (can't be cleared)
} Obd2Dtc;

// OBD-II data structure for live values
typedef struct {
    // Engine
    uint16_t rpm;               // Engine RPM
    uint8_t engineLoad;         // Calculated engine load (%)
    int8_t coolantTemp;         // Coolant temp (°C, -40 to 215)
    int8_t intakeTemp;          // Intake air temp (°C)
    uint8_t throttlePos;        // Throttle position (%)
    uint16_t mafRate;           // MAF rate (g/s * 100)
    int8_t timingAdvance;       // Timing advance (° - 64)

    // Vehicle
    uint8_t vehicleSpeed;       // Speed (km/h)
    uint16_t runtime;           // Engine runtime (seconds)
    uint16_t distanceMil;       // Distance with MIL on (km)
    uint16_t distanceCleared;   // Distance since codes cleared (km)

    // Fuel
    uint8_t fuelLevel;          // Fuel level (%)
    uint16_t fuelPressure;      // Fuel rail pressure (kPa)
    uint16_t fuelRate;          // Fuel consumption (L/h * 20)
    int8_t shortFuelTrim1;      // Short term fuel trim bank 1 (%)
    int8_t longFuelTrim1;       // Long term fuel trim bank 1 (%)

    // Electrical
    uint16_t controlModuleV;    // Control module voltage (mV)
    uint8_t baroPressure;       // Barometric pressure (kPa)
    int8_t ambientTemp;         // Ambient temperature (°C)

    // Status
    bool milOn;                 // MIL light on
    uint8_t numDtcs;            // Number of stored DTCs
} Obd2LiveData;

// OBD-II diagnostics class
class Obd2Diagnostics {
public:
    // Initialize OBD-II handler
    void init();

    // Process incoming OBD-II request
    // Returns true if message was handled
    bool processRequest(uint32_t canId, uint8_t* data, uint8_t length);

    // Update live data (call from main loop)
    void setRpm(uint16_t rpm) { m_liveData.rpm = rpm; }
    void setEngineLoad(uint8_t load) { m_liveData.engineLoad = load; }
    void setCoolantTemp(int8_t temp) { m_liveData.coolantTemp = temp; }
    void setIntakeTemp(int8_t temp) { m_liveData.intakeTemp = temp; }
    void setThrottlePos(uint8_t pos) { m_liveData.throttlePos = pos; }
    void setVehicleSpeed(uint8_t speed) { m_liveData.vehicleSpeed = speed; }
    void setMafRate(uint16_t rate) { m_liveData.mafRate = rate; }
    void setFuelLevel(uint8_t level) { m_liveData.fuelLevel = level; }
    void setFuelPressure(uint16_t pressure) { m_liveData.fuelPressure = pressure; }
    void setControlVoltage(uint16_t mv) { m_liveData.controlModuleV = mv; }
    void setRuntime(uint16_t seconds) { m_liveData.runtime = seconds; }
    void setBaroPressure(uint8_t kpa) { m_liveData.baroPressure = kpa; }
    void setAmbientTemp(int8_t temp) { m_liveData.ambientTemp = temp; }
    void setTimingAdvance(int8_t advance) { m_liveData.timingAdvance = advance; }
    void setShortFuelTrim(int8_t trim) { m_liveData.shortFuelTrim1 = trim; }
    void setLongFuelTrim(int8_t trim) { m_liveData.longFuelTrim1 = trim; }

    // DTC management
    void setDtc(uint16_t code, bool active = true, bool pending = false);
    void clearDtc(uint16_t code);
    void clearAllDtcs();
    uint8_t getDtcCount();

    // Get live data
    const Obd2LiveData& getLiveData() { return m_liveData; }

private:
    Obd2LiveData m_liveData;
    Obd2Dtc m_dtcs[MAX_DTCS];
    uint8_t m_dtcCount;

    // Send OBD-II response
    void sendResponse(uint8_t* data, uint8_t length);

    // Handle specific modes
    void handleMode01(uint8_t pid);
    void handleMode03();
    void handleMode04();
    void handleMode09(uint8_t pid);

    // Get supported PIDs bitmask
    uint32_t getSupportedPids01_20();
    uint32_t getSupportedPids21_40();
    uint32_t getSupportedPids41_60();
};

// -----------------------------------------
// Implementation
// -----------------------------------------

inline void Obd2Diagnostics::init() {
    m_dtcCount = 0;

    // Initialize live data with defaults
    m_liveData.rpm = 0;
    m_liveData.engineLoad = 0;
    m_liveData.coolantTemp = 20;
    m_liveData.intakeTemp = 20;
    m_liveData.throttlePos = 0;
    m_liveData.vehicleSpeed = 0;
    m_liveData.mafRate = 0;
    m_liveData.fuelLevel = 50;
    m_liveData.fuelPressure = 0;
    m_liveData.controlModuleV = 12000;
    m_liveData.runtime = 0;
    m_liveData.baroPressure = 101;
    m_liveData.ambientTemp = 20;
    m_liveData.timingAdvance = 0;
    m_liveData.shortFuelTrim1 = 0;
    m_liveData.longFuelTrim1 = 0;
    m_liveData.milOn = false;
    m_liveData.numDtcs = 0;
}

inline bool Obd2Diagnostics::processRequest(uint32_t canId, uint8_t* data, uint8_t length) {
    // Check if this is an OBD-II request
    if (canId != OBD2_REQUEST_ID && canId != 0x7E0) {
        return false;
    }

    if (length < 2) {
        return false;
    }

    uint8_t numBytes = data[0];
    uint8_t mode = data[1];

    (void)numBytes;  // Suppress unused warning

    switch (mode) {
        case OBD2_MODE_CURRENT_DATA:
            if (length >= 3) {
                handleMode01(data[2]);
            }
            break;

        case OBD2_MODE_READ_DTC:
            handleMode03();
            break;

        case OBD2_MODE_CLEAR_DTC:
            handleMode04();
            break;

        case OBD2_MODE_VEHICLE_INFO:
            if (length >= 3) {
                handleMode09(data[2]);
            }
            break;

        default:
            return false;
    }

    return true;
}

inline void Obd2Diagnostics::handleMode01(uint8_t pid) {
    uint8_t response[8] = {0};
    uint8_t len = 0;

    response[0] = 0x06;  // Number of data bytes
    response[1] = 0x41;  // Mode 01 response
    response[2] = pid;

    switch (pid) {
        case OBD2_PID_SUPPORTED_01_20:
            {
                uint32_t supported = getSupportedPids01_20();
                response[3] = (supported >> 24) & 0xFF;
                response[4] = (supported >> 16) & 0xFF;
                response[5] = (supported >> 8) & 0xFF;
                response[6] = supported & 0xFF;
                len = 7;
            }
            break;

        case OBD2_PID_MIL_STATUS:
            response[3] = m_liveData.milOn ? 0x80 : 0x00;
            response[3] |= m_dtcCount & 0x7F;
            response[4] = 0x07;  // Tests available
            response[5] = 0x00;
            response[6] = 0x00;
            len = 7;
            break;

        case OBD2_PID_COOLANT_TEMP:
            response[3] = m_liveData.coolantTemp + 40;
            len = 4;
            break;

        case OBD2_PID_ENGINE_RPM:
            response[3] = (m_liveData.rpm * 4) >> 8;
            response[4] = (m_liveData.rpm * 4) & 0xFF;
            len = 5;
            break;

        case OBD2_PID_VEHICLE_SPEED:
            response[3] = m_liveData.vehicleSpeed;
            len = 4;
            break;

        case OBD2_PID_THROTTLE_POS:
            response[3] = (m_liveData.throttlePos * 255) / 100;
            len = 4;
            break;

        case OBD2_PID_ENGINE_LOAD:
            response[3] = (m_liveData.engineLoad * 255) / 100;
            len = 4;
            break;

        case OBD2_PID_INTAKE_TEMP:
            response[3] = m_liveData.intakeTemp + 40;
            len = 4;
            break;

        case OBD2_PID_MAF_RATE:
            response[3] = m_liveData.mafRate >> 8;
            response[4] = m_liveData.mafRate & 0xFF;
            len = 5;
            break;

        case OBD2_PID_TIMING_ADVANCE:
            response[3] = (m_liveData.timingAdvance + 64) * 2;
            len = 4;
            break;

        case OBD2_PID_RUNTIME:
            response[3] = m_liveData.runtime >> 8;
            response[4] = m_liveData.runtime & 0xFF;
            len = 5;
            break;

        case OBD2_PID_SUPPORTED_21_40:
            {
                uint32_t supported = getSupportedPids21_40();
                response[3] = (supported >> 24) & 0xFF;
                response[4] = (supported >> 16) & 0xFF;
                response[5] = (supported >> 8) & 0xFF;
                response[6] = supported & 0xFF;
                len = 7;
            }
            break;

        case OBD2_PID_FUEL_LEVEL:
            response[3] = (m_liveData.fuelLevel * 255) / 100;
            len = 4;
            break;

        case OBD2_PID_BARO_PRESSURE:
            response[3] = m_liveData.baroPressure;
            len = 4;
            break;

        case OBD2_PID_SUPPORTED_41_60:
            {
                uint32_t supported = getSupportedPids41_60();
                response[3] = (supported >> 24) & 0xFF;
                response[4] = (supported >> 16) & 0xFF;
                response[5] = (supported >> 8) & 0xFF;
                response[6] = supported & 0xFF;
                len = 7;
            }
            break;

        case OBD2_PID_CONTROL_MODULE_V:
            response[3] = m_liveData.controlModuleV >> 8;
            response[4] = m_liveData.controlModuleV & 0xFF;
            len = 5;
            break;

        case OBD2_PID_AMBIENT_TEMP:
            response[3] = m_liveData.ambientTemp + 40;
            len = 4;
            break;

        case OBD2_PID_SHORT_FUEL_TRIM_1:
            response[3] = ((m_liveData.shortFuelTrim1 * 128) / 100) + 128;
            len = 4;
            break;

        case OBD2_PID_LONG_FUEL_TRIM_1:
            response[3] = ((m_liveData.longFuelTrim1 * 128) / 100) + 128;
            len = 4;
            break;

        default:
            return;  // Unsupported PID
    }

    if (len > 0) {
        response[0] = len - 1;  // Update length byte
        sendResponse(response, len);
    }
}

inline void Obd2Diagnostics::handleMode03() {
    uint8_t response[8] = {0};

    response[0] = 2 + (m_dtcCount * 2);
    response[1] = 0x43;  // Mode 03 response
    response[2] = m_dtcCount;

    // Send DTCs (max 3 per message)
    uint8_t idx = 3;
    for (int i = 0; i < m_dtcCount && i < 3; i++) {
        response[idx++] = m_dtcs[i].code >> 8;
        response[idx++] = m_dtcs[i].code & 0xFF;
    }

    sendResponse(response, idx);
}

inline void Obd2Diagnostics::handleMode04() {
    clearAllDtcs();

    uint8_t response[8] = {1, 0x44, 0, 0, 0, 0, 0, 0};
    sendResponse(response, 2);
}

inline void Obd2Diagnostics::handleMode09(uint8_t pid) {
    uint8_t response[8] = {0};

    response[1] = 0x49;  // Mode 09 response
    response[2] = pid;

    switch (pid) {
        case 0x02:  // VIN (first frame)
            response[0] = 5;
            response[3] = 1;  // Number of data items
            response[4] = 'F';
            response[5] = 'O';
            response[6] = 'M';
            sendResponse(response, 7);
            break;

        case 0x04:  // Calibration ID
            response[0] = 5;
            response[3] = 1;
            response[4] = 'R';
            response[5] = 'X';
            response[6] = '8';
            sendResponse(response, 7);
            break;

        default:
            break;
    }
}

inline void Obd2Diagnostics::setDtc(uint16_t code, bool active, bool pending) {
    // Check if DTC already exists
    for (int i = 0; i < m_dtcCount; i++) {
        if (m_dtcs[i].code == code) {
            m_dtcs[i].active = active;
            m_dtcs[i].pending = pending;
            return;
        }
    }

    // Add new DTC
    if (m_dtcCount < MAX_DTCS) {
        m_dtcs[m_dtcCount].code = code;
        m_dtcs[m_dtcCount].active = active;
        m_dtcs[m_dtcCount].pending = pending;
        m_dtcs[m_dtcCount].permanent = false;
        m_dtcCount++;
        m_liveData.numDtcs = m_dtcCount;

        if (active) {
            m_liveData.milOn = true;
        }
    }
}

inline void Obd2Diagnostics::clearDtc(uint16_t code) {
    for (int i = 0; i < m_dtcCount; i++) {
        if (m_dtcs[i].code == code && !m_dtcs[i].permanent) {
            // Shift remaining DTCs
            for (int j = i; j < m_dtcCount - 1; j++) {
                m_dtcs[j] = m_dtcs[j + 1];
            }
            m_dtcCount--;
            m_liveData.numDtcs = m_dtcCount;
            return;
        }
    }
}

inline void Obd2Diagnostics::clearAllDtcs() {
    // Clear non-permanent DTCs
    int newCount = 0;
    for (int i = 0; i < m_dtcCount; i++) {
        if (m_dtcs[i].permanent) {
            m_dtcs[newCount++] = m_dtcs[i];
        }
    }
    m_dtcCount = newCount;
    m_liveData.numDtcs = m_dtcCount;
    m_liveData.milOn = (m_dtcCount > 0);
}

inline uint8_t Obd2Diagnostics::getDtcCount() {
    return m_dtcCount;
}

inline uint32_t Obd2Diagnostics::getSupportedPids01_20() {
    // PIDs 01, 05, 0C, 0D, 0F, 10, 11, 1F, 20
    return 0b10011000000110001000000000000001;
}

inline uint32_t Obd2Diagnostics::getSupportedPids21_40() {
    // PIDs 2F, 33, 40
    return 0b00000000000000010000000100000001;
}

inline uint32_t Obd2Diagnostics::getSupportedPids41_60() {
    // PIDs 42, 46
    return 0b01000100000000000000000000000000;
}

// Platform-specific send (to be implemented)
extern void obd2SendCan(uint32_t id, uint8_t* data, uint8_t length);

inline void Obd2Diagnostics::sendResponse(uint8_t* data, uint8_t length) {
    obd2SendCan(OBD2_RESPONSE_ID, data, length);
}

// Global instance
extern Obd2Diagnostics obd2Diag;

#endif // OBD2_DIAGNOSTICS_H
