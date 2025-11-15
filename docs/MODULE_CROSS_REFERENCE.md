# Module Cross-Reference Guide

Quick reference for module compatibility, communication methods, and integration scenarios.

---

## Module Compatibility Matrix

| Module | Works Standalone | Works with ECU | Works with EV | Needs CAN | Needs Serial | Custom Hardware |
|--------|------------------|----------------|---------------|-----------|--------------|-----------------|
| **ECU_Module** | ✅ (primary) | N/A (is ECU) | ❌ (mutually exclusive) | ✅ Required | ⚪ Optional | Throttle pedal I/O |
| **EV_ECU_Module** | ✅ (primary) | ❌ (mutually exclusive) | N/A (is EV ECU) | ✅ Required | ⚪ Optional | Open Inverter CAN |
| **Dash_Controller_Module** | ✅ (alternative) | ❌ (alternative to ECU) | ❌ (alternative to EV) | ✅ Required | ❌ No | Varies (PlatformIO) |
| **AC_Display_Module** | ✅ | ✅ | ✅ | ⚪ Optional | ⚪ Optional | Factory AC display |
| **ESP8266_Companion** | ❌ (needs AC Display) | ✅ (via AC Display) | ✅ (via AC Display) | ❌ No | ✅ Required | WiFi/BT antenna |
| **Aftermarket_Display_Module** | ❌ (needs CAN data) | ✅ | ✅ | ✅ Required | ❌ No | RGB OLED displays |
| **Coolant_Monitor_Module** | ✅ | ✅ | ✅ | ❌ No | ⚪ Optional | Custom PCB, sensors |
| **Wipers_Module** | ✅ | ✅ | ✅ | ❌ No | ❌ No | Wiper control access |
| **Sim_Racing_Module** | ✅ (PC-connected) | ❌ (standalone) | ❌ (standalone) | ⚪ Optional | ✅ Required (USB) | None (cluster only) |

**Legend**:
- ✅ Yes / Supported
- ❌ No / Not compatible
- ⚪ Optional
- N/A: Not applicable

---

## Primary ECU Module Selection (Choose ONE)

### ECU_Module (ICE Engine)
**When to use**:
- Engine swaps (LS, 2JZ, K-series, rotary, etc.)
- Standalone engine management
- Factory PCM removed
- Gasoline/petrol engine

**Cannot use with**: EV_ECU_Module, Dash_Controller_Module

### EV_ECU_Module (Electric Motor)
**When to use**:
- EV conversions
- Open Inverter or similar motor controller
- Factory PCM removed
- Electric motor replaces engine

**Cannot use with**: ECU_Module, Dash_Controller_Module

### Dash_Controller_Module (Alternative)
**When to use**:
- Reference implementation for development
- Alternative code structure (C++ classes)
- Educational purposes
- Protocol validation

**Cannot use with**: ECU_Module, EV_ECU_Module (serves same purpose)

---

## Module Communication Methods

### CAN Bus Communication

**Modules that transmit CAN**:
- ECU_Module (0x201, 0x420, 0x620, 0x630, 0x041)
- EV_ECU_Module (0x201, 0x420, 0x620, 0x630, 0x041)
- Dash_Controller_Module (various)

**Modules that receive CAN**:
- Aftermarket_Display_Module (reads 0x201, 0x420, etc.)
- AC_Display_Module (optional - can read CAN for data)

**Important**: Only ONE module should transmit on each CAN message ID. Multiple modules can receive (read-only).

### Serial Communication

**Serial Transmitters**:
- AC_Display_Module (can send battery voltage, etc. via Serial3)
- Coolant_Monitor_Module (can send temp/pressure via Serial)
- ECU_Module (can send RPM, speed via Serial1)
- EV_ECU_Module (can send motor data via Serial)

**Serial Receivers**:
- ESP8266_Companion (receives from AC_Display_Module)
- AC_Display_Module (can receive from ECU modules)

**Protocol**: Text-based `$COMMAND:VALUE\n` format

### Standalone (No Communication)

**Modules that work independently**:
- Coolant_Monitor_Module (dedicated sensors, OLED displays)
- Wipers_Module (taps wiper control signals)
- Sim_Racing_Module (PC-connected via USB)

---

## Integration Scenarios by Use Case

### Scenario 1: Minimal ECU Replacement

**Goal**: Basic engine swap functionality

**Modules**:
- ECU_Module

**Communication**: CAN bus to vehicle only

**Hardware**: Arduino Leonardo + MCP2515

**Complexity**: Low

---

### Scenario 2: ECU + Basic Monitoring

**Goal**: Engine swap + aftermarket gauges

**Modules**:
- ECU_Module (primary)
- Aftermarket_Display_Module (displays)

**Communication**:
- ECU_Module transmits on CAN bus
- Aftermarket_Display_Module reads CAN bus (passive)

