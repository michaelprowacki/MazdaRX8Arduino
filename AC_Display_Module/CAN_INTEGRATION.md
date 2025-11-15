# AC Display Module - CAN Bus Integration

## Overview

The **AC Display Module** can optionally integrate with the RX8 CAN bus to display live engine data alongside AC controls. This document explains how to add CAN bus reading to show RPM, speed, temperature, and other ECU data on the factory AC display.

**Note**: CAN integration is **optional** - the AC Display works standalone without CAN.

---

## Why Add CAN Integration?

### Advantages

✅ **Multi-function Display**: Show engine data on factory AC display
✅ **Real-time Updates**: 100ms refresh rate from ECU
✅ **Menu Pages**: Add custom pages for RPM, speed, voltage, temp
✅ **Warning Indicators**: Display ECU warnings on AC display
✅ **Data Correlation**: Share CAN data with ESP8266_Companion for logging
✅ **Validation**: Debug ECU_Module output in real-time

### Use Cases

1. **Engine Data Display**: Show RPM/speed/temp when AC not in use
2. **ECU Debugging**: Validate ECU_Module CAN output
3. **Custom Gauges**: Add menu pages for battery voltage, wheel speeds, etc.
4. **Warning Display**: Flash AC display on overheating, low oil pressure
5. **Data Logging**: Send CAN data to ESP8266 for Bluetooth/WiFi logging

---

## Integration Options

### Option 1: Additional Menu Pages (Recommended)

Add CAN data as custom menu pages:

```
[AC Controls] ──Long Press──→ [Battery] ──Next──→ [RPM] ──Next──→ [Speed] ──Next──→ [Temp]
     ↑                                                                                  │
     └──────────────────────────────────────────────────────────────────────────────────┘
```

**Display Format**:
```
┌─────────────────────┐
│  12:34   [Icons]    │  ← Time + AC icons
├─────────────────────┤
│      RPM            │  ← Label (scrolling text)
│      3250           │  ← Value (7-segment)
└─────────────────────┘
```

**Benefits**:
- Non-intrusive (AC controls still primary)
- Easy navigation with rotary encoder
- Reuses existing menu system
- No custom display driver needed

---

### Option 2: Status Bar

Show live data in unused display area:

```
┌─────────────────────┐
│  12:34   RPM:3250   │  ← Time + RPM in corner
├─────────────────────┤
│      AUTO  AC       │
│       72°F          │  ← AC controls (unchanged)
└─────────────────────┘
```

**Benefits**:
- Always visible
- Quick glance at key parameters
- AC controls unaffected

**Challenges**:
- Limited display space
- May conflict with AC icons

---

### Option 3: Full Takeover Mode

Replace AC display with gauge cluster when engine running:

```
Long Press OFF → Toggle to Gauge Mode:

┌─────────────────────┐
│   RPM      SPEED    │
│   3250      65      │
├─────────────────────┤
│   TEMP     THROTTLE │
│    92        45%    │
└─────────────────────┘
```

**Benefits**:
- Maximum data visibility
- Multi-parameter dashboard
- Racing/performance focus

**Challenges**:
- AC controls temporarily unavailable
- Requires significant display driver changes

---

## Hardware Requirements

### Existing (AC Display Module)
- Arduino Mega 2560
- Factory AC display unit
- Button panel (4x2 matrix + 2 encoders)
- RTC module (I2C)
- ESP8266 Companion (optional)

### Additional (for CAN integration)
- **MCP2515 CAN module** (SPI)
- **Wiring to RX8 CAN bus** (CAN_H, CAN_L)
- **2 additional I/O pins** (CS, INT)

### Wiring

| MCP2515 | Arduino Mega |
|---------|--------------|
| VCC     | 5V           |
| GND     | GND          |
| CS      | Pin 53 (or available) |
| SI      | MOSI (Pin 51) |
| SO      | MISO (Pin 50) |
| SCK     | SCK (Pin 52)  |
| INT     | Pin 21 (INT0) |

**Note**: Arduino Mega SPI pins are different from Leonardo/Uno!

---

## Software Architecture

### Integration Approach

```
AC Display Module
├── main.cpp (existing AC control logic)
├── can_reader.cpp (NEW - CAN message handling)
├── menu_pages.cpp (NEW - CAN data menu pages)
└── RX8_CAN_Messages.h (shared library)
```

### Data Flow

