# Adding Advanced Features to FOME

Plan to integrate body systems support, Simulink, and XCP/CCP into FOME.

## Feature Overview

| Feature | Current FOME | Implementation Approach |
|---------|-------------|------------------------|
| Dashboard CAN | No | Integrate RX8 CAN encoder |
| ABS/DSC | No | Add vehicle-specific modules |
| Power Steering | No | CAN message generation |
| Immobilizer | No | Protocol handlers |
| Simulink | No | Code generation target |
| XCP/CCP | No | Protocol implementation |

---

## 1. Body Systems Integration (RX8 Specific)

### Approach: Add Vehicle Profile System

Create a modular vehicle profile system in FOME that handles body electronics for different vehicles.

**File Structure**:
```
fome-fw/
├── firmware/
│   ├── controllers/
│   │   ├── vehicle_profiles/
│   │   │   ├── vehicle_profile.h
│   │   │   ├── rx8_profile.cpp
│   │   │   ├── miata_profile.cpp
│   │   │   └── generic_profile.cpp
│   │   ├── body_control/
│   │   │   ├── dashboard_can.cpp
│   │   │   ├── abs_dsc.cpp
│   │   │   ├── power_steering.cpp
│   │   │   └── immobilizer.cpp
```

### Implementation

**vehicle_profile.h**:
```cpp
#ifndef VEHICLE_PROFILE_H
#define VEHICLE_PROFILE_H

// Base class for vehicle-specific body electronics
class VehicleProfile {
public:
    virtual void initBodyCAN() = 0;
    virtual void sendDashboardMessages() = 0;
    virtual void handleImmobilizer(uint8_t* data) = 0;
    virtual void updateABSDSC() = 0;
    virtual void updatePowerSteering() = 0;

    // Common interface
    void setRPM(int rpm) { engineRPM = rpm; }
    void setSpeed(int speed) { vehicleSpeed = speed; }
    void setTemp(int temp) { coolantTemp = temp; }
    void setWarnings(uint8_t warnings) { warningFlags = warnings; }

protected:
    int engineRPM = 0;
    int vehicleSpeed = 0;
    int coolantTemp = 0;
    uint8_t warningFlags = 0;
};

// Factory function
VehicleProfile* createVehicleProfile(int vehicleType);

#endif
```

