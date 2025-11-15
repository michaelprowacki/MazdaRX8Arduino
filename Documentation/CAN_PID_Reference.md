# Mazda RX-8 CAN Bus PID Reference

## Overview

This document provides a comprehensive reference for CAN Bus Parameter IDs (PIDs) used in the Mazda RX-8. This information is valuable for telemetry, data logging, and understanding vehicle communication.

**Sources**:
- https://github.com/topolittle/RX8-CAN-BUS
- https://github.com/rnd-ash/rx8-reverse-engineering
- MazdaRX8Arduino project testing

**Test Equipment** (from topolittle):
- OBDLink MX+ Bluetooth Scanner
- RaceChrono v7.5.3
- 2006 Mazda RX-8 (Series 1, Manual Transmission)

---

## CAN Bus Configuration

- **Speed**: 500 kbit/s (High Speed CAN)
- **Protocol**: ISO 15765-4 CAN (11-bit ID)
- **Byte Order**: Big Endian (MSB first)
- **Bit Position**: MSB 0 (Most Significant Bit = position 0)

---

## Standard PIDs (OBD-II Mode 01)

### Engine RPM
- **PID**: 0x0C
- **Bytes**: 2
- **Formula**: `(A * 256 + B) / 4`
- **Unit**: RPM
- **Range**: 0 - 16,383 RPM

### Vehicle Speed
- **PID**: 0x0D
- **Bytes**: 1
- **Formula**: `A`
- **Unit**: km/h
- **Range**: 0 - 255 km/h

### Throttle Position
- **PID**: 0x11
- **Bytes**: 1
- **Formula**: `A * 100 / 255`
- **Unit**: %
- **Range**: 0 - 100%

### Coolant Temperature
- **PID**: 0x05
- **Bytes**: 1
- **Formula**: `A - 40`
- **Unit**: °C
- **Range**: -40 - 215°C

### Intake Air Temperature
- **PID**: 0x0F
- **Bytes**: 1
- **Formula**: `A - 40`
- **Unit**: °C
- **Range**: -40 - 215°C

---

## RX-8 Specific CAN Messages

### 0x201 - PCM Status (Primary)

**Length**: 8 bytes
**Update Rate**: 100ms (10 Hz)
**Source**: PCM (Powertrain Control Module)

**Byte Layout**:
```
Byte 0-1: Engine RPM (16-bit)
Byte 2-3: 0xFF 0xFF (fixed)
Byte 4-5: Vehicle Speed (16-bit)
Byte 6:   Throttle Pedal Position
Byte 7:   0xFF (fixed)
```

**Engine RPM Calculation**:
```
Option 1 (Standard): RPM = ((Byte0 * 256) + Byte1) / 3.85
Option 2 (RaceChrono): RPM = ((Byte0 * 256) + Byte1) / 4

Note: 3.85 divisor matches dashboard display more accurately
```

**Vehicle Speed Calculation**:
```
Speed (mph) = (((Byte4 * 256) + Byte5) - 10000) / 100
Speed (km/h) = Speed (mph) * 1.60934
```

**Throttle Pedal**:
```
Throttle (%) = Byte6 * 0.5
Range: 0-100% in 0.5% increments
```

**RaceChrono Configuration**:
```
CAN ID: 0x201
Bit Start: 0
Bit Count: 16
Byte Order: Big Endian
Formula: (A / 3.85)
```

---

### 0x420 - Warning Lights & Temperature

**Length**: 7 bytes
**Update Rate**: 100ms (10 Hz)
**Source**: PCM

**Byte Layout**:
```
Byte 0:   Engine Temperature
Byte 1:   Odometer data
Byte 2-3: Unknown
Byte 4:   Oil Pressure Status
Byte 5:   Warning Light Flags 1
Byte 6:   Warning Light Flags 2
```

**Engine Temperature**:
```
Temperature (°C) = Byte0
Normal operating: 145
```

**Oil Pressure**:
```
Byte4 = 1: Oil pressure OK
Byte4 = 0: Oil pressure low
```

