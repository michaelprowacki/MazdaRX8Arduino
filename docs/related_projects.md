# Related RX8 Projects

This document catalogs related Mazda RX8 Arduino and reverse engineering projects that complement or provide alternatives to the MazdaRX8Arduino project.

---

## Project Relationship to MazdaRX8Arduino

**This Project (MazdaRX8Arduino)**:
- **Purpose**: Complete ECU/PCM replacement using Arduino
- **Hardware**: Arduino Leonardo + Arduino Mega 2560 (AC Display)
- **Use Case**: Engine swaps, aftermarket ECU, custom control systems

The projects below are organized by how they relate to our project.

---

## CATEGORY 1: Direct Companions

### ESP8266 Companion for AC Display

**Repository**: https://github.com/michaelprowacki/S1-RX8-AC-Display-ESP8266-Companion

**Status**: ✅ Integrated as `AC_Display_Module/ESP8266_Companion/`

**Purpose**: Adds WiFi/Bluetooth to AC Display Module

**Key Features**:
- Persistent data storage
- Bluetooth communication
- ESP-Now wireless protocol
- Data logging capabilities
- Mobile app connectivity

**Integration**: Git submodule or direct copy
**Documentation**: `AC_Display_Module/ESP8266_Companion/README.md`

---

## CATEGORY 2: Electric Vehicle (EV) Conversion

### 2.1 RX8-dash-CAN-controller (EV Conversion)

**Repository**: https://github.com/EV8-Steve/RX8-dash-CAN-controller

**Status**: 📚 Reference for EV conversion use case

**Purpose**: RX8 dashboard integration for electric vehicle conversions

**Key Features**:
- Arduino Nano based CAN controller
- Integrates with Open Inverter (popular EV motor controller)
- ABS system integration
- Power Assisted Steering (PAS) support
- 8kHz PWM for motor control
- Odometer pulse generation
- Wheel speed monitoring
- Dashboard warning light control

**Technical Details**:
- **Version**: 1.4 (8kHz PWM, microsecond-adjusted)
- **Hardware**: Arduino Nano
- **CAN Speed**: 500 kbps
- **Messages**: Same as factory (0x201, 0x420, etc.)
- **Special**: Converts EV motor data to ICE-equivalent signals

**How It Relates**:
- **Very similar** to MazdaRX8Arduino project
- Same CAN messages, different data source
- Replaces engine with electric motor
- Maps inverter temperature → coolant temperature
- Maps motor RPM → engine RPM
- Adds odometer pulse output

**Relevance**: ⭐⭐⭐⭐⭐ HIGH - Nearly identical use case

**Integration Potential**:
- Could merge EV-specific features
- Add optional "EV mode" to our project
- Extract odometer pulse code
- Reference for Open Inverter integration

**Use Case**: Engine swap where "engine" is an electric motor

**Languages**: C++ (100%)
**Created**: April 2023
**Activity**: 1 commit (working implementation)

---

## CATEGORY 3: CAN Bus Reference & Documentation

### 3.1 RX8 Reverse Engineering

**Repository**: https://github.com/rnd-ash/rx8-reverse-engineering

**Status**: ✅ DBC file integrated as `Documentation/rx8_can_database.dbc`

**Purpose**: Comprehensive RX8 CAN bus reverse engineering

**Key Resources**:
- **rx8.dbc**: CAN database file with signal definitions
- **Wiki**: Extensive documentation of findings
- **Community**: Active reverse engineering community

**How We Use It**:
- Validate our CAN message implementations
- Reference for signal names and definitions
- Tool compatibility (SavvyCAN, Kayak, etc.)

**25 stars** | **4 forks** | DBC format

---

### 3.2 RX8 CAN Bus PIDs for Racing

**Repository**: https://github.com/topolittle/RX8-CAN-BUS

**Status**: ✅ Documentation integrated as `Documentation/CAN_PID_Reference.md`

**Purpose**: CAN PIDs for telemetry and racing applications

**Key Features**:
- 9 documented CAN PIDs with formulas
- Tested with OBDLink MX+ and RaceChrono
- RPM, throttle, brake, steering angle, temperatures
- Racing telemetry configuration examples

**How We Use It**:
- Cross-reference our CAN implementations
- Additional PIDs we could add (steering, brake)
- Validation of conversion formulas

**Test Vehicle**: 2006 RX-8 Series 1 MT
**Equipment**: OBDLink MX+, RaceChrono v7.5.3

---

### 3.3 RX8-Dash CAN Protocol

**Repository**: https://github.com/Antipixel/RX8-Dash

**Status**: 📚 Reference for code validation

**Purpose**: Complete CAN protocol for RX8 instrument cluster

**Key Features**:
- Detailed message format documentation
- Bit-level flag definitions
- Warning light control mappings
- C++/PlatformIO implementation

**How We Use It**:
- Validate our message structures
- Extract detailed bit-field documentation
- Compare implementation approaches