**rx8_profile.cpp**:
```cpp
#include "vehicle_profile.h"
#include "can_hw.h"

class RX8Profile : public VehicleProfile {
public:
    void initBodyCAN() override {
        // Initialize RX8-specific CAN messages
        initializeECUMessages();
    }

    void sendDashboardMessages() override {
        // Send all RX8 body CAN messages (100ms cycle)

        // 0x201 - PCM Status (RPM, Speed, Throttle)
        uint8_t msg201[8];
        int tempRPM = engineRPM * 3.85;
        msg201[0] = highByte(tempRPM);
        msg201[1] = lowByte(tempRPM);
        msg201[2] = 0xFF;
        msg201[3] = 0xFF;
        int tempSpeed = (vehicleSpeed * 100) + 10000;
        msg201[4] = highByte(tempSpeed);
        msg201[5] = lowByte(tempSpeed);
        msg201[6] = throttlePedal * 2;
        msg201[7] = 0xFF;
        canTransmit(0x201, msg201, 8);

        // 0x420 - MIL/Warning Lights
        uint8_t msg420[7] = {0};
        msg420[0] = coolantTemp;
        msg420[1] = odometerByte++;
        msg420[4] = 0x01;  // Oil pressure OK
        // Warning light bits
        if (checkEngineMIL) msg420[5] |= 0x40;
        if (lowWaterMIL) msg420[6] |= 0x02;
        if (batChargeMIL) msg420[6] |= 0x40;
        if (oilPressureMIL) msg420[6] |= 0x80;
        canTransmit(0x420, msg420, 7);

        // 0x620 - ABS System
        uint8_t msg620[7] = {0, 0, 0, 0, 16, 0, 4};
        canTransmit(0x620, msg620, 7);

        // 0x630 - ABS Config
        uint8_t msg630[8] = {8, 0, 0, 0, 0, 0, 106, 106};
        canTransmit(0x630, msg630, 8);

        // Additional PCM status messages
        uint8_t msg203[7] = {19, 0, 0, 0, 0, 0, 0};
        uint8_t msg215[8] = {2, 0, 0, 0, 0, 0, 0, 0};
        uint8_t msg231[5] = {15, 0, 82, 0, 0};
        uint8_t msg240[8] = {4, 0, 40, 0, 2, 55, 6, 129};
        uint8_t msg650[1] = {0};

        canTransmit(0x203, msg203, 7);
        canTransmit(0x215, msg215, 8);
        canTransmit(0x231, msg231, 5);
        canTransmit(0x240, msg240, 8);
        canTransmit(0x650, msg650, 1);
    }

    void handleImmobilizer(uint8_t* data) override {
        // RX8 immobilizer handshake on CAN ID 0x047
        if (data[1] == 127 && data[2] == 2) {
            uint8_t response[8] = {7, 12, 48, 242, 23, 0, 0, 0};
            canTransmit(0x041, response, 8);
        }
        if (data[1] == 92 && data[2] == 244) {
            uint8_t response[8] = {129, 127, 0, 0, 0, 0, 0, 0};
            canTransmit(0x041, response, 8);
        }
    }

    void updateABSDSC() override {
        // Optional: Send 0x212 for DSC control
        uint8_t msg212[7] = {0};
        // Set bits based on dscOff, absMIL, etc.
        canTransmit(0x212, msg212, 7);
    }

    void updatePowerSteering() override {
        // RX8 power steering works with RPM from 0x201
        // No additional messages needed
    }

private:
    uint8_t odometerByte = 0;
    bool checkEngineMIL = false;
    bool lowWaterMIL = false;
    bool batChargeMIL = false;
    bool oilPressureMIL = false;
    uint8_t throttlePedal = 0;
};
```

### Integration with FOME Main Loop

**engine_controller.cpp** (modify existing):
```cpp
#include "vehicle_profile.h"

VehicleProfile* vehicleProfile = nullptr;

void initializeEngine() {
    // ... existing FOME init ...

    // Initialize vehicle profile based on config
    int vehicleType = engineConfiguration->vehicleType;
    vehicleProfile = createVehicleProfile(vehicleType);
    if (vehicleProfile) {
        vehicleProfile->initBodyCAN();
    }
}

void onSlowCallback() {  // 100ms callback
    // ... existing FOME slow callback ...

    // Send body CAN messages
    if (vehicleProfile) {
        vehicleProfile->setRPM(Sensor::get(SensorType::Rpm).Value);
        vehicleProfile->setSpeed(Sensor::get(SensorType::VehicleSpeed).Value);
        vehicleProfile->setTemp(Sensor::get(SensorType::Clt).Value);
        vehicleProfile->sendDashboardMessages();
    }
}

void onCanRx(int canId, uint8_t* data, int length) {
    // ... existing FOME CAN handling ...

    // Handle immobilizer
    if (vehicleProfile && canId == 0x047) {
        vehicleProfile->handleImmobilizer(data);
    }
}
```

### TunerStudio Integration

Add to tune file (.ini):
```ini
[VehicleProfile]
vehicleType = bits, U08, 0, [0:3], "Generic", "Mazda RX8", "Mazda Miata NC", "BMW E46"

[RX8Settings]
enableDashboardCAN = bits, U08, 1, [0:0], "No", "Yes"
enableImmobilizer = bits, U08, 1, [1:1], "No", "Yes"
enableABSDSC = bits, U08, 1, [2:2], "No", "Yes"
```

---

## 2. XCP/CCP Protocol Implementation

