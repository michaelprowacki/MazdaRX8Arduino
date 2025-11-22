# Coolant Monitor Module - CAN Bus Integration

## Overview

The Coolant Monitor Module currently uses **dedicated sensors** (thermistor + pressure sensor) for precise coolant monitoring. This document explains how to **optionally** add CAN bus reading for:

1. **Comparison** with ECU coolant temperature
2. **Validation** of dedicated sensor accuracy
3. **Warning detection** from ECU
4. **Data logging** correlation

**Note**: The dedicated sensors remain the **primary** measurement method for accuracy.

---

## Why Add CAN Integration?

### Advantages

✅ **Validation**: Compare dedicated sensor vs ECU reading
✅ **Redundancy**: Cross-check temperature measurements
✅ **Warning Sync**: Show ECU warnings on coolant monitor
✅ **Data Correlation**: Log both sensor and ECU data together
✅ **Debugging**: Identify sensor drift or ECU issues

### Disadvantages

❌ **Additional Hardware**: Requires MCP2515 CAN module
❌ **Code Complexity**: More complex firmware
❌ **Not Required**: Dedicated sensors are sufficient alone

---

## Integration Options

### Option 1: Display Comparison (Recommended)

Show both dedicated sensor and ECU readings side-by-side:

```
┌─────────────────────┐
│  COOLANT MONITOR    │
├─────────────────────┤
│ Sensor:  92°C       │  ← From thermistor
│ ECU:     90°C       │  ← From CAN bus (0x420)
│ Diff:    +2°C       │  ← Calculated difference
├─────────────────────┤
│ Pressure: 15 PSI    │  ← From pressure sensor
└─────────────────────┘
```

**Benefits**:
- Validate sensor accuracy
- Detect sensor drift
- Identify ECU calibration differences

---

### Option 2: Warning Synchronization

Display ECU warning lights alongside sensor readings:

```
┌─────────────────────┐
│  COOLANT MONITOR    │
├─────────────────────┤
│ Temp:  92°C         │
│ Press: 15 PSI       │
├─────────────────────┤
│ ECU Warnings:       │
│ ⚠ Low Coolant       │  ← From CAN 0x420
│ ⚠ Overheat          │  ← From CAN 0x420
└─────────────────────┘
```

**Benefits**:
- Show ECU status
- Comprehensive warning system
- Early detection of issues

---

### Option 3: Data Logging

Log both sensor and ECU data for analysis:

**CSV Output**:
```csv
Time,Sensor_Temp_C,ECU_Temp_C,Diff_C,Pressure_PSI,ECU_Low_Coolant,ECU_Overheat
0,92,90,2,15,0,0
1000,93,91,2,15,0,0
2000,95,93,2,14,0,0
3000,98,96,2,13,1,0
```

**Benefits**:
- Track temperature delta over time
- Correlate pressure with temperature
- Identify trends and patterns
- Export for analysis

---

## Hardware Requirements

**Existing** (Coolant Monitor):
- Arduino Pro Micro
- Custom PCB
- Delphi thermistor (or AEM)
- AEM pressure sensor
- 3x OLED displays (I2C)

**Additional** (for CAN integration):
- MCP2515 CAN module
- Wiring to RX8 CAN bus (CAN_H, CAN_L)
- 2 additional I/O pins (CS, INT)

**Wiring**:
| MCP2515 | Pro Micro |
|---------|-----------|
| VCC | 5V |
| GND | GND |
| CS | Pin 10 (or available) |
| SI | MOSI |
| SO | MISO |
| SCK | SCK |
| INT | Pin 2 (or available) |

---

## Code Example

### Add CAN Reading to Existing Firmware

```cpp
#include "coolant_monitor.h"  // Existing headers
#include <mcp_can.h>
#include "RX8_CAN_Messages.h"  // NEW!

// Existing sensor variables
float sensorTempC = 0;
float pressurePSI = 0;

// NEW: CAN decoder
#define CAN_CS 10
#define CAN_INT 2
MCP_CAN CAN0(CAN_CS);
RX8_CAN_Decoder decoder;

// NEW: ECU data
int ecuTempC = 0;
bool ecuLowCoolant = false;
bool ecuOverheat = false;

void setup() {
    // Existing setup code...
    setupSensors();
    setupDisplays();

    // NEW: Initialize CAN bus (optional - don't halt if fails)
    if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
        CAN0.setMode(MCP_NORMAL);
        pinMode(CAN_INT, INPUT);
        Serial.println("CAN: OK");
    } else {
        Serial.println("CAN: Not available - sensor-only mode");
    }
}

void loop() {
    // Existing sensor reading
    sensorTempC = getCoolantTempCelsius();  // From thermistor
    pressurePSI = getPressurePSI();         // From pressure sensor

    // NEW: Read CAN data (if available)
    readCANData();

    // NEW: Display comparison
    displayWithComparison();

    delay(100);
}

void readCANData() {
    long unsigned int rxId;
    unsigned char len = 0;
    unsigned char rxBuf[8];

    if (!digitalRead(CAN_INT)) {
        if (CAN0.readMsgBuf(&rxId, &len, rxBuf) == CAN_OK) {
            if (rxId == CAN_ID_WARNING_LIGHTS) {  // 0x420
                decoder.decode0x420(rxBuf);

                // Extract ECU data
                ecuTempC = decoder.tempToCelsius(
                    decoder.warningLights.coolantTemperature
                );
                ecuLowCoolant = decoder.warningLights.lowCoolantMIL;
                ecuOverheat = decoder.warningLights.engineOverheat;
            }
        }
    }
}

void displayWithComparison() {
    // Display 1: Sensor temperature (primary)
    display1.clearDisplay();
    display1.setCursor(0, 0);
    display1.setTextSize(2);
    display1.print(sensorTempC, 1);
    display1.print("C");

    // Show ECU temp for comparison
    display1.setTextSize(1);
    display1.setCursor(0, 20);
    display1.print("ECU: ");
    display1.print(ecuTempC);
    display1.print("C");

    // Show difference
    int diff = sensorTempC - ecuTempC;
    display1.setCursor(0, 30);
    display1.print("Diff: ");
    if (diff > 0) display1.print("+");
    display1.print(diff);
    display1.print("C");

    display1.display();

    // Display 2: Pressure (unchanged)
    // ... existing pressure display code ...

    // Display 3: Warnings (add ECU warnings)
    display3.clearDisplay();
    display3.setCursor(0, 0);
    display3.setTextSize(1);

    if (sensorTempC > COOLANT_TEMP_HIGH_WARNING) {
        display3.println("! SENSOR HIGH");
    }
    if (ecuOverheat) {
        display3.println("! ECU OVERHEAT");
    }
    if (ecuLowCoolant) {
        display3.println("! ECU LOW COOLANT");
    }
    if (pressurePSI > COOLANT_PSI_HIGH_WARNING) {
        display3.println("! HIGH PRESSURE");
    }

    display3.display();
}
```

