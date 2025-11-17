# Dash Controller Module - Alternative Implementation

## Overview

Alternative RX8 dashboard CAN controller implementation with detailed protocol documentation.

**Source**: https://github.com/Antipixel/RX8-Dash
**Author**: Antipixel
**Language**: C++ (PlatformIO)

---

## Purpose

Reference implementation for:
- Complete CAN message documentation
- Bit-level flag definitions
- Warning light control
- Dashboard protocol validation

---

## Key Features

📋 **Comprehensive Documentation** - Every CAN message bit documented
📋 **Message Formats** - 0x420, 0x201, 0x650, 0x231, 0x300, 0x212
📋 **PlatformIO Project** - Modern build system
📋 **C++ Classes** - Object-oriented structure

---

## CAN Messages Documented

- **0x201** - PCM Status (RPM, Speed, Throttle)
- **0x420** - Warning Lights & Temperature
- **0x650** - Cruise Control Indicators
- **0x231** - Transmission Gear Display
- **0x300** - Electronic Power Steering
- **0x212** - ABS/Traction/Stability Systems

---

## Use Cases

### 1. Reference Implementation
Cross-check bit definitions with main ECU_Module

### 2. Alternative Build System
PlatformIO instead of Arduino IDE

### 3. Code Structure Ideas
Object-oriented approach to CAN handling

---

## Building

```bash
cd Dash_Controller_Module
pio run              # Build
pio run -t upload    # Upload
```

---

## Integration Notes

**Overlaps with ECU_Module**:
- Same CAN messages (0x201, 0x420, etc.)
- Different code structure (classes vs functions)
- More detailed bit-field documentation

**Use This Module To**:
- Validate ECU_Module implementation
- Extract missing bit definitions
- Learn alternative code organization

---

## See Also

- **ECU_Module** - Primary ICE ECU implementation
- **Documentation/CAN_PID_Reference.md** - Complete protocol reference

---

*Integrated from Antipixel/RX8-Dash - see docs/related_projects.md*
