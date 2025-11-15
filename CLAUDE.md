# CLAUDE.md - AI Assistant Guide for Mazda RX8 Arduino Project

## Project Overview

This is an **Arduino-based ECU/PCM replacement** for the Mazda Mark 1 RX8. The project allows removal of the factory PCM while maintaining all other vehicle systems through CAN Bus emulation.

**Primary Purpose**: Replace the factory Engine Control Module (ECU/PCM) while keeping other vehicle systems functional (ABS, DSC, traction control, power steering, dashboard, immobilizer).

**Author**: David Blackhurst (dave@blackhurst.co.uk)
**Hardware**: Arduino Leonardo
**CAN Bus Speed**: 500 kbps (High Speed CAN)

**⚠️ SAFETY WARNING**: This code controls critical vehicle systems. Any modifications must be thoroughly tested. This is automotive safety-critical code that can affect vehicle operation.

---

## Repository Structure

```
MazdaRX8Arduino/
├── RX8_CANBUS.ino              # Main Arduino sketch (PRIMARY CODE FILE)
├── README.md                    # Project introduction and donation info
├── CLAUDE.md                    # This file - AI assistant guide
│
├── Documentation/
│   ├── 08_Steering.pdf         # Steering system documentation
│   ├── 13Electrical.pdf        # Electrical system documentation
│   ├── Engine-Manual.pdf       # Engine manual
│   ├── RX8 CanBus and Throttle Explained.pdf  # CAN Bus technical details
│   └── control system.jpg      # Control system diagram
│
└── Data Files/
    ├── CanBus Raw Output.xlsx  # Raw CAN Bus data capture
    └── RX8CanBus20-05-2020.xlsx # Processed CAN Bus data
```

---

## Hardware Architecture

### Components
- **Microcontroller**: Arduino Leonardo (ATmega32U4)
- **CAN Controller**: MCP2515 (library: mcp_can.h)
- **CS Pin**: Digital pin 17
- **CAN Interrupt**: Digital pin 2
- **Throttle Input**: Analog pin A1 (1.7V - 4.0V range)
- **Throttle Output**: PWM pin 5
- **Status LEDs**: Digital pins 7 and 8

### Pin Configuration
```cpp
#define CANint 2        // CAN interrupt pin
#define LED2 8          // Status LED 2
#define LED3 7          // Status LED 3
int analogPin = A1;     // Throttle pedal input
int outputPin = 5;      // Throttle controller output
pinMode(23, OUTPUT);    // Additional control pin
```

---

## CAN Bus Architecture

### Communication Protocol
- **Bus Speed**: 500 kbps (CAN_500KBPS)
- **Message Format**: Standard CAN frames (not extended)
- **Update Frequency**: 100ms (10 Hz) for status messages

### Critical CAN Message IDs (Hexadecimal)

#### **Transmitted Messages** (Arduino → Vehicle)
| ID (Hex) | ID (Dec) | Purpose | Length | Update Rate |
|----------|----------|---------|--------|-------------|
| 0x201 | 513 | PCM Status (RPM, Speed, Throttle) | 8 | 100ms |
| 0x203 | 515 | Traction Control Data | 7 | 100ms |
| 0x212 | 530 | DSC/ABS Status | 7 | 100ms* |
| 0x215 | 533 | PCM Status Supplement | 8 | 100ms |
| 0x231 | 561 | PCM Status Supplement | 5 | 100ms |
| 0x240 | 576 | PCM Status Supplement | 8 | 100ms |
| 0x420 | 1056 | MIL/Warning Lights/Engine Temp | 7 | 100ms |
| 0x620 | 1568 | ABS System Data | 7 | 100ms |
| 0x630 | 1584 | ABS Config (AT/MT, Wheel Size) | 8 | 100ms |
| 0x650 | 1616 | ABS Supplement | 1 | 100ms |
| 0x041 | 65 | Immobilizer Response | 8 | On request |

*Note: 0x212 (DSC) is commented out by default - see line 268-271

