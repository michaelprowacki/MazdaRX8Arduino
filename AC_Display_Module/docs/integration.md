# AC Display Module Integration Guide

## Overview

This document describes how to integrate the AC Display Module with the main RX8 CAN Bus ECU replacement project.

---

## Architecture

The MazdaRX8Arduino project now consists of two main components:

1. **RX8_CANBUS.ino** - ECU/PCM replacement (Arduino Leonardo)
2. **AC_Display_Module/** - AC display controller (Arduino Mega 2560)

These modules can operate independently or communicate with each other for enhanced functionality.

---

## Integration Scenarios

### Scenario 1: Independent Operation (Recommended for Beginners)

**Description**: Both modules operate independently without communication.

**Advantages**:
- Simple setup
- No additional wiring between modules
- Isolated fault domains
- Each module can be tested separately

**Configuration**:
- Program Arduino Leonardo with RX8_CANBUS.ino
- Program Arduino Mega with AC_Display_Module code
- Connect each to their respective hardware
- Both share vehicle power and ground only

**Use Case**:
- Engine swap with custom motor controller
- Keep factory AC display functional
- No need for integrated data display

---

### Scenario 2: Serial Communication Bridge

**Description**: Modules communicate via Serial connection to share data.

**Advantages**:
- Share sensor data between modules
- Display ECU data on AC display
- Coordinated warning lights
- Simple protocol implementation

**Hardware Connection**:
```
Arduino Leonardo          Arduino Mega 2560
(RX8_CANBUS)             (AC_Display)
-----------------         -----------------
TX (Pin 1)    --------->  RX3 (Pin 15)
RX (Pin 0)    <---------  TX3 (Pin 14)
GND           <---------> GND
```

**Data Shared**:
- Engine RPM → Display on AC unit
- Vehicle speed → Speed-dependent functions
- Engine temperature → Temperature display
- Battery voltage → Voltage monitoring
- Warning states → Coordinated alerts

**Protocol Example**:
```
Format: $CMD:VALUE\n

From ECU to Display:
$RPM:3500\n         // Engine RPM
$SPD:65\n           // Vehicle speed (mph)
$TMP:145\n          // Engine temp
$BAT:13.8\n         // Battery voltage
$WRN:0\n            // Warning flags

From Display to ECU:
$ACK\n              // Acknowledgment
$REQ:RPM\n          // Request RPM update
```

**Implementation**:

In `RX8_CANBUS.ino`:
```cpp
// In loop(), send data every 500ms
if (millis() - lastSerialUpdate >= 500) {
    Serial.print("$RPM:");
    Serial.println(engineRPM);

    Serial.print("$SPD:");
    Serial.println(vehicleSpeed);

    Serial.print("$TMP:");
    Serial.println(engTemp);

    lastSerialUpdate = millis();
}
```

In `AC_Display_Module/src/main.cpp`:
```cpp
// In loop(), read serial data
if (Serial3.available()) {
    String data = Serial3.readStringUntil('\n');

    if (data.startsWith("$RPM:")) {
        int rpm = data.substring(5).toInt();
        // Display RPM on custom menu page
    }
    // ... handle other commands
}
```

---

### Scenario 3: CAN Bus Integration (Advanced)

**Description**: Both modules on the same CAN bus, sharing all vehicle data.

**Advantages**:
- Single CAN bus network
- All modules see all data
- Standard automotive protocol
- Scalable to additional modules

**Hardware Connection**:
```
         CAN Bus (500 kbps)
              |
    ----------+----------
    |                   |
Leonardo             Mega 2560
MCP2515              MCP2515 (add second)
RX8_CANBUS           AC_Display
```

**Requirements**:
- Add MCP2515 CAN controller to Arduino Mega
- Both modules use same CAN speed (500 kbps)
- Unique CAN IDs for new messages

**CAN Message IDs**:

Existing (from RX8_CANBUS.ino):
- 0x201 - PCM Status (RPM, Speed, Throttle)
- 0x420 - Warning lights
- 0x4B1 - Wheel speeds (received)

New (for AC Display):
- 0x500 - AC Display status
- 0x501 - AC button presses
- 0x502 - AC menu selection

**Implementation**:

In `AC_Display_Module/src/main.cpp`:
```cpp
#include <mcp_can.h>

MCP_CAN CAN1(10);  // CS pin 10 for Mega

void setup() {
    if (CAN1.begin(CAN_500KBPS) == CAN_OK) {
        Serial.println("CAN initialized");
    }
}

void loop() {
    // Read CAN messages from ECU
    if (CAN_MSGAVAIL == CAN1.checkReceive()) {
        unsigned long id;
        unsigned char len;
        unsigned char buf[8];

        CAN1.readMsgBufID(&id, &len, buf);

        if (id == 0x201) {  // PCM status
            int rpm = ((buf[0] << 8) | buf[1]) / 3.85;
            int speed = (((buf[4] << 8) | buf[5]) - 10000) / 100;
            // Update display
        }
    }

    // Send AC status
    byte acStatus[8] = {fanSpeed, temp, mode, ...};
    CAN1.sendMsgBuf(0x500, 0, 8, acStatus);
}
```

---

## Physical Installation

### Power Distribution

Both modules require 12V power from vehicle:

```
Vehicle Battery 12V
        |
        +--- Fuse 10A --- Arduino Leonardo (5V regulator onboard)
        |
        +--- Fuse 10A --- Arduino Mega 2560 (5V regulator onboard)
        |
        +--- Common Ground to chassis
```

**Important**:
- Use separate fuses for each module
- Connect grounds to same chassis point
- Use switched 12V (ignition-on) to prevent battery drain

### Mounting Locations

**Arduino Leonardo (ECU Replacement)**:
- Mount in original PCM location or nearby
- Weather-sealed enclosure recommended
- Good access for CAN bus connections

**Arduino Mega (AC Display)**:
- Mount behind dashboard near AC controls
- Short cable run to display unit
- Access to Serial3 for ESP8266 (optional)

---

## Software Configuration

### Option 1: Separate Projects (Simple)

Keep as separate projects:
- `RX8_CANBUS.ino` - Arduino IDE project
- `AC_Display_Module/` - PlatformIO project

### Option 2: Unified Project (Advanced)

Merge into single repository:
```
MazdaRX8Arduino/
├── ECU_Module/
│   └── RX8_CANBUS.ino
├── AC_Display_Module/
│   ├── platformio.ini
│   └── src/main.cpp
└── shared/
    ├── can_protocol.h      # Shared CAN message definitions
    └── serial_protocol.h   # Shared serial protocol
```

---

## Testing Procedure

### Phase 1: Bench Testing

1. **Test ECU Module Alone**
   - Upload RX8_CANBUS.ino
   - Verify CAN messages with analyzer
   - Test throttle input/output

2. **Test AC Display Module Alone**
   - Upload AC Display code
   - Test button inputs
   - Verify display output
   - Check RTC functionality

3. **Test Communication** (if using Serial bridge)
   - Connect TX/RX between modules
   - Send test data from ECU
   - Verify reception on AC Display
   - Check bidirectional communication

### Phase 2: Vehicle Testing

1. **Install ECU Module**
   - Connect CAN bus
   - Connect throttle
   - Power on, verify no warning lights
   - Test immobilizer bypass

2. **Install AC Display Module**
   - Connect to AC display hardware
   - Connect button panel
   - Power on, verify display
   - Test all buttons

3. **Verify Integration**
   - Check data sharing (RPM, speed, etc.)
   - Test coordinated warnings
   - Verify no CAN bus conflicts
   - Test under various driving conditions

---

## Troubleshooting

### Issue: Modules can't communicate via Serial

**Symptoms**: No data received, garbled data

**Solutions**:
- Check TX/RX connections (crossover required)
- Verify baud rates match (115200)
- Check common ground connection
- Test with loopback (TX to RX on same device)

### Issue: CAN bus conflicts

**Symptoms**: Messages corrupted, bus errors

**Solutions**:
- Verify both modules use 500 kbps
- Check CAN_H and CAN_L connections
- Ensure 120Ω termination resistors at bus ends
- Use unique message IDs (no conflicts)

### Issue: Display shows wrong data

**Symptoms**: RPM incorrect, speed wrong

**Solutions**:
- Check data parsing code
- Verify conversion factors match
- Test with known values
- Add debug output to verify received data

---

## Future Enhancements

### Possible Additions

1. **OBD-II Integration**
   - Read engine codes
   - Display on AC unit
   - Clear codes via button sequence

2. **Data Logging**
   - Log to SD card
   - Track performance metrics
   - Export for analysis

3. **Bluetooth Interface**
   - Connect to smartphone app
   - Remote monitoring
   - Configuration via app

4. **Additional Sensors**
   - Wideband O2 sensor
   - EGT (exhaust gas temp)
   - Oil pressure/temp
   - Boost pressure

---

## Support

For integration issues:
- Check main project CLAUDE.md
- Review this integration guide
- Test each module independently first
- Use Serial debugging extensively

---

*Last Updated: 2025-11-15*
*Part of MazdaRX8Arduino Project*