### What It Enables
- Professional calibration tools (ETAS INCA, Vector CANape, ATI Vision)
- Real-time parameter tuning
- Data acquisition at high rates
- A2L file compatibility

### Implementation

**xcp_protocol.h**:
```cpp
#ifndef XCP_PROTOCOL_H
#define XCP_PROTOCOL_H

// XCP Command Codes
#define XCP_CMD_CONNECT         0xFF
#define XCP_CMD_DISCONNECT      0xFE
#define XCP_CMD_GET_STATUS      0xFD
#define XCP_CMD_SYNCH           0xFC
#define XCP_CMD_SHORT_UPLOAD    0xF4
#define XCP_CMD_UPLOAD          0xF5
#define XCP_CMD_SET_MTA         0xF6
#define XCP_CMD_DOWNLOAD        0xF0
#define XCP_CMD_GET_DAQ_SIZE    0xDA
#define XCP_CMD_SET_DAQ_PTR     0xE2
#define XCP_CMD_WRITE_DAQ       0xE1
#define XCP_CMD_START_STOP_DAQ  0xDE

// XCP Response Codes
#define XCP_PID_RES             0xFF
#define XCP_PID_ERR             0xFE
#define XCP_PID_EV              0xFD
#define XCP_PID_SERV            0xFC

// XCP Resources
#define XCP_RESOURCE_CAL_PAG    0x01
#define XCP_RESOURCE_DAQ        0x04
#define XCP_RESOURCE_STIM       0x08
#define XCP_RESOURCE_PGM        0x10

class XcpSlave {
public:
    void init(int canId);
    void processCommand(uint8_t* data, int length);
    void sendDaqData();

private:
    // Memory Transfer Address
    uint32_t mta = 0;

    // DAQ (Data Acquisition) configuration
    struct DaqList {
        bool active;
        uint16_t eventChannel;
        uint8_t prescaler;
        struct Odt {
            uint32_t address;
            uint8_t size;
        } odts[16];
        int odtCount;
    };
    DaqList daqLists[4];

    // Command handlers
    void cmdConnect(uint8_t* data);
    void cmdDisconnect();
    void cmdGetStatus();
    void cmdSetMta(uint8_t* data);
    void cmdUpload(uint8_t* data);
    void cmdShortUpload(uint8_t* data);
    void cmdDownload(uint8_t* data);
    void cmdGetDaqSize(uint8_t* data);
    void cmdSetDaqPtr(uint8_t* data);
    void cmdWriteDaq(uint8_t* data);
    void cmdStartStopDaq(uint8_t* data);

    void sendResponse(uint8_t* data, int length);
    void sendError(uint8_t errorCode);

    int xcpCanId = 0x600;  // Default XCP CAN ID
    bool connected = false;
};

extern XcpSlave xcpSlave;

#endif
```