```
[ECU_Module] ──CAN Bus──→ [MCP2515] ──SPI──→ [Arduino Mega]
                                                    ↓
                                          [RX8_CAN_Decoder]
                                                    ↓
                            ┌───────────────────────┼───────────────────┐
                            ↓                       ↓                   ↓
                      [Menu Pages]          [Display Icons]      [ESP8266]
                      (RPM, Speed)          (Warnings)           (Logging)
```

---

## Code Example

### Basic CAN Integration

Add to `AC_Display_Module/src/main.hpp`:

```cpp
// CAN Bus Integration (optional)
#define ENABLE_CAN_BUS 1  // Set to 0 to disable

#ifdef ENABLE_CAN_BUS
#include <mcp_can.h>
#include "RX8_CAN_Messages.h"

// CAN configuration
#define CAN_CS 53     // Mega default SS pin
#define CAN_INT 21    // INT0 on Mega

// CAN objects
extern MCP_CAN CAN0;
extern RX8_CAN_Decoder canDecoder;

// CAN data variables
extern int liveRPM;
extern int liveSpeed;
extern int liveThrottle;
extern int liveCoolantTemp;
extern bool liveCheckEngine;
extern bool liveOilPressureWarning;

// CAN functions
void initCAN();
void readCANMessages();
#endif
```

### CAN Reader Implementation

Create `AC_Display_Module/src/can_reader.cpp`:

```cpp
#include "main.hpp"

#ifdef ENABLE_CAN_BUS

// CAN objects
MCP_CAN CAN0(CAN_CS);
RX8_CAN_Decoder canDecoder;

// Live CAN data
int liveRPM = 0;
int liveSpeed = 0;
int liveThrottle = 0;
int liveCoolantTemp = 0;
bool liveCheckEngine = false;
bool liveOilPressureWarning = false;

/**
 * Initialize CAN bus
 */
void initCAN() {
    pinMode(CAN_INT, INPUT);

    if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
        Serial.println(F("CAN: Initialized"));
        CAN0.setMode(MCP_NORMAL);
    } else {
        Serial.println(F("CAN: Failed - running without CAN"));
    }
}

/**
 * Read and decode CAN messages
 * Call this in main loop
 */
void readCANMessages() {
    long unsigned int rxId;
    unsigned char len = 0;
    unsigned char rxBuf[8];

    // Read all available messages
    while (!digitalRead(CAN_INT)) {
        if (CAN0.readMsgBuf(&rxId, &len, rxBuf) == CAN_OK) {
            switch(rxId) {
                case CAN_ID_PCM_STATUS:  // 0x201
                    canDecoder.decode0x201(rxBuf);
                    liveRPM = canDecoder.pcmStatus.engineRPM;
                    liveSpeed = canDecoder.pcmStatus.vehicleSpeed;
                    liveThrottle = canDecoder.pcmStatus.throttlePosition;
                    break;

                case CAN_ID_WARNING_LIGHTS:  // 0x420
                    canDecoder.decode0x420(rxBuf);
                    liveCoolantTemp = canDecoder.tempToCelsius(
                        canDecoder.warningLights.coolantTemperature
                    );
                    liveCheckEngine = canDecoder.warningLights.checkEngineMIL;
                    liveOilPressureWarning = canDecoder.warningLights.oilPressureMIL;
                    break;
            }
        }
    }
}

#endif // ENABLE_CAN_BUS
```

### Add Menu Pages

Add to `setup()` in `main.cpp`:

```cpp
#ifdef ENABLE_CAN_BUS
// Initialize CAN bus
initCAN();
Serial.println(F("CAN bus initialized"));

// Register CAN data menu pages
mainMenu.registerPage(new mainMenuPtrPage("RPM", &liveRPM, "", 0));
mainMenu.registerPage(new mainMenuPtrPage("SPD", &liveSpeed, "MPH", 0));
mainMenu.registerPage(new mainMenuPtrPage("TMP", &liveCoolantTemp, "C", 0));
mainMenu.registerPage(new mainMenuPtrPage("THR", &liveThrottle, "%", 0));
Serial.println(F("CAN menu pages registered"));
#endif
```

### Update Main Loop

Add to `loop()` in `main.cpp`:

```cpp
#ifdef ENABLE_CAN_BUS
// Read CAN messages
readCANMessages();

// Flash display on warnings
if (liveCheckEngine || liveOilPressureWarning) {
    // Blink backlight or show warning icon
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink >= 500) {
        // Toggle warning indicator
        lastBlink = millis();
    }
}
#endif
```

---

## Menu Page Configuration

### Available CAN Parameters