**Hardware**:
- Arduino Leonardo + MCP2515 (ECU)
- Arduino Uno/Nano + MCP2515 + RGB OLED displays (Aftermarket Display)

**Wiring**: Both modules connect to same CAN bus (CAN_H/CAN_L)

**Complexity**: Medium

---

### Scenario 3: ECU + AC Display

**Goal**: Engine swap + factory AC display control

**Modules**:
- ECU_Module (primary)
- AC_Display_Module (AC control)

**Communication**:
- **Option A**: Standalone (no communication)
- **Option B**: Serial (Leonardo Serial1 ↔ Mega Serial3)
- **Option C**: CAN bus (AC Display reads 0x201, 0x420)

**Hardware**:
- Arduino Leonardo + MCP2515 (ECU)
- Arduino Mega 2560 (AC Display)

**Wiring**:
- **Option A**: None (both independent)
- **Option B**: TX1→RX3, RX0→TX3, common ground
- **Option C**: CAN_H/CAN_L shared bus

**Complexity**: Medium

---

### Scenario 4: Full Monitoring Setup

**Goal**: Comprehensive monitoring with all displays and sensors

**Modules**:
- ECU_Module (primary)
- AC_Display_Module (factory display control)
- ESP8266_Companion (WiFi logging)
- Aftermarket_Display_Module (aftermarket gauges)
- Coolant_Monitor_Module (dedicated temp/pressure)

**Communication**:
```
ECU_Module ──[CAN]──→ Aftermarket_Display_Module
     │                        ↓
     │                   (reads data)
     │
     └─[Serial]──→ AC_Display_Module ──[Serial]──→ ESP8266_Companion
                          ↓                              ↓
                    (displays data)                (logs to SD/WiFi)

Coolant_Monitor_Module (standalone with dedicated sensors)
```

**Hardware**:
- Arduino Leonardo + MCP2515 (ECU)
- Arduino Mega 2560 (AC Display)
- ESP8266 NodeMCU (WiFi logging)
- Arduino Uno + MCP2515 + OLED displays (Aftermarket Display)
- Arduino Pro Micro + custom PCB + sensors (Coolant Monitor)

**Complexity**: High

---

### Scenario 5: Electric Vehicle Conversion

**Goal**: EV conversion with comprehensive monitoring

**Modules**:
- EV_ECU_Module (primary, maps motor data to dashboard)
- AC_Display_Module (display battery voltage as custom page)
- Aftermarket_Display_Module (kW, efficiency, battery stats)

**Communication**:
```
Open Inverter ──[CAN]──→ EV_ECU_Module ──[CAN]──→ RX8 Dashboard
                              │
                              │
                              └─[CAN]──→ Aftermarket_Display_Module
                              │              (battery stats)
                              │
                              └─[Serial]──→ AC_Display_Module
                                         (battery voltage display)
```

**Hardware**:
- Arduino Nano + MCP2515 (EV ECU)
- Arduino Mega 2560 (AC Display)
- Arduino Uno + MCP2515 + OLED displays (Aftermarket Display)

**Complexity**: High

---

### Scenario 6: Track Day Telemetry

**Goal**: Racing data logging and display

**Modules**:
- ECU_Module (primary)
- ESP8266_Companion (log to SD/WiFi)
- Aftermarket_Display_Module (real-time lap display)

**Communication**:
```
ECU_Module ──[CAN]──→ RX8 Dashboard
     │
     │
     └─[CAN]──→ Aftermarket_Display_Module (real-time display)
     │
     └─[Serial]──→ AC_Display_Module ──[Serial]──→ ESP8266_Companion
                                                   (SD card logging)
```

**Data Logged**:
- Engine RPM, speed, throttle (0x201)
- Coolant temp, warning lights (0x420)
- Wheel speeds (0x4B1)
- Steering angle (0x4BE)
- Accelerometer data (0x075)

**Complexity**: High

---

### Scenario 7: Sim Racing Setup

**Goal**: Use real RX8 cluster with PC racing games

**Modules**:
- Sim_Racing_Module (standalone)

**Communication**:
- PC (Python telemetry) ──[USB Serial]──→ Arduino ──[CAN/Direct]──→ RX8 Cluster

**Hardware**:
- Arduino Leonardo/Uno + MCP2515
- RX8 instrument cluster (removed from vehicle or spare)
- PC with Forza Horizon 5 or Dirt Rally 2.0

**Complexity**: Medium (requires cluster removal)

---

## Hardware Pinout Quick Reference

### ECU_Module (Arduino Leonardo)
- **CAN_CS**: Pin 17
- **CAN_INT**: Pin 2
- **Throttle Input**: A1 (analog)
- **Throttle Output**: Pin 5 (PWM)
- **LED Status**: Pins 7, 8
- **Serial1**: TX1/RX0 (optional for data sharing)