**Warning Lights (Byte 5)**:
```
Bit 6 (0x40): Check Engine MIL (Solid)
Bit 7 (0x80): Check Engine Backlight (Blinking)
```

**Warning Lights (Byte 6)**:
```
Bit 1 (0x02): Low Coolant
Bit 6 (0x40): Battery Charge
Bit 7 (0x80): Oil Pressure
```

---

### 0x4B0 - Steering Wheel Angle

**Length**: 8 bytes
**Update Rate**: ~20ms (50 Hz)
**Source**: EPS (Electric Power Steering)

**Byte Layout**:
```
Byte 0-1: Steering Angle (16-bit signed)
Byte 2-7: Unknown
```

**Steering Angle Calculation**:
```
// Convert to signed 16-bit integer
int16_t rawAngle = (Byte0 << 8) | Byte1;
float angle = rawAngle * 0.1;  // degrees

Positive: Right turn
Negative: Left turn
Range: ~±720° (2 full rotations)
```

**RaceChrono Configuration**:
```
CAN ID: 0x4B0
Bit Start: 0
Bit Count: 16
Byte Order: Big Endian
Signed: Yes
Formula: (A * 0.1)
Unit: degrees
```

---

### 0x4B1 - Wheel Speeds

**Length**: 8 bytes
**Update Rate**: ~20ms (50 Hz)
**Source**: ABS/DSC Module

**Byte Layout**:
```
Byte 0-1: Front Left Wheel Speed (16-bit)
Byte 2-3: Front Right Wheel Speed (16-bit)
Byte 4-5: Rear Left Wheel Speed (16-bit)
Byte 6-7: Rear Right Wheel Speed (16-bit)
```

**Wheel Speed Calculation**:
```
// For each wheel:
Speed (km/h) = (((ByteHigh * 256) + ByteLow) - 10000) / 100

// Example for front left:
FL_Speed = (((Byte0 * 256) + Byte1) - 10000) / 100
```

**Average Speed Calculation**:
```
// Using front wheels only (more reliable):
avgSpeed = ((FL_Speed + FR_Speed) / 2)

// Safety check for wheel slip:
if (abs(FL_Speed - FR_Speed) > 5.0) {
    // Possible wheel slip or sensor error
}
```

---

### 0x4BE - Brake Pedal & Misc

**Length**: 8 bytes
**Update Rate**: ~20ms (50 Hz)
**Source**: PCM/ABS

**Byte Layout**:
```
Byte 0:   Brake Pedal Position
Byte 1-7: Unknown/Mixed signals
```

**Brake Pedal Calculation**:
```
BrakePressure (%) = Byte0 * 0.5
Range: 0-100% in 0.5% increments
```

**RaceChrono Configuration**:
```
CAN ID: 0x4BE
Bit Start: 0
Bit Count: 8
Byte Order: Big Endian
Formula: (A * 0.5)
Unit: %
```

---

### 0x075 - Handbrake & Neutral

**Length**: 1-2 bytes
**Update Rate**: Variable
**Source**: BCM (Body Control Module)

**Byte Layout**:
```
Bit 0: Handbrake Status (1 = engaged)
Bit 1: Neutral Position (1 = in neutral)
Bit 2-7: Unknown
```

**Handbrake Status**:
```
Handbrake = (Byte0 & 0x01) ? 1 : 0
```

**Neutral Detection**:
```
InNeutral = (Byte0 & 0x02) ? 1 : 0
```

---

### 0x203 - Traction Control Data

**Length**: 7 bytes
**Update Rate**: 100ms (10 Hz)
**Source**: PCM

**Byte Layout**:
```
Fixed pattern: {19, 19, 19, 19, 175, 3, 19}
```

**Purpose**: Traction control system communication
**Note**: Values appear to be status flags rather than dynamic data

---

### 0x212 - DSC/ABS Status

**Length**: 7 bytes
**Update Rate**: 100ms (10 Hz)
**Source**: ABS/DSC Module