#### **Received Messages** (Vehicle → Arduino)
| ID (Hex) | ID (Dec) | Purpose | Processing |
|----------|----------|---------|------------|
| 0x047 | 71 | Keyless/Immobilizer Request | Immediate response required |
| 0x4B1 | 1201 | Wheel Speed Data | Used for vehicle speed calculation |

### Message Structure Details

#### 0x201 - PCM Status (Most Critical)
```cpp
byte send201[8] = {0, 0, 255, 255, 0, 0, 0, 255};
// Bytes 0-1: Engine RPM (RPM * 3.85)
// Bytes 2-3: 0xFF 0xFF (fixed)
// Bytes 4-5: Vehicle Speed ((mph * 100) + 10000)
// Byte 6: Throttle Pedal (0.5% increments, max 200)
// Byte 7: 0xFF (fixed)
```

#### 0x420 - MIL/Warning Lights
```cpp
byte send420[7] = {0, 0, 0, 0, 0, 0, 0};
// Byte 0: Engine temperature (145 = normal)
// Byte 1: Odometer data
// Byte 4: Oil pressure (1 = OK)
// Byte 5: Bit 6 = Check Engine MIL, Bit 7 = Check Engine Backlight
// Byte 6: Bit 1 = Low Water, Bit 6 = Bat Charge, Bit 7 = Oil Pressure
```

#### 0x620 - ABS System Data
```cpp
byte send620[7] = {0,0,0,0,16,0,4};
// Critical for ABS light
// Byte 7 varies by car (2, 3, or 4) - may need adjustment
```

#### 0x630 - ABS Configuration
```cpp
byte send630[8] = {8,0,0,0,0,0,106,106};
// Byte 0: AT/MT indicator (8 = specific config)
// Bytes 6-7: Wheel size parameters (106, 106)
```

---

## Code Architecture

### Main Components

#### 1. **Setup Phase** (`setup()` and `setDefaults()`)
- Initialize serial communication (115200 baud)
- Configure GPIO pins
- Initialize MCP2515 CAN controller
- Calibrate throttle pedal zero position (500ms delay)
- Set default values for all vehicle systems

#### 2. **Main Loop** (`loop()`)
- **Timing**: 100ms cycle for CAN message transmission
- **CAN Reception**: Continuous polling for incoming messages
- **Throttle Processing**: Real-time analog input processing
- **Safety Checks**: Voltage range validation

#### 3. **Key Functions**

```cpp
void setDefaults()      // Initialize all vehicle system states
void updateMIL()        // Update warning light status (0x420)
void updatePCM()        // Update RPM, speed, throttle (0x201)
void updateDSC()        // Update ABS/DSC status (0x212) - optional
void sendOnTenth()      // Transmit all CAN messages every 100ms
```

---

## Key Variables and States

### PCM Control Variables
```cpp
int engineRPM;          // Engine RPM (actual value, not encoded)
int vehicleSpeed;       // Speed in MPH
byte throttlePedal;     // Throttle position (0-100%)
byte engTemp;           // Engine temperature (145 = normal)
```

### Warning Light Control
```cpp
bool checkEngineMIL;    // Check engine light
bool checkEngineBL;     // Check engine backlight
bool lowWaterMIL;       // Low coolant warning
bool batChargeMIL;      // Battery charge warning
bool oilPressureMIL;    // Oil pressure warning
bool oilPressure;       // Oil pressure status (1 = OK)
```

### ABS/DSC Control (Optional - see line 268)
```cpp
bool dscOff;            // DSC system off indicator
bool absMIL;            // ABS warning light
bool brakeFailMIL;      // Brake failure warning
bool etcActiveBL;       // Electronic throttle control active
bool etcDisabled;       // Electronic throttle control disabled
```

### Wheel Speed Variables
```cpp
int frontLeft;          // Front left wheel speed
int frontRight;         // Front right wheel speed
int rearLeft;           // Rear left wheel speed
int rearRight;          // Rear right wheel speed
```