### EV_ECU_Module (Arduino Nano)
- **CAN_CS**: Pin 10
- **CAN_INT**: Pin 2
- **PWM Motor Control**: Pin 6 (8kHz PWM)
- **BMS Input**: Pin 3
- **LED Status**: Pin 8

### AC_Display_Module (Arduino Mega 2560)
- **Encoders**: Pins 2, 3, 18, 19 (interrupt-capable)
- **Button Matrix**: Pins 22-25 (rows), 29, 31 (columns)
- **SPI Display**: Pins 51 (MOSI), 52 (SCK), 53 (SS)
- **Serial AC Amp**: Pins 16 (TX), 17 (RX)
- **Serial3**: TX3/RX3 (for ESP8266 or ECU module)
- **Backlight**: Pins 9, 12

### Aftermarket_Display_Module (Arduino Uno/Nano)
- **CAN_CS**: Pin 10
- **CAN_INT**: Pin 2
- **OLED SPI**: Pin 13 (SCK), Pin 11 (MOSI), Pin 6 (CS), Pin 8 (RST), Pin 7 (DC)

### Coolant_Monitor_Module (Arduino Pro Micro)
- **I2C Displays**: Multiplexed via I/O pins
- **Temperature Sensor**: Analog pin (thermistor with Steinhart-Hart)
- **Pressure Sensor**: Analog pin
- **Battery Voltage**: Analog pin (voltage divider)
- **Reference Voltage**: External voltage reference IC

### ESP8266_Companion
- **Serial to AC Display**: TX/RX (to Mega Serial3)
- **WiFi**: Built-in antenna
- **Bluetooth**: HC-05 module (optional)

---

## Power Distribution Reference

### 12V Vehicle Power (Fused)

```
[12V Battery]
     │
     ├──[1A Fuse]── ECU_Module (Leonardo 5V regulator)
     │
     ├──[1A Fuse]── AC_Display_Module (Mega 5V regulator)
     │
     ├──[1A Fuse]── Aftermarket_Display_Module (5V regulator)
     │
     └──[1A Fuse]── Coolant_Monitor_Module (DC-DC converter on PCB)
```

### 5V Regulated (for ESP8266)

```
[12V Vehicle] ──[Buck Converter]── [5V] ── ESP8266 (3.3V onboard regulator)
```

---

## CAN Bus Wiring Topology

### Recommended (Linear Bus)

```
[ECU_Module] ─────┬───── CAN_H (twisted pair) ─────┬───── [Dashboard]
    (120Ω)        │                                │        (120Ω)
                  │                                │
                  ├─── [Aftermarket_Display]       │
                  │         (no termination)       │
                  │                                │
                  └───── CAN_L (twisted pair) ─────┘
```

**Rules**:
- 120Ω termination at **both ends only** (ECU and Dashboard)
- Twisted pair wire (minimum 24 AWG)
- Keep stub lengths short (<1 meter from main bus)
- Common ground for all modules

### Not Recommended (Star Topology)

```
             [ECU_Module]
                  │
         ┌────────┼────────┐
         │        │        │
    [Display1] [Display2] [Dashboard]
```
**Avoid**: Star topologies cause signal reflections at 500 kbps

---

## Serial Communication Examples

### ECU to AC Display (Leonardo → Mega)

**Wiring**:
- Leonardo TX1 (pin 1) → Mega RX3 (pin 15)
- Leonardo RX0 (pin 0) → Mega TX3 (pin 14)
- Common ground

**Leonardo Code** (send RPM):
```cpp
Serial1.print("$RPM:");
Serial1.println(engineRPM);
```

**Mega Code** (receive RPM):
```cpp
if(Serial3.available()) {
  String data = Serial3.readStringUntil('\n');
  if(data.startsWith("$RPM:")) {
    int rpm = data.substring(5).toInt();
  }
}
```

### AC Display to ESP8266 (Mega → ESP8266)

**Wiring**:
- Mega TX3 (pin 14) → ESP8266 RX
- Mega RX3 (pin 15) → ESP8266 TX
- Common ground
- ESP8266 powered by 5V (not 12V!)

**Mega Code** (send battery voltage):
```cpp
float voltage = analogRead(A4) * VOLTAGE_CONVERSION_FACTOR;
Serial3.print("$VBAT:");
Serial3.println(voltage, 2);
```

**ESP8266 Code** (receive and log):
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

## Module Data Capabilities

### What Each Module Can Provide