**Languages**: C++ (81.5%), C (18.5%)
**Framework**: PlatformIO

---

## CATEGORY 4: Alternative Displays & Monitoring

### 4.1 Arduino OBD2 Display

**Repository**: https://github.com/Radivv/arduino-display-obd2-can-mazda-rx8

**Status**: 📚 Reference for aftermarket display ideas

**Purpose**: Aftermarket OBD2 display for RX8

**Key Features**:
- 15 monitored parameters
- 1.5" RGB OLED displays
- MCP2515 CAN interface
- Engine diagnostics focus

**Relevance**: Different use case (aftermarket display vs. ECU replacement), but good reference for sensor monitoring algorithms

**Hardware**: Arduino + MCP2515 + WaveShark OLED
**12 stars** | C++ (100%)

---

### 4.2 RX8 Coolant Monitor

**Repository**: https://github.com/topolittle/rx8-coolant-monitor

**Status**: 📚 Reference for specialized monitoring

**Purpose**: Dedicated coolant temperature/pressure monitoring

**Key Features**:
- 3x 128x32 OLED displays
- Arduino Pro Micro based
- Steinhart-Hart thermistor equation
- Custom PCB + 3D-printed enclosure
- Pressure sensor integration

**Relevance**: Addresses RX8-specific weakness (poor coolant monitoring). Users wanting this feature can build it separately.

**Languages**: C (49.9%), C++ (45.1%)
**License**: 3-clause BSD

---

## CATEGORY 5: ECU Tuning Tools

### 5.1 RX8 ECU Dump Tool

**Repository**: https://github.com/ConnorRigby/rx8-ecu-dump

**Status**: 📚 Pre-installation recommendation

**Purpose**: Dump and flash RX8 ECU firmware via J2534

**Key Features**:
- ROM extraction (~2 min)
- RAM snapshots (~5 sec)
- Free alternative to commercial tools
- Work in progress (flashing incomplete)

**Relevance**: **Recommended BEFORE installing Arduino ECU replacement**
- Backup original ECU firmware
- Reference original calibrations
- Restore factory operation if needed

**Languages**: C (88.6%), C++ (10.2%)
**50 commits** | Open-source

---

### 5.2 RX8Defs - ECU Definitions

**Repository**: https://github.com/Rx8Man/RX8Defs

**Status**: 📚 Reference for factory ECU tuners

**Purpose**: Open-source ECU definitions for tuning software

**Key Features**:
- ECUFlash definitions
- RomRaider definitions
- 8 supported calibrations (2004-2005 RX8s)
- European, US, JDM variants

**Relevance**: For users who want to **tune** factory ECU instead of **replacing** it. Different approach than our project.

**10 stars** | **3 forks** | **50 commits**

---

### 5.3 Rx8Man Reflash Tool

**Repository**: https://github.com/Rx8Man/Rx8Man

**Status**: 📚 Alternative approach reference

**Purpose**: Minimal ECU reflashing tool for Series 1 RX8

**Key Features**:
- Tactrix OpenPort interface
- ECU firmware flashing
- Simple GUI tool
- Latest: v1.05 (Oct 2023)

**Relevance**: Alternative to Arduino replacement - modify factory ECU software instead

**14 stars** | **1 fork** | Binary release

---

### 5.4 Mazda RX8 PCM Reverse Engineering

**Repository**: https://github.com/equinox311/Mazda_RX8_PCM_ReverseEngineering

**Status**: 📚 Advanced reference

**Purpose**: Reverse engineering ECU firmware using Ghidra

**Key Contents**:
- Ghidra project archives
- Stock ROM files
- Live data captures
- Memory table identification

**Relevance**: Advanced users interested in understanding factory ECU internals

**Languages**: Python (100%)

---

### 5.5 GROM RomRaider

**Repository**: https://github.com/equinox311/GROM_RomRaider

**Status**: 📚 Tuning software reference

**Purpose**: RomRaider modified for Mazda RX8 CAN support

**Key Features**:
- Subaru tuning suite adapted for RX8
- Data logging capabilities
- ECU viewing and calibration
- Professional tuner interface

**Relevance**: Educational tool to understand factory ECU behavior

**Languages**: Java (99.7%)
**License**: GPL-2.0

---

## CATEGORY 6: Specialized Components

### 6.1 RX8 Wipers Speed-Sensitive

**Repository**: https://github.com/basilhussain/rx8-wipers

**Status**: 📚 Related project (out of scope)

**Purpose**: Add speed-sensitive intermittent wipers

**Key Features**:
- Speed-sensitive wiper control
- Firmware + hardware design
- Reads vehicle speed from CAN
- Modifies wiper timing

**Relevance**: Example of CAN bus usage for automotive enhancement. Out of scope for ECU replacement but interesting reference.

**7 stars** | C (79.7%), JavaScript (12.8%), HTML (7.5%)

---

## CATEGORY 7: Entertainment & Testing