---

## Critical Algorithms

### 1. Throttle Pedal Calibration
```cpp
// Voltage range: 1.7V - 4.0V from RX8 pedal
// ADC range: ~341 (1.7V) to 803 (4.0V)
// Output range: 0 - 960 (0 - 4.5V for controller)

lowPedal = analogRead(analogPin) - 40;  // Calibrate at startup
highPedal = 803;
convertThrottle = 960 / (highPedal - lowPedal);

// Runtime conversion:
base = val - lowPedal;
output = base * convertThrottle;
output = constrain(output, 0, 960);
throttlePedal = (100 / 960) * output;
analogWrite(outputPin, output/4);  // PWM output (8-bit)
```

### 2. Vehicle Speed Calculation
```cpp
// Read from CAN ID 0x4B1 (1201 decimal)
frontLeft = (buf[0] * 256) + buf[1] - 10000;
frontRight = (buf[2] * 256) + buf[3] - 10000;

// Safety check: detect wheel speed mismatch
if (abs(frontLeft - frontRight) > 500) {  // 5 kph difference
    checkEngineMIL = 1;  // Light up warning
    vehicleSpeed = 0;    // Safe state
} else {
    vehicleSpeed = ((frontLeft + frontRight) / 2) / 100;
}
```

### 3. Immobilizer Protocol
```cpp
// Two-part handshake on CAN ID 0x47 (71 decimal)
if (buf[1] == 127 && buf[2] == 2) {
    send41a = {7,12,48,242,23,0,0,0};
    CAN0.sendMsgBuf(0x041, 0, 8, send41a);
}
if (buf[1] == 92 && buf[2] == 244) {
    send41b = {129,127,0,0,0,0,0,0};
    CAN0.sendMsgBuf(0x041, 0, 8, send41b);
}
```

---

## Development Workflow

### Prerequisites
- **Arduino IDE** (or PlatformIO)
- **MCP_CAN Library** (Seed Studio or Cory J. Fowler fork)
- **Arduino Leonardo board** (ATmega32U4)
- **MCP2515 CAN Bus module**

### Building and Uploading
1. Install mcp_can library in Arduino IDE
2. Open `RX8_CANBUS.ino`
3. Select board: Tools → Board → Arduino Leonardo
4. Select correct COM port
5. Verify/Compile (Ctrl+R)
6. Upload (Ctrl+U)

### Serial Debugging
- **Baud Rate**: 115200
- **Output**: Wheel speed data and CAN ID 530 messages
- **Startup Messages**: Confirm CAN initialization

---

## Key Conventions for AI Assistants

### Code Modification Rules

1. **SAFETY FIRST**: Never modify safety-critical code without explicit understanding
   - Throttle safety limits (lines 329-331)
   - Wheel speed mismatch detection (lines 314-319)
   - CAN message timing (100ms cycle)

2. **Preserve Existing Behavior**: This code is tuned for a specific vehicle
   - CAN message byte arrays are vehicle-specific
   - Timing is critical (100ms cycle in `sendOnTenth()`)
   - Some values vary between RX8s (e.g., `send620[6]`)

3. **Comment Thoroughly**: Explain WHY, not just WHAT
   - CAN message purposes
   - Magic numbers (e.g., why 3.85 for RPM, why 10000 offset for speed)
   - Vehicle-specific variations

4. **Maintain Compatibility**
   - Keep Arduino Leonardo compatibility
   - Preserve MCP_CAN library usage
   - Don't break existing CAN message structure

### Variable Naming Convention
- **Lowercase camelCase** for variables: `engineRPM`, `vehicleSpeed`
- **Descriptive names** for CAN arrays: `send201`, `send420` (hex IDs)
- **Boolean flags** use clear states: `checkEngineMIL`, `oilPressure`