**Byte Layout**:
```
Byte 0-2: Unknown
Byte 3:   DSC Mode Flags
Byte 4:   ABS/Brake Warning Flags
Byte 5:   Electronic Throttle Control Flags
Byte 6:   Unknown
```

**DSC Mode (Byte 3)**:
```
Bit 2 (0x04): DSC Off indicator
```

**Warning Flags (Byte 4)**:
```
Bit 3 (0x08): ABS MIL
Bit 6 (0x40): Brake System Failure
```

**ETC Flags (Byte 5)**:
```
Bit 4 (0x10): ETC Disabled
Bit 5 (0x20): ETC Active Backlight
```

---

### 0x215, 0x231, 0x240 - PCM Supplements

**Length**: Varies (5-8 bytes)
**Update Rate**: 100ms (10 Hz)
**Source**: PCM

**Fixed Patterns**:
```
0x215: {2, 45, 2, 45, 2, 42, 6, 129}
0x231: {15, 0, 255, 255, 0}
0x240: {4, 0, 40, 0, 2, 55, 6, 129}
```

**Purpose**: PCM presence indicators
**Note**: Required for other modules to recognize PCM as present

---

### 0x620 - ABS System Data

**Length**: 7 bytes
**Update Rate**: 100ms (10 Hz)
**Source**: ABS Module

**Byte Layout**:
```
Byte 0-3: Unknown
Byte 4:   Fixed (16)
Byte 5:   Fixed (0)
Byte 6:   Vehicle-specific (2, 3, or 4)
```

**Note**: Byte 6 varies between RX8s. Try 2, 3, or 4 if ABS light stays on.

---

### 0x630 - ABS Configuration

**Length**: 8 bytes
**Update Rate**: 100ms (10 Hz)
**Source**: ABS Module

**Byte Layout**:
```
Byte 0:   AT/MT Configuration (8)
Byte 1-5: Unknown (0)
Byte 6:   Wheel Size Parameter (106)
Byte 7:   Wheel Size Parameter (106)
```

**Purpose**: ABS system configuration
**Note**: Required for ABS light to turn off

---

### 0x650 - ABS Supplement

**Length**: 1 byte
**Update Rate**: 100ms (10 Hz)
**Source**: ABS Module

**Byte Layout**:
```
Byte 0: Fixed (0)
```

---

## CAN Message Implementation Reference

### Current Project Usage

Messages currently implemented in `RX8_CANBUS.ino`:

| Message ID | Status | Purpose |
|------------|--------|---------|
| 0x041 | ✅ TX | Immobilizer response |
| 0x047 | ✅ RX | Immobilizer request |
| 0x201 | ✅ TX | PCM Status (RPM, Speed, Throttle) |
| 0x203 | ✅ TX | Traction Control |
| 0x212 | 🔶 TX | DSC/ABS (commented out by default) |
| 0x215 | ✅ TX | PCM Supplement |
| 0x231 | ✅ TX | PCM Supplement |
| 0x240 | ✅ TX | PCM Supplement |
| 0x420 | ✅ TX | Warning Lights & Temperature |
| 0x4B1 | ✅ RX | Wheel Speeds |
| 0x620 | ✅ TX | ABS System Data |
| 0x630 | ✅ TX | ABS Configuration |
| 0x650 | ✅ TX | ABS Supplement |

### Potential Additions

Messages documented but not yet implemented:

| Message ID | Potential Use | Priority |
|------------|---------------|----------|
| 0x4B0 | Steering angle display/logging | Medium |
| 0x4BE | Brake pressure monitoring | Medium |
| 0x075 | Handbrake/neutral detection | Low |
| 0x300 | EPS status monitoring | Low |

---

## Racing Telemetry Configuration

### RaceChrono v7 Example

**RPM (0x201)**:
```
Name: Engine RPM
CAN ID: 0x201
Start Bit: 0
Bit Count: 16
Byte Order: Big Endian
Data Type: Unsigned
Formula: A / 3.85
Min: 0
Max: 10000
Unit: rpm
```