### 7.1 RX8-Arduino Sim Racing

**Repository**: https://github.com/Izekeal/rx8-arduino

**Status**: 📚 Interesting but different use case

**Purpose**: Drive RX8 instrument cluster from sim racing games

**Key Features**:
- Forza Horizon 5 integration
- Dirt Rally 2.0 support
- Telemetry to CAN conversion
- Handbrake button support

**Relevance**:
- Fun project showing CAN protocol from different angle
- Could inspire test bench ideas
- Simulate vehicle behavior without running engine

**12 commits** | Python (40.2%), C++ (35.6%), C (24.2%)
**License**: MIT

---

## Comparison Matrix

| Project | Use Case | Hardware | Replaces ECU? | Stars |
|---------|----------|----------|---------------|-------|
| **MazdaRX8Arduino** (This) | ECU Replacement (ICE) | Leonardo + Mega | ✅ Yes | - |
| **RX8-dash-CAN-controller** | ECU Replacement (EV) | Arduino Nano | ✅ Yes | 0 |
| ESP8266 Companion | WiFi/BT for AC Display | ESP8266 | ❌ No | 2 |
| rx8-reverse-engineering | CAN Documentation | None (docs) | ❌ No | 25 |
| RX8-CAN-BUS | Telemetry PIDs | OBD scanner | ❌ No | - |
| RX8-Dash | Cluster Control | Arduino | 🔶 Partial | - |
| Arduino OBD2 Display | Aftermarket Display | Arduino + OLED | ❌ No | 12 |
| Coolant Monitor | Coolant Display | Pro Micro | ❌ No | - |
| rx8-ecu-dump | ECU Backup | J2534 interface | ❌ No | - |
| RX8Defs | ECU Tuning Defs | Computer | ❌ No | 10 |
| Rx8Man | ECU Reflashing | Tactrix | ❌ No | 14 |
| PCM Reverse Engineering | ECU Analysis | Computer + Ghidra | ❌ No | - |
| GROM RomRaider | ECU Tuning | Computer | ❌ No | - |
| rx8-wipers | Wiper Enhancement | Arduino | ❌ No | 7 |
| rx8-arduino | Sim Racing | Arduino | ❌ No | - |

---

## When to Use What

### You want to... Use this project:

**Replace the ECU (gasoline engine)** → **MazdaRX8Arduino** (this project)
- Engine swaps (rotary, LS, JZ, etc.)
- Aftermarket engine management
- Complete control system replacement

**Replace the ECU (electric motor)** → **RX8-dash-CAN-controller** (EV8-Steve)
- Electric vehicle conversion
- Open Inverter integration
- Map motor data to dashboard
- ICE → EV conversion

**Add WiFi/Bluetooth to AC Display** → **ESP8266 Companion**
- Data logging
- Mobile app connectivity
- Wireless features

**Tune your factory ECU** → **RX8Defs + Rx8Man + GROM RomRaider**
- Keep factory ECU
- Modify calibrations
- Professional tuning

**Backup ECU before modifications** → **rx8-ecu-dump**
- Save original firmware
- Peace of mind
- Restore capability

**Learn about CAN bus** → **rx8-reverse-engineering + RX8-CAN-BUS**
- Understand protocols
- Reference documentation
- Tool compatibility

**Add aftermarket display** → **Arduino OBD2 Display**
- Extra gauges
- Monitor parameters
- Dashboard enhancement

**Monitor coolant closely** → **Coolant Monitor**
- Temperature and pressure
- RX8-specific solution
- Dedicated display

**Control instrument cluster** → **RX8-Dash**
- Custom dashboard
- Standalone cluster
- Display testing

**Enhance wipers** → **rx8-wipers**
- Speed-sensitive
- Comfort feature
- CAN integration example

**Use cluster with sim racing** → **rx8-arduino**
- Entertainment
- Immersive gaming
- Test bench

---

## Integration Status Summary

### ✅ Integrated
- ESP8266 Companion (module structure + docs)
- rx8.dbc (CAN database file)
- CAN PID Reference (comprehensive documentation)

### 📚 Referenced
- All other projects linked in documentation
- Categorized by use case
- Clear relationship to main project

### 🚫 Not Integrated (Out of Scope)
- ECU tuning tools (different approach)
- Specialized components (wipers, sim racing)
- Aftermarket displays (different hardware)

---

## Contributing

If you know of other RX8 Arduino or reverse engineering projects that should be listed here, please:

1. Open an issue with the repository link
2. Describe how it relates to this project
3. Include key features and status

We maintain this list to help the RX8 community find the right tools for their needs.

---

## Credits

**Analysis and Integration**: Claude (AI Assistant) for MazdaRX8Arduino project
**Original Projects**: See individual repository links above
**Community**: RX8 enthusiasts, tuners, and developers worldwide

---

*Last Updated: 2025-11-15*
*Part of MazdaRX8Arduino Documentation*