### Magic Numbers to Understand
```cpp
3.85        // RPM encoding multiplier for CAN (line 208)
10000       // Speed offset for CAN encoding (line 209)
200         // Throttle pedal scaling factor (line 217)
500         // Wheel speed difference threshold (5 kph) (line 314)
100         // Main loop timing in milliseconds (line 276)
145         // Normal engine temperature value (line 129)
960         // Max throttle output value (line 161)
```

---

## Testing Guidance

### Bench Testing (No Vehicle)
```cpp
// Add to loop() for testing:
Serial.print("RPM: "); Serial.println(engineRPM);
Serial.print("Speed: "); Serial.println(vehicleSpeed);
Serial.print("Throttle: "); Serial.println(throttlePedal);
```

### CAN Bus Monitoring
- Use CAN bus analyzer or second Arduino with CAN sniffer
- Monitor transmitted message IDs: 0x201, 0x420, 0x620, 0x630
- Verify 100ms timing with oscilloscope or logic analyzer

### Safety Checks Before Vehicle Testing
- [ ] Throttle pedal reads 0% at rest
- [ ] Throttle pedal reaches 100% at full depression
- [ ] Engine temperature set to safe value (145)
- [ ] All warning lights configured correctly
- [ ] CAN bus initialized successfully
- [ ] Immobilizer responses tested

---

## Common Modifications

### 1. Adjust RPM Display
```cpp
// In updatePCM() - line 208
int tempEngineRPM = engineRPM * 3.85;  // Change multiplier if needed
```

### 2. Enable/Disable Warning Lights
```cpp
// In setDefaults() - lines 128-136
checkEngineMIL = 0;   // 0 = off, 1 = on
lowWaterMIL = 0;      // Modify as needed
```

### 3. Enable ABS/DSC Control
```cpp
// Uncomment lines 268-271 in sendOnTenth()
updateDSC();
CAN0.sendMsgBuf(0x212, 0, 7, send212);
```

### 4. Change Throttle Calibration
```cpp
// In setDefaults() - lines 152-163
lowPedal = analogRead(analogPin) - 40;  // Adjust offset
highPedal = 803;  // Adjust max value
```

### 5. Add Additional CAN Monitoring
```cpp
// In loop() - add new ID check:
if(ID == YOUR_ID_HERE) {
    // Process message
}
```

---

## Known Issues and Variations

### Vehicle-Specific Adjustments Needed

1. **0x620 Byte 7 Variation** (line 92)
   - Some RX8s need value `2`, others `3` or `4`
   - Symptom: ABS light stays on
   - Solution: Try different values until ABS light clears

2. **Throttle Pedal Voltage Range**
   - Standard: 1.64V - 4.04V
   - Code uses safe range: 1.7V - 4.0V
   - May need adjustment for different pedals

3. **Wheel Speed Sensor Variations**
   - Rear wheel speeds ignored due to wheelspin issues
   - Only front wheels used for speed calculation
   - Threshold of 500 (5 kph) may need tuning

---

## Dependencies

### Arduino Libraries
```cpp
#include <Arduino.h>      // Core Arduino library
#include <mcp_can.h>      // MCP2515 CAN controller library
#include <mcp_can_dfs.h>  // MCP2515 definitions
```

### Library Installation
- **MCP_CAN**: Install via Arduino Library Manager
- Search for "MCP_CAN" by Seed Studio or Cory J. Fowler
- Alternative: Download from GitHub and install manually

---

## Debugging Tips

### Serial Output Analysis
```cpp
// Current debug output (line 321):
Serial.print(vehicleSpeed);  // Prints vehicle speed

// Add more debugging:
Serial.print("FL: "); Serial.print(frontLeft);
Serial.print(" FR: "); Serial.println(frontRight);
```

### Common Failure Modes
1. **"Failed to find High Speed CAN"** (line 116)
   - Check MCP2515 wiring
   - Verify CS pin 17 connection
   - Check SPI connections (MOSI, MISO, SCK)

2. **ABS Light Won't Turn Off**
   - Check 0x620 byte 7 value (try 2, 3, or 4)
   - Verify 0x630 configuration
   - Ensure both messages transmitting