From **0x201** (PCM Status):
- `liveRPM` - Engine RPM (0-10000)
- `liveSpeed` - Vehicle Speed (MPH)
- `liveThrottle` - Throttle Position (0-100%)

From **0x420** (Warning Lights):
- `liveCoolantTemp` - Coolant Temperature (°C)
- `liveCheckEngine` - Check Engine MIL (bool)
- `liveOilPressureWarning` - Oil Pressure Warning (bool)

From **0x4B1** (Wheel Speeds):
- `liveFrontLeft` - Front Left Wheel Speed
- `liveFrontRight` - Front Right Wheel Speed

### Custom Menu Page Examples

**Simple Numeric Display**:
```cpp
mainMenu.registerPage(new mainMenuPtrPage("RPM", &liveRPM, "", 0));
// Display: "RPM  3250"
```

**With Units**:
```cpp
mainMenu.registerPage(new mainMenuPtrPage("SPD", &liveSpeed, "MPH", 0));
// Display: "SPD  65 MPH"
```

**With Decimal Places**:
```cpp
mainMenu.registerPage(new mainMenuPtrPage("BAT", &liveVoltage, "V", 1));
// Display: "BAT  13.8 V"
```

**Function-based** (for calculations):
```cpp
int getAvgWheelSpeed() {
    return (liveFrontLeft + liveFrontRight) / 200;  // Average in MPH
}

mainMenu.registerPage(new mainMenuFuncPage("AVG", getAvgWheelSpeed, "MPH", 0));
```

---

## Warning Indicators

### Display ECU Warnings on AC Display

```cpp
void updateWarningDisplay() {
    // Check for active warnings
    if (liveCheckEngine) {
        // Flash "CHECK" message
        disp.writeToCharDisp("CHK");
    }

    if (liveOilPressureWarning) {
        // Flash "OIL" message
        disp.writeToCharDisp("OIL");
    }

    if (liveCoolantTemp > 95) {
        // Flash "HOT" message
        disp.writeToCharDisp("HOT");
    }
}
```

### Use AC Display LEDs

```cpp
void updateWarningLEDs() {
    // Use AC display icons for warnings
    if (liveCheckEngine) {
        midIcons.checkEngine = true;  // If available on display
    }

    if (liveOilPressureWarning) {
        // Flash front demist icon as warning
        static bool warningBlink = false;
        warningBlink = !warningBlink;
        // Set LED state
    }
}
```

---

## ESP8266 Integration

### Send CAN Data to ESP8266 Companion

```cpp
void sendCANtoESP() {
    // Send CAN data over Serial3 to ESP8266
    esp.sendCommand("RPM", liveRPM);
    esp.sendCommand("SPEED", liveSpeed);
    esp.sendCommand("TEMP", liveCoolantTemp);
    esp.sendCommand("THROTTLE", liveThrottle);
}
```

### ESP8266 Can Then:
- Log data to SD card
- Send via Bluetooth to phone app
- Upload to WiFi/cloud logging
- Trigger remote alerts

---

## Performance Impact

### CPU Usage

Adding CAN reading has minimal impact:
- **CAN Reading**: ~10μs per message
- **Decoding**: ~10μs per message
- **Display Update**: Unchanged (same 7-segment driver)
- **Total Overhead**: <1% CPU usage

### Memory Usage