**xcp_protocol.cpp**:
```cpp
#include "xcp_protocol.h"
#include "can_hw.h"

XcpSlave xcpSlave;

void XcpSlave::init(int canId) {
    xcpCanId = canId;
    connected = false;
}

void XcpSlave::processCommand(uint8_t* data, int length) {
    uint8_t cmd = data[0];

    switch (cmd) {
        case XCP_CMD_CONNECT:
            cmdConnect(data);
            break;
        case XCP_CMD_DISCONNECT:
            cmdDisconnect();
            break;
        case XCP_CMD_GET_STATUS:
            cmdGetStatus();
            break;
        case XCP_CMD_SET_MTA:
            cmdSetMta(data);
            break;
        case XCP_CMD_UPLOAD:
            cmdUpload(data);
            break;
        case XCP_CMD_SHORT_UPLOAD:
            cmdShortUpload(data);
            break;
        case XCP_CMD_DOWNLOAD:
            cmdDownload(data);
            break;
        case XCP_CMD_GET_DAQ_SIZE:
            cmdGetDaqSize(data);
            break;
        case XCP_CMD_SET_DAQ_PTR:
            cmdSetDaqPtr(data);
            break;
        case XCP_CMD_WRITE_DAQ:
            cmdWriteDaq(data);
            break;
        case XCP_CMD_START_STOP_DAQ:
            cmdStartStopDaq(data);
            break;
        default:
            sendError(0x20);  // Command not available
            break;
    }
}

void XcpSlave::cmdConnect(uint8_t* data) {
    connected = true;

    uint8_t response[8];
    response[0] = XCP_PID_RES;
    response[1] = XCP_RESOURCE_CAL_PAG | XCP_RESOURCE_DAQ;  // Available resources
    response[2] = 0x00;  // COMM_MODE_BASIC
    response[3] = 8;     // MAX_CTO
    response[4] = 0;     // MAX_DTO low
    response[5] = 8;     // MAX_DTO high
    response[6] = 0x01;  // XCP protocol version
    response[7] = 0x01;  // Transport layer version

    sendResponse(response, 8);
}

void XcpSlave::cmdShortUpload(uint8_t* data) {
    // Read memory: data[1]=count, data[2]=reserved, data[3-6]=address
    uint8_t count = data[1];
    uint32_t address = (data[3] << 24) | (data[4] << 16) | (data[5] << 8) | data[6];

    if (count > 6) {
        sendError(0x22);  // Out of range
        return;
    }

    uint8_t response[8];
    response[0] = XCP_PID_RES;

    // Copy memory to response
    memcpy(&response[1], (void*)address, count);

    sendResponse(response, 1 + count);
}

void XcpSlave::cmdDownload(uint8_t* data) {
    // Write memory: data[1]=count, data[2-7]=data
    uint8_t count = data[1];

    if (count > 6) {
        sendError(0x22);
        return;
    }

    // Copy data to MTA
    memcpy((void*)mta, &data[2], count);
    mta += count;

    uint8_t response[2];
    response[0] = XCP_PID_RES;
    response[1] = 0;

    sendResponse(response, 2);
}

void XcpSlave::cmdStartStopDaq(uint8_t* data) {
    uint8_t mode = data[1];
    uint8_t daqListNum = data[2];

    if (daqListNum >= 4) {
        sendError(0x22);
        return;
    }

    if (mode == 0x00) {
        // Stop
        daqLists[daqListNum].active = false;
    } else if (mode == 0x01) {
        // Start
        daqLists[daqListNum].active = true;
    } else if (mode == 0x02) {
        // Select
        // Configure DAQ list
    }

    uint8_t response[2];
    response[0] = XCP_PID_RES;
    response[1] = 0;
    sendResponse(response, 2);
}

void XcpSlave::sendDaqData() {
    // Called periodically to send DAQ data
    for (int i = 0; i < 4; i++) {
        if (!daqLists[i].active) continue;

        for (int j = 0; j < daqLists[i].odtCount; j++) {
            uint8_t data[8];
            data[0] = (i << 4) | j;  // DAQ/ODT identifier

            // Copy data from configured addresses
            uint32_t addr = daqLists[i].odts[j].address;
            uint8_t size = daqLists[i].odts[j].size;
            memcpy(&data[1], (void*)addr, size);

            canTransmit(xcpCanId + 1, data, 1 + size);
        }
    }
}

void XcpSlave::sendResponse(uint8_t* data, int length) {
    canTransmit(xcpCanId + 1, data, length);
}

void XcpSlave::sendError(uint8_t errorCode) {
    uint8_t response[2];
    response[0] = XCP_PID_ERR;
    response[1] = errorCode;
    sendResponse(response, 2);
}
```

### A2L File Generation

Create a tool to generate A2L files from FOME's variable definitions:

**generate_a2l.py**:
```python
#!/usr/bin/env python3
"""Generate A2L file from FOME variable definitions"""

import re

# Parse FOME sensor definitions
sensors = [
    ("rpm", "Engine RPM", "UWORD", 0x20001000, 0, 16000),
    ("coolantTemp", "Coolant Temperature", "SWORD", 0x20001004, -40, 200),
    ("tps", "Throttle Position", "UBYTE", 0x20001008, 0, 100),
    ("map", "Manifold Pressure", "UWORD", 0x2000100A, 0, 400),
    ("afr", "Air Fuel Ratio", "UWORD", 0x2000100C, 0, 30),
    # Add more sensors...
]

# Parse FOME table definitions
tables = [
    ("veTable", "Volumetric Efficiency", 0x20002000, 16, 16, 0, 200),
    ("ignitionTable", "Ignition Timing", 0x20003000, 16, 16, -20, 60),
    # Add more tables...
]

def generate_a2l():
    output = """ASAP2_VERSION 1 71

/begin PROJECT FOME "FOME ECU"
  /begin MODULE ECU "FOME Engine Control Unit"

    /begin MOD_PAR ""
      ADDR_EPK 0x20000000
      EPK "FOME v1.0"
    /end MOD_PAR

    /begin MOD_COMMON ""
      BYTE_ORDER MSB_LAST
      ALIGNMENT_BYTE 1
      ALIGNMENT_WORD 2
      ALIGNMENT_LONG 4
    /end MOD_COMMON

"""

    # Add measurements
    for name, desc, dtype, addr, low, high in sensors:
        output += f"""    /begin MEASUREMENT {name}
      "{desc}"
      {dtype}
      NO_COMPU_METHOD
      0 0 {low} {high}
      ECU_ADDRESS 0x{addr:08X}
    /end MEASUREMENT

"""

    # Add characteristics (tables)
    for name, desc, addr, rows, cols, low, high in tables:
        output += f"""    /begin CHARACTERISTIC {name}
      "{desc}"
      MAP
      0x{addr:08X}
      NO_DEPOSIT
      100.0
      NO_COMPU_METHOD
      {low} {high}
      /begin AXIS_DESCR STD_AXIS rpm_axis NO_COMPU_METHOD {cols} 0 16000
      /end AXIS_DESCR
      /begin AXIS_DESCR STD_AXIS load_axis NO_COMPU_METHOD {rows} 0 400
      /end AXIS_DESCR
    /end CHARACTERISTIC

"""

    output += """  /end MODULE
/end PROJECT
"""

    return output

if __name__ == "__main__":
    with open("fome.a2l", "w") as f:
        f.write(generate_a2l())
    print("Generated fome.a2l")
```

---

## 3. Simulink Integration

### Approach: Simulink Embedded Coder Target

Create a Simulink target that generates C code compatible with FOME.

### Option A: Export to C (Simpler)

1. **Develop model in Simulink**
2. **Generate C code** with Embedded Coder
3. **Integrate generated code** into FOME manually

**Workflow**:
```
Simulink Model → Embedded Coder → C Code → Copy to FOME → Compile
```

**Integration wrapper**:
```cpp
// simulink_integration.cpp

extern "C" {
    // Include Simulink-generated code
    #include "fome_model.h"
    #include "fome_model_private.h"
}

// Initialize Simulink model
void initSimulinkModel() {
    fome_model_initialize();
}

// Run Simulink model step (call from FOME main loop)
void runSimulinkStep() {
    // Set inputs from FOME sensors
    fome_model_U.rpm = Sensor::get(SensorType::Rpm).Value;
    fome_model_U.map = Sensor::get(SensorType::Map).Value;
    fome_model_U.tps = Sensor::get(SensorType::Tps).Value;
    fome_model_U.clt = Sensor::get(SensorType::Clt).Value;

    // Run model step
    fome_model_step();

    // Get outputs to FOME actuators
    float fuelPw = fome_model_Y.fuelPulseWidth;
    float ignTiming = fome_model_Y.ignitionTiming;

    // Apply to engine
    setInjectorPulseWidth(fuelPw);
    setIgnitionTiming(ignTiming);
}
```