| Module | Provides | Via |
|--------|----------|-----|
| **ECU_Module** | RPM, speed, throttle, coolant temp, wheel speeds, warning lights | CAN (0x201, 0x420) |
| **EV_ECU_Module** | Motor RPM (as engine RPM), inverter temp (as coolant), power output | CAN (0x201, 0x420) |
| **AC_Display_Module** | Battery voltage, ambient temp, menu system | Serial |
| **ESP8266_Companion** | WiFi data logging, OBD-II data, timestamp | Serial |
| **Aftermarket_Display_Module** | 15+ OBD-II parameters (coolant, IAT, MAF, throttle, fuel, etc.) | Display only |
| **Coolant_Monitor_Module** | Coolant temp (precise), coolant pressure, battery voltage | OLED display / Serial |
| **Sim_Racing_Module** | Game telemetry (speed, RPM, gear, etc.) | PC via USB |

---

## Common Integration Questions

### Q: Can I run ECU_Module and EV_ECU_Module together?
**A**: No. These are mutually exclusive - they both transmit on the same CAN message IDs (0x201, 0x420, etc.). Choose one based on your powertrain (ICE or EV).

### Q: How many displays can I add?
**A**: As many as you want, as long as they only **read** the CAN bus (don't transmit). Examples: AC_Display, Aftermarket_Display, and Coolant_Monitor can all run together.

### Q: Do I need CAN bus for everything?
**A**: No. Coolant_Monitor_Module and Wipers_Module work standalone. Sim_Racing_Module uses USB to PC.

### Q: Can I share data between modules without CAN?
**A**: Yes, use serial communication. Example: ECU_Module sends data to AC_Display_Module via Serial1→Serial3.

### Q: What if I want to add my own custom module?
**A**: Follow these guidelines:
- Don't transmit on existing CAN IDs (0x201, 0x420, 0x620, 0x630, 0x041, 0x047)
- Use unused CAN ID range (0x500-0x5FF available)
- Or use serial communication (`$CMD:VAL\n` format)
- Document in CLAUDE.md and this cross-reference

### Q: How do I avoid CAN bus conflicts?
**A**: Only **one** module should transmit on each CAN message ID. Multiple modules can receive (read-only). If you want two modules to share data, use serial communication instead.

### Q: Can I use Bluetooth instead of WiFi?
**A**: Yes. ESP8266_Companion supports optional HC-05 Bluetooth module. Connect to ESP8266 Serial and configure for Bluetooth SPP.

---

## Module Upgrade Paths

### Start with ECU → Add monitoring

1. **Stage 1**: ECU_Module only (basic functionality)
2. **Stage 2**: Add Aftermarket_Display_Module (gauges)
3. **Stage 3**: Add AC_Display_Module (factory display control)
4. **Stage 4**: Add ESP8266_Companion (WiFi logging)
5. **Stage 5**: Add Coolant_Monitor_Module (precise temp/pressure)

### Start with EV → Add monitoring

1. **Stage 1**: EV_ECU_Module only (basic EV functionality)
2. **Stage 2**: Add AC_Display_Module with battery voltage page
3. **Stage 3**: Add Aftermarket_Display_Module for kW/efficiency
4. **Stage 4**: Add ESP8266_Companion for battery data logging

---

## Troubleshooting by Module Combination

### ECU + Aftermarket Display

**Problem**: Display shows wrong values
- Check CAN bus wiring (CAN_H/CAN_L not swapped)
- Verify both modules use 500 kbps CAN speed
- Confirm Aftermarket Display decodes messages correctly (RPM = raw / 3.85)

**Problem**: CAN bus errors
- Add 120Ω termination resistors at both ends
- Check stub length (<1 meter from main bus)
- Verify common ground

### ECU + AC Display (Serial)

**Problem**: No data received
- Check TX/RX crossed correctly (TX1→RX3, RX0→TX3)
- Verify baud rate match (115200 on both)
- Confirm common ground

**Problem**: Garbled data
- Reduce cable length (<3 feet for 115200 baud)
- Check for electrical noise (add capacitors)
- Verify voltage levels (both 5V logic)

### AC Display + ESP8266 (Serial)

**Problem**: ESP8266 won't connect
- Verify ESP8266 powered by **5V** (not 12V!)
- Check ESP8266 onboard 3.3V regulator output
- Confirm WiFi credentials in code

**Problem**: Data logging fails
- Check SD card formatting (FAT32)
- Verify SD card CS pin in code
- Ensure sufficient power (ESP8266 + SD = ~300mA)

---

## Related Documentation

- **[Documentation/INTEGRATION_GUIDE.md](../Documentation/INTEGRATION_GUIDE.md)** - Comprehensive integration scenarios and examples
- **[CLAUDE.md](../CLAUDE.md)** - Complete developer guide
- **[Documentation/CAN_PID_Reference.md](../Documentation/CAN_PID_Reference.md)** - CAN protocol reference
- **Individual Module READMEs** - Module-specific documentation

---

*Last Updated: 2025-11-15*
*For questions: See individual module README files*