- **RX8_CAN_Decoder**: ~200 bytes
- **CAN buffer**: 8 bytes
- **Data variables**: ~20 bytes
- **Total**: ~230 bytes (0.3% of Mega's 8KB RAM)

### No Degradation

CAN integration does not affect:
- AC control responsiveness
- Button scanning
- Menu navigation
- Display refresh rate

---

## Testing Strategy

### Bench Testing (Without Vehicle)

1. **CAN Simulator**:
   - Use second Arduino as CAN message sender
   - Send test messages (0x201, 0x420)
   - Verify decoding and display

2. **Mock Data**:
   - Comment out CAN reading
   - Manually set `liveRPM = 3000`
   - Test menu page display

### Vehicle Testing

1. **Passive Monitoring**:
   - Connect to CAN bus (read-only)
   - Verify messages received
   - Compare to dashboard

2. **Menu Navigation**:
   - Cycle through CAN data pages
   - Verify values update in real-time
   - Check responsiveness

3. **Warning Display**:
   - Trigger warnings (low oil pressure, overheat)
   - Verify AC display shows alerts
   - Test warning dismiss/acknowledge

---

## Troubleshooting

### CAN Bus Not Initializing

**Symptom**: "CAN: Failed" message

**Solutions**:
- Check MCP2515 wiring (CS, INT, SPI pins)
- Verify CAN_H/CAN_L connections
- Check 120Ω termination resistors
- Try different CS pin

### No CAN Messages Received

**Symptom**: `liveRPM` stays at 0

**Solutions**:
- Verify ECU_Module is transmitting (use CAN sniffer)
- Check CAN bus speed (500 kbps)
- Verify interrupt pin (digitalRead should toggle)
- Add debug output in `readCANMessages()`

### Display Garbled

**Symptom**: Strange characters on 7-segment display

**Solutions**:
- Check data types (int overflow?)
- Verify menu page format string
- Check display driver library compatibility
- Add bounds checking to values

### ESP8266 Not Receiving Data

**Symptom**: ESP8266 doesn't log CAN data

**Solutions**:
- Verify Serial3 baud rate (115200)
- Check `esp.sendCommand()` format
- Monitor Serial3 TX with logic analyzer
- Verify ESP8266 command parser

---

## Advanced Features

### Data Fusion (CAN + Local Sensors)

```cpp
// Compare AC display battery reading with CAN bus battery voltage
float localBatVolt = getBatVolt();  // From analog input
float canBatVolt = canDecoder.getVoltage();  // From CAN (if available)

// Average for accuracy
float fusedVoltage = (localBatVolt + canBatVolt) / 2;

mainMenu.registerPage(new mainMenuFuncPage("BAT", getFusedVoltage, "V", 1));
```

### Conditional Display

```cpp
// Only show RPM when engine running
void loop() {
    if (liveRPM > 500) {
        // Engine running - enable RPM page
        mainMenu.rpmPage->setEnabled(true);
    } else {
        // Engine off - hide RPM page
        mainMenu.rpmPage->setEnabled(false);
    }
}
```

### Warning History

```cpp
// Track warning events
struct WarningEvent {
    unsigned long timestamp;
    uint8_t warningType;
};

WarningEvent warningLog[10];
int warningCount = 0;

void logWarning(uint8_t type) {
    if (warningCount < 10) {
        warningLog[warningCount].timestamp = millis();
        warningLog[warningCount].warningType = type;
        warningCount++;
    }
}

// Display warning count on menu page
mainMenu.registerPage(new mainMenuPtrPage("WRN", &warningCount, "", 0));
```

---

## Comparison: Standalone vs CAN-Enabled

| Feature | Standalone | With CAN Integration |
|---------|------------|---------------------|
| **AC Control** | ✅ Full | ✅ Full |
| **Clock Display** | ✅ RTC | ✅ RTC |
| **Battery Voltage** | ✅ Analog input | ✅ Analog + CAN validation |
| **Motor Temp** | ✅ Manual/static | ✅ Live from ECU |
| **Engine RPM** | ❌ None | ✅ Live from ECU |
| **Vehicle Speed** | ❌ None | ✅ Live from ECU |
| **Throttle Position** | ❌ None | ✅ Live from ECU |
| **Warning Lights** | ❌ None | ✅ Live from ECU |
| **Data Logging** | ❌ None | ✅ Via ESP8266 |
| **Update Rate** | N/A | ✅ 100ms |
| **Additional Hardware** | None | MCP2515 CAN module |

---

## Recommendation

### For Most Users
**Standalone mode is sufficient**. The AC display's primary purpose is climate control.

### Consider CAN Integration If:
- ✅ You want multi-function display (AC + engine data)
- ✅ You're debugging ECU_Module
- ✅ You want centralized data logging via ESP8266
- ✅ You want warning display redundancy
- ✅ You have spare GPIO pins on Mega

### Skip CAN Integration If:
- ❌ You only need AC control
- ❌ You have a separate gauge cluster
- ❌ You want simpler firmware
- ❌ You don't want additional wiring

---

## Related Documentation

- **AC Display Module README**: [README.md](README.md)
- **RX8_CAN_Messages Library**: [../lib/README.md](../lib/README.md)
- **CAN Protocol Reference**: [../Documentation/CAN_PID_Reference.md](../Documentation/CAN_PID_Reference.md)
- **Integration Guide**: [../Documentation/INTEGRATION_GUIDE.md](../Documentation/INTEGRATION_GUIDE.md)
- **ESP8266 Companion**: [ESP8266_Companion/README.md](ESP8266_Companion/README.md)

---

*Last Updated: 2025-11-15*
*Integration: Optional enhancement for advanced users*
