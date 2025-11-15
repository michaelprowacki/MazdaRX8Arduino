# EV ECU Module - Electric Vehicle Conversion

## Overview

Arduino-based ECU for **electric vehicle conversions** using the RX8 chassis. Maps electric motor data to factory dashboard signals.

**Source**: https://github.com/EV8-Steve/RX8-dash-CAN-controller
**Author**: EV8-Steve
**License**: As per original repository

---

## Purpose

Controls RX8 dashboard when converting to electric power:
- Maps motor RPM → engine RPM display
- Maps inverter temperature → coolant temperature
- Generates odometer pulses
- Maintains ABS/DSC functionality
- Enables power steering

---

## Hardware

- **Microcontroller**: Arduino Nano (ATmega328P)
- **CAN Controller**: MCP2515
- **Motor Controller**: Open Inverter (or compatible)
- **PWM Output**: 8kHz for motor control

---

## Key Features

⚡ **Open Inverter Integration** - Reads motor controller CAN bus
⚡ **Odometer Pulse Generation** - Proper speedometer operation
⚡ **8kHz PWM** - High-frequency motor control
⚡ **Temperature Mapping** - Inverter → coolant temp
⚡ **Same CAN Messages** - Uses factory 0x201, 0x420, etc.

---

## Differences from ICE ECU Module

| Feature | ICE ECU | EV ECU |
|---------|---------|--------|
| Hardware | Leonardo | Nano |
| Data Source | Engine sensors | Motor controller CAN |
| Odometer | Via wheel speed | Generated pulses |
| PWM Output | No | Yes (8kHz motor control) |

---

## Integration with Open Inverter

Reads these messages from inverter:
- Motor RPM
- Motor temperature
- Motor controller status
- Power output

Maps to RX8 dashboard as:
- Engine RPM
- Coolant temperature
- "Engine" status
- Throttle position

---

## Quick Start

1. Connect to Open Inverter CAN bus
2. Connect to RX8 dashboard CAN bus
3. Upload firmware to Arduino Nano
4. Calibrate motor RPM scaling
5. Test dashboard function

---

## See Also

- **ECU_Module** - ICE version (similar code structure)
- **Documentation/CAN_PID_Reference.md** - CAN message details
- **Open Inverter Documentation** - Motor controller setup

---

*Integrated from EV8-Steve's project - see RX8_ECOSYSTEM.md for analysis*
