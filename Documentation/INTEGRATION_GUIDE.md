# RX8 Arduino Modules Integration Guide

## Overview

This repository contains multiple Arduino-based modules for the Mazda RX8. Each module serves a specific purpose and can work independently or in combination with others. This guide explains how to integrate these modules into a complete RX8 electronics system.

### NEW: CAN Message Decoder Library

This repository now includes a **comprehensive CAN message decoder library** (`lib/RX8_CAN_Messages.h`) that makes it easy to decode all RX8 CAN messages:

- ✅ Decode RPM, Speed, Throttle (0x201)
- ✅ Decode Warning Lights, Temperature (0x420)
- ✅ Decode Wheel Speeds (0x4B0, 0x4B1)
- ✅ Decode Steering Angle (0x4BE)
- ✅ Decode Accelerometer (0x075)
- ✅ Encode messages for transmitting
- ✅ Helper functions (unit conversions, validation)
- ✅ Zero external dependencies

**Quick Example**:
```cpp
#include "RX8_CAN_Messages.h"
RX8_CAN_Decoder decoder;

// In your CAN receive loop:
if(rxId == 0x201) {
  decoder.decode0x201(rxBuf);
  int rpm = decoder.pcmStatus.engineRPM;
  int speed = decoder.pcmStatus.vehicleSpeed;
}
```

See `lib/README.md` for complete documentation and examples.

---

## Module Categories

### Primary ECU Modules (Mutually Exclusive)

These modules replace the factory ECU/PCM. **Choose ONE based on your powertrain**:

1. **ECU_Module** - Internal combustion engine (ICE) replacement
   - **Use Case**: Engine swaps (LS, 2JZ, K-series, etc.) or standalone engine management
   - **Hardware**: Arduino Leonardo + MCP2515
   - **Status**: Production-ready, extensively tested

2. **EV_ECU_Module** - Electric vehicle conversion
   - **Use Case**: EV conversions using Open Inverter or similar motor controllers
   - **Hardware**: Arduino Nano + MCP2515
   - **Status**: Production-ready for EV conversions

3. **Dash_Controller_Module** - Alternative dashboard controller
   - **Use Case**: Reference implementation with detailed protocol documentation
   - **Hardware**: Platform varies (PlatformIO project)
   - **Status**: Educational/reference, alternative to ECU_Module

### Complementary Display Modules (Can Run Alongside ECU)

These modules add additional display capabilities and can coexist with any ECU module:

4. **AC_Display_Module** - Factory AC display controller
   - **Use Case**: Control factory AC display, add custom pages (voltage, temps)
   - **Hardware**: Arduino Mega 2560
   - **Integration**: Can communicate with ECU via serial or CAN

5. **AC_Display_Module/ESP8266_Companion** - WiFi/Bluetooth for AC Display
   - **Use Case**: Add wireless connectivity, OBD-II data logging
   - **Hardware**: ESP8266 (NodeMCU or similar)
   - **Integration**: Serial connection to AC Display Module

6. **Aftermarket_Display_Module** - OBD2 OLED displays
   - **Use Case**: Aftermarket gauge display for 15+ engine parameters
   - **Hardware**: Arduino + MCP2515 + RGB OLED displays
   - **Integration**: Reads CAN bus alongside ECU module

### Specialized Function Modules (Standalone Add-ons)

These modules add specific functionality and operate independently:

7. **Coolant_Monitor_Module** - Dedicated temperature/pressure monitoring
   - **Use Case**: Accurate coolant monitoring (RX8's factory gauge is inadequate)
   - **Hardware**: Arduino Pro Micro + dedicated sensors + custom PCB
   - **Integration**: Standalone, uses dedicated sensors (not CAN dependent)
   - **Includes**: Firmware, PCB designs, 3D-printable enclosures

8. **Wipers_Module** - Speed-sensitive wiper control
   - **Use Case**: Add speed-dependent wiper timing to intermittent mode
   - **Hardware**: Varies (see module documentation)
   - **Integration**: Standalone, taps into wiper control signals

9. **Sim_Racing_Module** - Sim racing instrument cluster driver
   - **Use Case**: Drive RX8 cluster from PC racing games (Forza, Dirt Rally)
   - **Hardware**: Arduino + Python telemetry scripts
   - **Integration**: Standalone, PC-connected via serial

### Development Tools (Not Arduino Code)

10. **Tools/PCM_Analysis** - Ghidra-based ECU reverse engineering
    - **Use Case**: Analyze factory ECU firmware, understand ROM structure
    - **Tools**: Ghidra archives, Python scripts, ROM dumps

11. **Tools/ECU_Definitions** - Tuning definitions for ECUFlash/RomRaider
    - **Use Case**: Tune factory ECU instead of replacing it
    - **Tools**: XML definitions for 8 ECU calibrations (2004-2005)

---

## Integration Scenarios

### Scenario 1: ICE Engine Swap - Minimal Setup

**Goal**: Replace factory ECU with Arduino, maintain all vehicle systems

**Modules Used**:
- ECU_Module (primary)

**Setup**:
1. Install Arduino Leonardo with MCP2515 CAN controller
2. Connect to high-speed CAN bus (500 kbps)
3. Wire throttle pedal input (A1) and output (PWM pin 5)
4. Upload RX8_CANBUS.ino
5. Calibrate throttle pedal at startup

**Result**: Dashboard, ABS, power steering, immobilizer all functional

---

### Scenario 2: ICE Engine Swap - Full Monitoring

**Goal**: ECU replacement + comprehensive monitoring displays

**Modules Used**:
- ECU_Module (primary)
- AC_Display_Module (custom displays)
- Coolant_Monitor_Module (dedicated temp/pressure)
- Aftermarket_Display_Module (OLED gauges)

**Setup**:
1. Install ECU_Module as primary controller (Scenario 1)
2. Install AC_Display_Module for custom menu pages
3. Install Coolant_Monitor_Module with dedicated sensors (PCB + 3D-printed enclosure)
4. Install Aftermarket_Display_Module reading CAN bus for additional parameters

**Communication**:
- ECU_Module transmits on CAN bus (0x201, 0x420, etc.)
- AC_Display_Module can optionally read CAN or use serial connection
- Aftermarket_Display_Module reads CAN bus passively
- Coolant_Monitor_Module uses dedicated sensors (standalone)

**Result**: Full dashboard + AC control + accurate coolant monitoring + aftermarket gauge cluster

---

### Scenario 3: Electric Vehicle Conversion

**Goal**: Convert RX8 to electric power while maintaining factory systems

**Modules Used**:
- EV_ECU_Module (primary)
- AC_Display_Module (display motor temps as "coolant")
- Aftermarket_Display_Module (battery/motor stats)

**Setup**:
1. Install EV_ECU_Module (Arduino Nano + MCP2515)
2. Connect to Open Inverter CAN bus (read motor data)
3. Connect to RX8 CAN bus (send dashboard data)
4. Configure motor RPM → engine RPM mapping
5. Install AC_Display_Module with custom page for battery voltage
6. Install Aftermarket_Display_Module for power/efficiency monitoring

**Data Flow**:
- Open Inverter → EV_ECU_Module (motor RPM, temp, power)
- EV_ECU_Module → RX8 CAN bus (mapped as engine data)
- AC_Display_Module reads CAN or serial for battery voltage
- Aftermarket_Display_Module reads CAN for kW, efficiency, etc.

**Result**: Factory dashboard shows "engine RPM" (motor RPM), temperature (inverter temp), plus additional EV-specific displays

---

### Scenario 4: Reference Development Platform

**Goal**: Develop and validate CAN protocol implementations

**Modules Used**:
- ECU_Module (primary implementation)
- Dash_Controller_Module (reference implementation)
- Documentation/rx8_can_database.dbc (CAN definitions)
- Documentation/CAN_PID_Reference.md (protocol documentation)

**Setup**:
1. Study Dash_Controller_Module for detailed bit-level protocol docs
2. Cross-reference ECU_Module implementation
3. Validate against DBC file using SavvyCAN or Wireshark
4. Test implementations on bench with CAN analyzer

**Result**: Validated CAN protocol implementation, comprehensive documentation for future development

---

### Scenario 5: Sim Racing Setup

**Goal**: Use real RX8 cluster as sim racing display

**Modules Used**:
- Sim_Racing_Module (standalone)

**Setup**:
1. Remove RX8 cluster from vehicle (or use spare cluster)
2. Wire Arduino to cluster connector
3. Install Python telemetry scripts on PC
4. Configure for Forza Horizon 5 or Dirt Rally 2.0
5. Connect Arduino to PC via USB

**Result**: RX8 instrument cluster displays live data from racing games

---

### Scenario 6: Track Day Telemetry

**Goal**: Log CAN data for racing analysis

**Modules Used**:
- ECU_Module (primary)
- AC_Display_Module + ESP8266_Companion (data logging)
- Aftermarket_Display_Module (real-time display)

**Setup**:
1. ECU_Module provides base CAN data
2. ESP8266_Companion logs CAN data to SD card or transmits via WiFi
3. Aftermarket_Display_Module shows real-time lap data
4. Use CAN_PID_Reference.md to configure RaceChrono or similar

**Data Logged**:
- Engine RPM, speed, throttle position (0x201)
- Coolant temp, warning lights (0x420)
- Wheel speeds (0x4B1)
- Steering angle (0x4BE)
- Accelerometer data (0x075)

**Result**: Comprehensive track telemetry for analysis

---

## Communication Methods

### 1. CAN Bus (Primary Method)

**Best For**: Real-time vehicle data sharing between modules

**How It Works**:
- All modules connect to same CAN bus (500 kbps high-speed CAN)
- ECU module transmits critical messages (0x201, 0x420, etc.)
- Other modules read CAN bus passively
- No conflicts if modules only read (don't transmit same message IDs)

**Example**:
```cpp
// ECU_Module transmits
CAN0.sendMsgBuf(0x201, 0, 8, send201);  // PCM status

// Aftermarket_Display_Module receives
if(ID == 0x201) {
  engineRPM = (buf[0] * 256 + buf[1]) / 3.85;
}
```

**Wiring**:
- Connect all MCP2515 CAN_H pins together
- Connect all CAN_L pins together
- Add 120Ω termination resistors at each end of bus

---

### 2. Serial Communication (Secondary Method)

**Best For**: Module-to-module communication without CAN overhead

**How It Works**:
- Connect TX of one Arduino to RX of another
- Use simple text-based protocol
- Lower overhead than CAN for point-to-point

**Example**:
```cpp
// ECU_Module (Leonardo) sends data
Serial1.print("$RPM:");
Serial1.print(engineRPM);
Serial1.println();

// AC_Display_Module (Mega) receives
if(Serial3.available()) {
  String cmd = Serial3.readStringUntil('\n');
  if(cmd.startsWith("$RPM:")) {
    int rpm = cmd.substring(5).toInt();
  }
}
```

**Protocol Format**: `$COMMAND:VALUE\n`

**Wiring**:
- Leonardo Serial1 (TX1/RX0) ↔ Mega Serial3 (TX3/RX3)
- Common ground between Arduinos
- 5V logic level compatible (both Leonardo and Mega)

---

### 3. I2C (For Display Modules)

**Best For**: Connecting multiple displays to one controller

**How It Works**:
- Used internally by AC_Display_Module and Coolant_Monitor_Module
- Multiplexing required for displays with same I2C address
- Short cable runs (<12 inches recommended)

**Example** (from Coolant_Monitor_Module):
```cpp
// Multiplex between three OLED displays
digitalWrite(DISPLAY_SELECT_1, state);
display1.print(temperature);
```

---

### 4. Standalone Operation

**Best For**: Modules that don't need vehicle data

**Modules**:
- Coolant_Monitor_Module (uses dedicated sensors)
- Wipers_Module (taps wiper control signals directly)
- Sim_Racing_Module (PC-connected, not vehicle-integrated)

**Advantage**: No integration complexity, works independently

---

## Hardware Compatibility Matrix

| Module | Microcontroller | CAN Controller | CAN Speed | Power | Special Requirements |
|--------|----------------|----------------|-----------|-------|---------------------|
| ECU_Module | Leonardo | MCP2515 | 500 kbps | 12V vehicle | Throttle pedal I/O |
| EV_ECU_Module | Nano | MCP2515 | 500 kbps | 12V vehicle | Open Inverter CAN |
| Dash_Controller_Module | Varies | MCP2515 | 500 kbps | 12V vehicle | PlatformIO build |
| AC_Display_Module | Mega 2560 | None | N/A | 12V vehicle | Factory AC display |
| ESP8266_Companion | ESP8266 | None | N/A | 5V regulated | WiFi/BT antenna |
| Aftermarket_Display_Module | Uno/Nano | MCP2515 | 500 kbps | 12V vehicle | RGB OLED displays |
| Coolant_Monitor_Module | Pro Micro | None | N/A | 12V vehicle | Custom PCB, sensors |
| Wipers_Module | Varies | None | N/A | 12V vehicle | Wiper control access |
| Sim_Racing_Module | Leonardo/Uno | MCP2515 | 500 kbps | USB/12V | PC with Python |

---

## CAN Bus Topology

### Recommended Wiring

```
[ECU_Module] ─────┬───── CAN_H (twisted pair) ─────┬───── [Dashboard]
    (120Ω)        │                                │        (120Ω)
                  ├───── [Aftermarket_Display]     │
                  │                                │
                  └───── [AC_Display (optional)]   │
                                                   │
                              CAN_L (twisted pair) ─┘
```

**Best Practices**:
- Use twisted pair wire for CAN_H/CAN_L (minimum 24 AWG)
- 120Ω termination resistors at both ends ONLY
- Keep stub lengths short (<1 meter from main bus)
- Star topologies can cause reflections (avoid if possible)
- Maximum bus length: ~40 meters at 500 kbps

---

## Power Distribution

### 12V Vehicle Power

All modules except ESP8266_Companion can run directly from 12V vehicle power:

```
[12V Battery] ──┬── [ECU_Module] (onboard 5V regulator)
                ├── [AC_Display_Module] (onboard 5V regulator)
                ├── [Aftermarket_Display] (onboard 5V regulator)
                └── [Coolant_Monitor] (DC-DC converter on PCB)
```

**Recommendations**:
- Fuse each module separately (1-2A fuses)
- Use switched 12V (ignition-controlled) for most modules
- Coolant_Monitor_Module can use parking light detection

### 5V Regulated Power

ESP8266_Companion needs regulated 5V:

```
[12V Vehicle] ──[Buck Converter]── [ESP8266] (3.3V onboard regulator)
                                └── [AC_Display_Module] (5V to Serial3)
```

---

## Data Sharing Examples

### Example 1: Share Engine RPM via CAN

**ECU_Module sends**:
```cpp
// In updatePCM()
int tempEngineRPM = engineRPM * 3.85;
send201[0] = highByte(tempEngineRPM);
send201[1] = lowByte(tempEngineRPM);
CAN0.sendMsgBuf(0x201, 0, 8, send201);
```

**Aftermarket_Display_Module receives**:
```cpp
if(ID == 0x201) {
  int rawRPM = (buf[0] * 256) + buf[1];
  engineRPM = rawRPM / 3.85;
  updateOLED(engineRPM);
}
```

---

### Example 2: Share Battery Voltage via Serial

**AC_Display_Module sends**:
```cpp
float voltage = analogRead(A4) * VOLTAGE_CONVERSION_FACTOR;
Serial3.print("$VBAT:");
Serial3.println(voltage, 2);  // 2 decimal places
```

**ESP8266_Companion receives**:
```cpp
if(Serial.available()) {
  String data = Serial.readStringUntil('\n');
  if(data.startsWith("$VBAT:")) {
    float voltage = data.substring(6).toFloat();
    logToSD(voltage);
  }
}
```

---

### Example 3: Share Temperature from Dedicated Sensor

**Coolant_Monitor_Module** (standalone):
- Reads thermistor using Steinhart-Hart equation
- Displays on OLED directly
- No sharing needed (dedicated display)

**Alternative**: Add serial output for logging:
```cpp
Serial.print("$COOLANT_TEMP:");
Serial.println(tempCelsius);
```

Then ESP8266 or AC_Display can log/display this data.

---

## Testing and Validation

### Bench Testing (No Vehicle)

1. **CAN Bus Simulation**:
   - Use two Arduinos: one sends, one receives
   - Verify message timing (100ms cycle)
   - Check message format with logic analyzer

2. **Serial Communication**:
   - Test with Arduino IDE Serial Monitor
   - Verify protocol format (`$CMD:VAL\n`)
   - Check baud rate compatibility (115200 recommended)

3. **Power Supply Testing**:
   - Bench power supply 12V (simulate vehicle)
   - Measure current draw for each module
   - Verify voltage regulation (should be stable 5V to Arduino)

---

### Vehicle Testing (Staged Approach)

**Stage 1: ECU Module Only**
- Install ECU module
- Verify dashboard functions (RPM, speed, temp)
- Check warning lights
- Test immobilizer bypass
- Confirm ABS/power steering operation

**Stage 2: Add Display Modules**
- Install AC_Display_Module or Aftermarket_Display_Module
- Verify CAN bus not disrupted
- Check data accuracy (compare to ECU readings)

**Stage 3: Add Specialized Modules**
- Install Coolant_Monitor_Module
- Verify dedicated sensors read correctly
- Compare to ECU temperature readings

**Stage 4: Integration Testing**
- Test all modules together
- Monitor CAN bus for conflicts
- Verify no interference between modules
- Check current draw (ensure alternator can supply)

---

## Troubleshooting

### CAN Bus Issues

**Symptom**: No CAN messages received
- Check wiring (CAN_H/CAN_L not swapped)
- Verify 120Ω termination at both ends
- Check MCP2515 CS and INT pin connections
- Confirm CAN speed (500 kbps)

**Symptom**: Intermittent CAN messages
- Check for poor connections
- Verify twisted pair wiring
- Reduce stub lengths
- Check ground connections

**Symptom**: Dashboard shows errors despite ECU running
- Verify message timing (100ms cycle)
- Check message format (byte order)
- Confirm immobilizer responses (0x047/0x041)
- Verify ABS messages (0x620, 0x630)

---

### Serial Communication Issues

**Symptom**: No serial data received
- Check TX/RX connections (crossed correctly)
- Verify baud rate match (both ends 115200)
- Confirm common ground
- Check voltage levels (5V logic)

**Symptom**: Garbled serial data
- Baud rate mismatch
- Electrical noise (add capacitors)
- Cable too long (keep <3 feet for 115200 baud)

---

### Power Issues

**Symptom**: Module resets randomly
- Insufficient current capacity
- Voltage drops during cranking
- Add bulk capacitor (1000µF) near module
- Use thicker power wires

**Symptom**: Module won't power on
- Check fuse
- Verify voltage at module (should be 11-14V)
- Test 5V regulator output
- Check for shorts

---

## Module Modification Guidelines

### When Modifying for Integration

1. **Preserve CAN Message IDs**:
   - Don't change existing message IDs (0x201, 0x420, etc.)
   - Add new message IDs in unused range (0x500-0x5FF available)

2. **Add Serial Communication**:
   ```cpp
   // Add to setup()
   Serial1.begin(115200);  // Leonardo Serial1
   // or
   Serial3.begin(115200);  // Mega Serial3

   // Add to loop()
   if(Serial1.available()) {
     // Parse incoming commands
   }
   ```

3. **Share Data Proactively**:
   ```cpp
   // In main loop
   if(millis() - lastSerialSend >= 100) {  // 10 Hz update
     Serial1.print("$DATA:");
     Serial1.println(value);
     lastSerialSend = millis();
   }
   ```

4. **Avoid CAN Conflicts**:
   - Only one module should transmit on each message ID
   - Multiple modules can receive (read-only)
   - If adding new CAN transmit, document in CLAUDE.md

---

## Best Practices

### Code Organization

- Keep module code in separate directories
- Document pin assignments in module README
- Use consistent variable naming across modules
- Add integration examples in documentation

### Hardware Installation

- Label all wires clearly
- Use automotive-grade connectors
- Weatherproof outdoor installations
- Route CAN bus away from high-current wires

### Testing

- Test each module individually first
- Add modules one at a time to vehicle
- Keep CAN analyzer for debugging
- Document any vehicle-specific variations

---

## Future Integration Opportunities

### Planned Improvements

1. **Unified Serial Protocol**:
   - Standardized command/response format
   - CRC checksums for reliability
   - Binary protocol for efficiency

2. **CAN Message Library**:
   - Shared header file for all modules
   - Consistent encoding/decoding functions
   - Centralized documentation

3. **Web-Based Configuration**:
   - ESP8266_Companion hosts web interface
   - Configure all modules wirelessly
   - Live data streaming

4. **Data Logging**:
   - Centralized SD card logging
   - Export to CSV for analysis
   - Integration with RaceChrono

---

## Related Documentation

- **CLAUDE.md** - Comprehensive AI assistant guide for codebase
- **RX8_ECOSYSTEM.md** - Integration analysis for 15 repositories
- **Documentation/CAN_PID_Reference.md** - Complete CAN protocol reference
- **Documentation/rx8_can_database.dbc** - CAN signal definitions
- **docs/related_projects.md** - Categorized project catalog

---

## Support and Community

For questions and integration help:
- Review module-specific README files
- Check Documentation/CAN_PID_Reference.md for protocol details
- Reference RX8_ECOSYSTEM.md for project relationships
- Contact module authors (see individual README files)

---

*Last Updated: 2025-11-15*
*Repository: MazdaRX8Arduino*