3. **Immobilizer Not Disabling**
   - Monitor CAN ID 0x47 for requests
   - Verify responses on 0x041
   - Check handshake byte values (lines 298-303)

4. **Throttle Not Responding**
   - Check voltage at A1 (should be 1.7V - 4.0V)
   - Verify lowPedal calibration at startup
   - Monitor Serial output of val and output

---

## File Modification Guidelines

### When Modifying RX8_CANBUS.ino

1. **Always backup first**: Copy file before changes
2. **Test incrementally**: Change one thing at a time
3. **Document changes**: Add comments explaining modifications
4. **Preserve structure**: Don't reorganize working code unnecessarily
5. **Validate safety**: Check throttle limits and error handling

### Git Workflow
```bash
# Create feature branch
git checkout -b feature/your-feature-name

# Make changes, test, commit
git add RX8_CANBUS.ino
git commit -m "Description of changes and why"

# Push to repository
git push -u origin feature/your-feature-name
```

---

## Resources

### Documentation Files
- **08_Steering.pdf**: Power steering system details
- **13Electrical.pdf**: Complete electrical system documentation
- **Engine-Manual.pdf**: Engine specifications and operation
- **RX8 CanBus and Throttle Explained.pdf**: In-depth CAN protocol details
- **CanBus Raw Output.xlsx**: Raw CAN data for analysis
- **RX8CanBus20-05-2020.xlsx**: Processed CAN message database

### External References
- MCP2515 Datasheet: https://www.microchip.com/en-us/product/MCP2515
- CAN Bus Protocol: https://en.wikipedia.org/wiki/CAN_bus
- Arduino Leonardo: https://docs.arduino.cc/hardware/leonardo

---

## Project Context

This project represents **hundreds of hours** of reverse engineering:
- CAN bus message sniffing and analysis
- Trial and error testing in vehicle
- Communication with RX8 community
- Hardware debugging (several Arduinos damaged)

**Use Case**: Typically used for engine swaps where the factory PCM is removed but the rest of the vehicle systems (dashboard, ABS, power steering, etc.) need to remain functional.

---

## Support and Attribution

**Author**: David Blackhurst
**Contact**: dave@blackhurst.co.uk
**Donations**: https://www.paypal.me/DBlackhurst
**Note**: If this code saves you time, consider supporting the author's next project (Leaf RX8 Conversion)

Code contains snippets from various sources accumulated during research. If you recognize your code, contact the author for proper attribution.

---

## AI Assistant Quick Reference

### When Asked to Modify Code
1. ✅ DO: Read and understand existing implementation first
2. ✅ DO: Explain what changes will do and WHY
3. ✅ DO: Preserve safety checks and timing requirements
4. ✅ DO: Add detailed comments to new code
5. ❌ DON'T: Change CAN message timing (100ms is critical)
6. ❌ DON'T: Remove safety checks without explicit permission
7. ❌ DON'T: Modify throttle safety limits without careful consideration
8. ❌ DON'T: Change CAN message structures without understanding impact

### Quick File Reference
- **Main code**: `RX8_CANBUS.ino` (lines 1-343)
- **CAN initialization**: Lines 100-124
- **Default values**: Lines 126-169
- **MIL lights**: Lines 171-205
- **PCM status**: Lines 207-218
- **DSC status**: Lines 220-250
- **CAN transmit**: Lines 252-272
- **Main loop**: Lines 274-342
- **Immobilizer**: Lines 296-304
- **Wheel speeds**: Lines 306-322
- **Throttle processing**: Lines 325-342

### Critical Line Numbers
- **100ms timing**: Line 276
- **Throttle safety**: Lines 329-331
- **Speed safety**: Lines 314-319
- **CAN messages**: Lines 83-99 (arrays), 254-266 (transmission)

---

*Last Updated: 2025-11-15*
*Repository: MazdaRX8Arduino*
*This document is for AI assistants working on this codebase*