---

## Validation Use Cases

### 1. Sensor Calibration

Compare dedicated sensor vs ECU to verify calibration:

```
Sensor:  92°C
ECU:     90°C
Diff:    +2°C  ← Acceptable (within tolerance)
```

**Action**: If difference >5°C, check sensor calibration

---

### 2. Sensor Drift Detection

Monitor temperature delta over time:

```
Time  | Sensor | ECU | Diff
------|--------|-----|-----
0s    | 90°C   | 90°C| 0°C   ← Calibrated
1hr   | 92°C   | 90°C| +2°C  ← Slight drift
2hr   | 95°C   | 90°C| +5°C  ← Significant drift
```

**Action**: If drift increases over time, sensor may be failing

---

### 3. ECU Accuracy Verification

Factory ECU coolant readings are known to be inaccurate. Validate with precise sensor:

```
Actual:  92°C  ← From precision thermistor
ECU:     85°C  ← Factory reading (smoothed/damped)
```

**Conclusion**: Factory ECU readings lag and are smoothed for dashboard

---

### 4. Warning Correlation

Correlate sensor warnings with ECU warnings:

```
Sensor Temp: 105°C  → Trigger HIGH TEMP warning
ECU Temp:    103°C  → ECU triggers OVERHEAT warning

Pressure:    25 PSI → Trigger HIGH PRESSURE warning
ECU:         Normal  → ECU doesn't monitor pressure
```

**Insight**: Dedicated sensors provide earlier/more accurate warnings

---

## Performance Impact

### Minimal Overhead

Adding CAN reading has minimal impact on existing firmware:

- **CAN Reading**: ~10μs per message
- **Decoding**: ~10μs per message
- **Total**: <1% CPU usage
- **Display**: Unchanged (still 100ms update rate)

### No Dependency

CAN bus integration is **optional**:
- Works in sensor-only mode if CAN unavailable
- Graceful degradation if CAN fails
- Dedicated sensors remain primary measurement

---

## Advanced: Temperature Fusion

Combine sensor and ECU readings using weighted average:

```cpp
// Weighted average (favor sensor reading)
float fusedTemp = (sensorTempC * 0.8) + (ecuTempC * 0.2);

// Or: Use sensor normally, fall back to ECU if sensor fails
float displayTemp = (sensorTempC > 0) ? sensorTempC : ecuTempC;

// Or: Average if within tolerance, use sensor if diverged
float tempDiff = abs(sensorTempC - ecuTempC);
if (tempDiff < 5) {
    displayTemp = (sensorTempC + ecuTempC) / 2;  // Average
} else {
    displayTemp = sensorTempC;  // Trust sensor
}
```

---

## Recommendation

**For most users**: Dedicated sensors alone are sufficient. The factory ECU coolant reading is unreliable.

**Consider CAN integration if**:
- You want to validate sensor accuracy
- You're debugging sensor issues
- You want comprehensive warning display
- You're logging data for analysis
- You want redundant temperature measurement

**Skip CAN integration if**:
- You trust dedicated sensor readings
- You want simpler firmware
- You don't have spare I/O pins
- You don't want additional hardware cost

---

## Related Documentation

- **Coolant Monitor README**: [README.md](README.md)
- **CAN Protocol Reference**: [../Documentation/CAN_PID_Reference.md](../Documentation/CAN_PID_Reference.md)
- **RX8_CAN_Messages Library**: [../lib/README.md](../lib/README.md)
- **Integration Guide**: [../Documentation/INTEGRATION_GUIDE.md](../Documentation/INTEGRATION_GUIDE.md)

---

*Last Updated: 2025-11-15*
*Integration: Optional enhancement, not required*