### Option B: Native Simulink Target (Complex)

Create a custom Simulink target that directly integrates with FOME build system.

**fome_target.tlc** (Target Language Compiler):
```tlc
%% FOME Target
%selectfile NULL_FILE

%function FcnGenerateMainFunction() void
  %openfile mainBuf

  #include "fome_model.h"
  #include "fome_integration.h"

  // This gets called from FOME's main loop
  void simulinkUpdate() {
      %<LibCallModelStep(0)>
  }

  %closefile mainBuf
  %<SLibSetModelFileAttribute(mainFile, "SystemBody", mainBuf)>
%endfunction
```

**Effort**: 2-4 weeks for basic target, 2-3 months for full integration.

---

## 4. Implementation Roadmap

### Phase 1: Body Systems (2-3 weeks)

1. Create vehicle profile base class
2. Implement RX8Profile with all CAN messages
3. Integrate with FOME main loop
4. Add TunerStudio configuration
5. Test with RX8 dashboard

**Deliverable**: FOME with integrated RX8 body systems support.

### Phase 2: XCP Protocol (4-6 weeks)

1. Implement XCP slave core
2. Add DAQ (data acquisition) support
3. Implement calibration (download) commands
4. Create A2L file generator
5. Test with Vector CANape free version

**Deliverable**: FOME compatible with professional calibration tools.

### Phase 3: Simulink Integration (4-8 weeks)

1. Create basic Embedded Coder workflow
2. Develop integration wrapper
3. Document model structure requirements
4. Create example control models
5. Test fuel/ignition control via Simulink

**Deliverable**: FOME supporting Simulink-generated control algorithms.

---

## 5. Development Effort Summary

| Feature | Effort | Skills Required |
|---------|--------|-----------------|
| Body Systems | 2-3 weeks | C++, CAN protocols |
| XCP/CCP | 4-6 weeks | C++, XCP spec knowledge |
| Simulink Target | 4-8 weeks | Simulink, TLC, C++ |
| **Total** | **10-17 weeks** | |

---

## 6. Alternative: Fork Strategy

Instead of modifying FOME upstream, create a specialized fork:

**FOME-RX8** or **FOME-Pro**:
- Base: FOME firmware
- Add: Body systems, XCP, Simulink
- Target: Professional/OEM users

### Benefits
- Don't destabilize main FOME
- Focused development
- Can merge upstream improvements

### Repository Structure
```
fome-pro/
├── firmware/           # FOME base
├── vehicle_profiles/   # RX8, Miata, etc.
├── xcp/               # XCP protocol
├── simulink/          # Simulink integration
└── tools/
    └── a2l_generator/ # A2L file tools
```

---

## 7. Contribution to FOME Upstream

These features could be contributed back to FOME:

**High chance of acceptance**:
- Vehicle profile system (general utility)
- Basic XCP support (professional feature)

**Lower chance** (specialized):
- Full RX8 body systems (vehicle-specific)
- Simulink target (requires licenses)

### Process
1. Discuss on FOME Discord
2. Create feature branch
3. Implement with tests
4. Submit PR
5. Address review feedback

---

## Summary

Adding these features to FOME requires:

| Feature | Approach | Time | Complexity |
|---------|----------|------|------------|
| Dashboard/ABS/DSC/Immobilizer | Vehicle profile module | 2-3 weeks | Medium |
| XCP/CCP | Protocol implementation | 4-6 weeks | High |
| Simulink | Embedded Coder integration | 4-8 weeks | High |

**Total**: 10-17 weeks of development to create a professional-grade open-source ECU with:
- Body systems support (like our RX8 project)
- Professional calibration tools (like OpenECU)
- Model-based development (like Alma SPARK)

This would make FOME competitive with commercial ECUs at a fraction of the cost.