**Speed (0x201)**:
```
Name: Speed
CAN ID: 0x201
Start Bit: 32
Bit Count: 16
Byte Order: Big Endian
Data Type: Unsigned
Formula: (A - 10000) / 100
Min: 0
Max: 200
Unit: km/h
```

**Throttle (0x201)**:
```
Name: Throttle
CAN ID: 0x201
Start Bit: 48
Bit Count: 8
Byte Order: Big Endian
Data Type: Unsigned
Formula: A * 0.5
Min: 0
Max: 100
Unit: %
```

**Brake (0x4BE)**:
```
Name: Brake
CAN ID: 0x4BE
Start Bit: 0
Bit Count: 8
Byte Order: Big Endian
Data Type: Unsigned
Formula: A * 0.5
Min: 0
Max: 100
Unit: %
```

**Steering (0x4B0)**:
```
Name: Steering
CAN ID: 0x4B0
Start Bit: 0
Bit Count: 16
Byte Order: Big Endian
Data Type: Signed
Formula: A * 0.1
Min: -720
Max: 720
Unit: degrees
```

---

## Conversion Formulas Summary

### Temperature Conversions
```cpp
// Celsius to display value
uint8_t tempValue = temperature_C;  // Direct

// Display value to Celsius
float temperature = (float)tempValue;  // Direct
```

### Speed Conversions
```cpp
// MPH to CAN value
uint16_t canSpeed = (mph * 100) + 10000;

// CAN value to MPH
float mph = (canSpeed - 10000) / 100.0;

// MPH to KM/H
float kmh = mph * 1.60934;
```

### RPM Conversions
```cpp
// RPM to CAN value (our implementation)
uint16_t canRPM = rpm * 3.85;

// CAN value to RPM
float rpm = canRPM / 3.85;
```

### Throttle Conversions
```cpp
// Percentage to CAN value
uint8_t canThrottle = (throttle_percent * 200) / 100;  // 0.5% increments

// CAN value to percentage
float percent = canThrottle * 0.5;
```

---

## Data Logging Recommendations

### Sample Rate Guidelines

| Parameter | Recommended Rate | CAN Update Rate |
|-----------|------------------|-----------------|
| RPM | 10-20 Hz | 10 Hz |
| Speed | 10-20 Hz | 10 Hz |
| Throttle | 10-20 Hz | 10 Hz |
| Wheel Speeds | 20-50 Hz | 50 Hz |
| Steering Angle | 20-50 Hz | 50 Hz |
| Brake Pressure | 20-50 Hz | 50 Hz |
| Temperature | 1-5 Hz | 10 Hz |
| Warning Lights | 1-5 Hz | 10 Hz |

### Buffer Size Calculations

For 1 hour of logging at 20 Hz:
```
Samples = 20 Hz * 3600 sec = 72,000 samples
Bytes per sample = ~20 bytes (all parameters)
Total = 72,000 * 20 = 1.44 MB/hour
```

---

## CAN Bus Tools

### Recommended Software
- **SavvyCAN**: Free, open-source CAN analysis
- **Kayak**: DBC editor and CAN analyzer
- **Wireshark**: With SocketCAN plugin
- **CANalyzer**: Professional (expensive)

### Recommended Hardware
- **OBDLink MX+**: Bluetooth, 500 kbit/s capable
- **Tactrix Openport 2.0**: J2534 compatible
- **PCAN-USB**: Professional CAN interface
- **Arduino + MCP2515**: DIY solution (this project!)

---

## References

- Original PID Documentation: https://github.com/topolittle/RX8-CAN-BUS
- DBC File: https://github.com/rnd-ash/rx8-reverse-engineering
- RX8 Dash Implementation: https://github.com/Antipixel/RX8-Dash
- MazdaRX8Arduino Project: Current implementation

---

*Last Updated: 2025-11-15*
*Maintained as part of MazdaRX8Arduino project*
