# Mazda RX8 Arduino Electronics Repository

**The comprehensive, ubiquitous RX8 Arduino electronics repository** - integrating ECU replacement, display controllers, monitoring systems, and development tools for the Mazda Mark 1 RX8.

---

## Overview

This repository contains **everything you need** to replace, enhance, or control the electronics in a Mazda RX8 using Arduino-based systems. From complete ECU replacement for engine swaps to aftermarket displays, telemetry logging, and reverse engineering tools.

### What's Included

- ✅ **ECU Replacement Modules** - Replace factory ECU for ICE or EV conversions
- ✅ **Display Controllers** - Factory AC display, aftermarket OLED gauges, coolant monitors
- ✅ **Specialized Function Modules** - Speed-sensitive wipers, sim racing cluster drivers
- ✅ **Development Tools** - CAN protocol documentation, ECU tuning definitions, reverse engineering tools
- ✅ **Complete Integration Guides** - How to combine modules for your specific use case

---

## Quick Start

### Choose Your Use Case

#### 🔧 **Engine Swap (ICE)** → Start with [ECU_Module](ECU_Module/)
Replace the factory ECU while maintaining dashboard, ABS, power steering, and immobilizer.
- **Hardware**: Arduino Leonardo + MCP2515 CAN controller
- **Status**: Production-ready, extensively tested
- **Documentation**: [ECU_Module/README.md](ECU_Module/README.md)

#### ⚡ **Electric Vehicle Conversion** → Start with [EV_ECU_Module](EV_ECU_Module/)
Convert RX8 to electric power using Open Inverter or similar motor controllers.
- **Hardware**: Arduino Nano + MCP2515 CAN controller
- **Status**: Production-ready for EV conversions
- **Documentation**: [EV_ECU_Module/README.md](EV_ECU_Module/README.md)

#### 📊 **Add Aftermarket Displays** → See [Aftermarket_Display_Module](Aftermarket_Display_Module/)
RGB OLED displays showing 15+ engine parameters via CAN bus.
- **Works alongside**: Any ECU module
- **Hardware**: Arduino + MCP2515 + RGB OLED displays
- **Documentation**: [Aftermarket_Display_Module/README.md](Aftermarket_Display_Module/README.md)

#### 🌡️ **Accurate Coolant Monitoring** → See [Coolant_Monitor_Module](Coolant_Monitor_Module/)
The RX8's factory coolant gauge is inadequate. This module provides precise temperature and pressure monitoring.
- **Works**: Standalone (dedicated sensors) or alongside any ECU
- **Includes**: Firmware, PCB designs, 3D-printable enclosures
- **Documentation**: [Coolant_Monitor_Module/README.md](Coolant_Monitor_Module/README.md)

#### 🎮 **Sim Racing** → See [Sim_Racing_Module](Sim_Racing_Module/)
Drive a real RX8 instrument cluster from Forza Horizon 5 or Dirt Rally 2.0.
- **Works**: Standalone (PC-connected)
- **Documentation**: [Sim_Racing_Module/README.md](Sim_Racing_Module/README.md)

---

## Repository Structure

```
MazdaRX8Arduino/
│
├── ECU_Module/                      # Primary ECU replacement (ICE engines)
│   ├── RX8_CANBUS.ino              # Main Arduino sketch
│   └── README.md                    # Module documentation
│
├── EV_ECU_Module/                   # Electric vehicle ECU replacement
│   ├── rx8can_v1.4__8khz_pwm_adjusted_micros_.ino
│   └── README.md
│
├── Dash_Controller_Module/          # Alternative dashboard implementation
│   ├── main.cpp                     # PlatformIO project
│   └── README.md                    # Reference implementation docs
│
├── AC_Display_Module/               # Factory AC display controller
│   ├── src/main.cpp                 # Main code
│   ├── ESP8266_Companion/           # WiFi/Bluetooth add-on
│   │   ├── src/main.cpp            # ESP8266 firmware
│   │   └── platformio.ini          # PlatformIO config
│   └── README.md                    # Module + ESP8266 docs
│
├── Aftermarket_Display_Module/      # OBD2 OLED display (15+ parameters)
│   ├── additional_display/          # Arduino sketch
│   └── README.md
│
├── Coolant_Monitor_Module/          # Dedicated temp/pressure monitor
│   ├── EMBEDDED/src/                # Arduino firmware
│   ├── ELEC/                        # PCB designs (KiCad)
│   ├── MECHA/                       # 3D models (STEP, STL)
│   └── README.md                    # Complete build guide
│
├── Wipers_Module/                   # Speed-sensitive wiper control
│   └── README.md
│
├── Sim_Racing_Module/               # Sim racing cluster driver
│   ├── src/forza/forza.ino         # Forza Horizon 5 support
│   ├── src/dirt/dirt.ino           # Dirt Rally 2.0 support
│   └── README.md
│
├── Tools/
│   ├── PCM_Analysis/                # Ghidra reverse engineering
│   │   ├── Ghidra_Archives/        # ECU firmware analysis
│   │   ├── Stock_ROMs/             # Factory ROM dumps
│   │   └── README.md
│   │
│   └── ECU_Definitions/             # ECUFlash/RomRaider tuning defs
│       ├── *.xml                    # 8 ECU calibration definitions
│       └── README.md
│
├── Documentation/
│   ├── INTEGRATION_GUIDE.md         # How modules work together
│   ├── CAN_PID_Reference.md         # Complete CAN protocol reference
│   ├── rx8_can_database.dbc         # CAN signal definitions (DBC format)
│   ├── PDFs/                        # Factory service manuals
│   │   ├── 08_Steering.pdf
│   │   ├── 13Electrical.pdf
│   │   ├── Engine-Manual.pdf
│   │   └── RX8 CanBus and Throttle Explained.pdf
│   └── Excel files/                 # CAN bus data captures
│
├── docs/
│   ├── related_projects.md          # Catalog of 15 RX8 projects
│   └── MODULE_CROSS_REFERENCE.md    # Module compatibility matrix
│
├── CLAUDE.md                        # AI assistant guide (comprehensive)
└── RX8_ECOSYSTEM.md                 # Integration analysis for 15 repos
```

---

## Documentation Hub

### For Users (Start Here)

- **[Documentation/INTEGRATION_GUIDE.md](Documentation/INTEGRATION_GUIDE.md)** - How to combine modules for your specific setup
- **[docs/MODULE_CROSS_REFERENCE.md](docs/MODULE_CROSS_REFERENCE.md)** - Quick reference for module compatibility
- **Individual Module READMEs** - Detailed documentation for each module

### For Developers

- **[CLAUDE.md](CLAUDE.md)** - Comprehensive guide for AI assistants and developers (550+ lines)
- **[Documentation/CAN_PID_Reference.md](Documentation/CAN_PID_Reference.md)** - Complete CAN protocol reference (600+ lines)
- **[Documentation/rx8_can_database.dbc](Documentation/rx8_can_database.dbc)** - CAN signal definitions (import to SavvyCAN, Kayak, Wireshark)
- **[RX8_ECOSYSTEM.md](RX8_ECOSYSTEM.md)** - Integration analysis for 15 related repositories

### For Researchers

- **[Tools/PCM_Analysis/](Tools/PCM_Analysis/)** - Ghidra-based ECU firmware reverse engineering
- **[Tools/ECU_Definitions/](Tools/ECU_Definitions/)** - ECUFlash/RomRaider tuning definitions
- **[docs/related_projects.md](docs/related_projects.md)** - Catalog of all related RX8 projects

---

## Key Features

### ECU Replacement (ECU_Module / EV_ECU_Module)

- ✅ Complete CAN bus emulation (500 kbps high-speed CAN)
- ✅ Dashboard functionality (RPM, speed, temperature)
- ✅ Warning light control (check engine, oil pressure, coolant, battery)
- ✅ ABS/DSC integration (maintains factory safety systems)
- ✅ Immobilizer bypass (two-part handshake protocol)
- ✅ Power steering enabled
- ✅ Throttle pedal processing (1.7V - 4.0V input range)
- ✅ Wheel speed monitoring (front and rear)
- ✅ Vehicle-specific variations documented

### Additional Features (Other Modules)

- 📊 **Aftermarket Displays**: 15+ engine parameters on RGB OLED displays
- 🌡️ **Accurate Coolant Monitoring**: Custom PCB + 3D-printed enclosure + dedicated sensors
- 🎛️ **AC Display Control**: Custom menu pages (battery voltage, motor temp, etc.)
- 📡 **WiFi/Bluetooth**: ESP8266 companion for data logging and OTA updates
- 🏁 **Sim Racing**: Drive real RX8 cluster from PC games
- 💧 **Smart Wipers**: Speed-sensitive intermittent timing

---

## Hardware Requirements

### Minimum (ECU Replacement Only)

- **Microcontroller**: Arduino Leonardo (ATmega32U4) or Nano (for EV)
- **CAN Controller**: MCP2515 module
- **CAN Bus**: Connection to RX8 high-speed CAN (500 kbps)
- **Power**: 12V vehicle power (with 5V regulator)
- **Wiring**: Throttle pedal input/output (for ICE ECU)

### Recommended (Full Monitoring Setup)

- All of the above PLUS:
- Arduino Mega 2560 (for AC Display Module)
- Arduino Pro Micro + custom PCB (for Coolant Monitor)
- RGB OLED displays (for Aftermarket Display Module)
- ESP8266 NodeMCU (for WiFi/Bluetooth logging)
- Delphi temperature sensor + AEM pressure sensor (for Coolant Monitor)

See **[Documentation/INTEGRATION_GUIDE.md](Documentation/INTEGRATION_GUIDE.md)** for complete hardware compatibility matrix.

---

## Integration Scenarios

### Scenario 1: Minimal ECU Replacement
**Modules**: ECU_Module only
**Result**: Dashboard, ABS, power steering, immobilizer all functional

### Scenario 2: Full Monitoring Setup
**Modules**: ECU_Module + AC_Display_Module + Coolant_Monitor_Module + Aftermarket_Display_Module
**Result**: Complete dashboard + custom displays + accurate coolant monitoring + aftermarket gauges

### Scenario 3: Electric Vehicle Conversion
**Modules**: EV_ECU_Module + AC_Display_Module + Aftermarket_Display_Module
**Result**: Factory dashboard shows motor data as "engine RPM", plus EV-specific displays

### Scenario 4: Track Day Telemetry
**Modules**: ECU_Module + ESP8266_Companion + Aftermarket_Display_Module
**Result**: Live CAN data logging to SD card or WiFi, real-time lap display

See **[Documentation/INTEGRATION_GUIDE.md](Documentation/INTEGRATION_GUIDE.md)** for complete integration scenarios.

---

## CAN Bus Protocol

This repository uses the RX8's high-speed CAN bus (500 kbps). Key message IDs:

| ID (Hex) | ID (Dec) | Purpose | Update Rate |
|----------|----------|---------|-------------|
| 0x201 | 513 | PCM Status (RPM, Speed, Throttle) | 100ms |
| 0x420 | 1056 | Warning Lights, Engine Temp | 100ms |
| 0x4B0 | 1200 | Wheel Speeds (for ABS/DSC) | 10ms |
| 0x4B1 | 1201 | Wheel Speeds (for Dashboard) | 10ms |
| 0x047 | 71 | Immobilizer Request | On request |
| 0x041 | 65 | Immobilizer Response | On request |

**Complete Protocol**: See [Documentation/CAN_PID_Reference.md](Documentation/CAN_PID_Reference.md) for 600+ lines of detailed CAN message documentation.

**DBC File**: Import [Documentation/rx8_can_database.dbc](Documentation/rx8_can_database.dbc) into SavvyCAN, Kayak, or Wireshark for analysis.

---

## Safety Warnings

⚠️ **CRITICAL SAFETY INFORMATION** ⚠️

This code controls **safety-critical vehicle systems**. Any modifications must be thoroughly tested.

- **Throttle Safety**: Never modify throttle safety limits without understanding consequences
- **ABS/DSC**: Incorrect CAN messages can disable stability control systems
- **Warning Lights**: Ensure warning lights function correctly (oil pressure, coolant temp, etc.)
- **Bench Test First**: Test all modifications on bench before installing in vehicle
- **Backup Factory ECU**: Keep factory ECU for restoration if needed

**Use at your own risk**. The authors are not responsible for vehicle damage, personal injury, or death resulting from use of this code.

---

## Contributing

This repository integrates work from **15+ RX8 Arduino projects** and hundreds of hours of reverse engineering.

### How to Contribute

1. **Bug Reports**: Open an issue describing the problem
2. **Feature Requests**: Describe your use case and proposed solution
3. **Code Contributions**:
   - Test thoroughly on bench and in vehicle
   - Document CAN message changes
   - Update CLAUDE.md with integration notes
   - Follow existing code style

### Attribution

This repository integrates code from:
- **David Blackhurst** - Original ECU_Module (RX8_CANBUS.ino)
- **EV8-Steve** - EV_ECU_Module (electric vehicle conversion)
- **Antipixel** - Dash_Controller_Module (alternative dashboard)
- **michaelprowacki** - AC_Display_Module and ESP8266_Companion
- **Radivv** - Aftermarket_Display_Module (OLED gauges)
- **topolittle** - Coolant_Monitor_Module (PCB + 3D models)
- **basilhussain** - Wipers_Module (speed-sensitive wipers)
- **Izekeal** - Sim_Racing_Module (Forza/Dirt Rally support)
- **equinox311** - PCM_Analysis tools (Ghidra reverse engineering)
- **Rx8Man** - ECU_Definitions (ECUFlash/RomRaider definitions)
- **rnd-ash** - rx8_can_database.dbc (CAN signal definitions)

See individual module README files for specific attribution and licenses.

---

## Support the Authors

If this repository has saved you time or helped your project, please consider supporting the original authors:

- **David Blackhurst** (ECU_Module): https://www.paypal.me/DBlackhurst
- **topolittle** (Coolant Monitor): See [Coolant_Monitor_Module/README.md](Coolant_Monitor_Module/README.md)
- **Rx8Man** (ECU Definitions): https://www.buymeacoffee.com/RX8Man
- **equinox311** (PCM Analysis): See [Tools/PCM_Analysis/README.md](Tools/PCM_Analysis/README.md)

This repository represents **hundreds of hours** of:
- CAN bus reverse engineering
- Trial and error testing in vehicles
- Hardware debugging (several Arduinos damaged)
- Community collaboration

Your support helps fund future projects (like the Leaf RX8 Conversion mentioned by David Blackhurst).

---

## License

This repository contains code from multiple sources with varying licenses. See individual module directories for specific license information.

**General License**: Most modules use permissive licenses (BSD, MIT, GPL). Check each module's LICENSE or COPYING.TXT file.

**Exceptions**:
- Some 3D models have non-commercial use restrictions (see Coolant_Monitor_Module)
- Adafruit library derivatives maintain original BSD license
- Factory service manuals are copyrighted by Mazda Motor Corporation

---

## Related Projects

This repository integrates or references **15 RX8 Arduino projects**:

### Integrated (Source Code Included)

1. **EV8-Steve/RX8-dash-CAN-controller** - Electric vehicle conversion ECU
2. **Antipixel/RX8-Dash** - Alternative dashboard controller with detailed protocol docs
3. **michaelprowacki/S1-RX8-AC-Display-controller** - Factory AC display control
4. **michaelprowacki/S1-RX8-AC-Display-ESP8266-Companion** - WiFi/Bluetooth for AC display
5. **Radivv/arduino-display-obd2-can-mazda-rx8** - Aftermarket OLED gauge display
6. **topolittle/rx8-coolant-monitor** - Dedicated coolant temp/pressure monitor
7. **basilhussain/rx8-wipers** - Speed-sensitive wiper enhancement
8. **Izekeal/rx8-arduino** - Sim racing cluster driver
9. **equinox311/Mazda_RX8_PCM_ReverseEngineering** - Ghidra ECU analysis
10. **Rx8Man/RX8Defs** - ECUFlash/RomRaider tuning definitions

### Referenced (External Tools)

11. **rnd-ash/rx8-reverse-engineering** - CAN database (DBC file included)
12. **ConnorRigby/rx8-ecu-dump** - Free J2534 ECU backup tool
13. **equinox311/GROM_RomRaider** - RomRaider tuning software fork
14. **Rx8Man/Rx8Man** - ECU reflashing tool (Tactrix)

See **[docs/related_projects.md](docs/related_projects.md)** for complete categorized catalog.

See **[RX8_ECOSYSTEM.md](RX8_ECOSYSTEM.md)** for integration analysis and recommendations.

---

## Getting Started

### 1. Choose Your Primary Module

- **Engine swap (ICE)** → [ECU_Module](ECU_Module/)
- **EV conversion** → [EV_ECU_Module](EV_ECU_Module/)
- **Aftermarket displays** → [Aftermarket_Display_Module](Aftermarket_Display_Module/)
- **Coolant monitoring** → [Coolant_Monitor_Module](Coolant_Monitor_Module/)
- **Sim racing** → [Sim_Racing_Module](Sim_Racing_Module/)

### 2. Read the Documentation

- Module README (hardware requirements, setup)
- [CLAUDE.md](CLAUDE.md) (comprehensive development guide)
- [Documentation/INTEGRATION_GUIDE.md](Documentation/INTEGRATION_GUIDE.md) (multi-module setups)

### 3. Gather Hardware

- Arduino (see module README for specific model)
- MCP2515 CAN controller (if using CAN bus)
- Power supply (12V vehicle or bench power)
- Module-specific components (displays, sensors, etc.)

### 4. Build and Upload

#### Arduino IDE
```bash
1. Install Arduino IDE
2. Install required libraries (MCP_CAN, Adafruit, etc.)
3. Open module .ino file
4. Select board (Leonardo, Mega, Nano, etc.)
5. Upload
```

#### PlatformIO
```bash
cd [Module_Directory]
pio run              # Build
pio run -t upload    # Upload
```

### 5. Test and Integrate

- **Bench test first** (verify basic functionality)
- **Install in vehicle** (stage by stage)
- **Monitor CAN bus** (use analyzer or second Arduino)
- **Validate all systems** (dashboard, ABS, power steering, etc.)

---

## FAQ

### Q: Can I use this with a Series 2 RX8?
**A**: These modules are designed for Series 1 RX8 (2004-2008). Series 2 (2009+) uses a different CAN protocol and is not currently supported.

### Q: Do I need to know how to code?
**A**: Basic Arduino knowledge is helpful but not required. The code is ready to upload. However, **you must understand the safety implications** of modifying vehicle systems.

### Q: Can I run multiple modules together?
**A**: Yes! See [Documentation/INTEGRATION_GUIDE.md](Documentation/INTEGRATION_GUIDE.md) for integration scenarios and wiring diagrams.

### Q: What if my dashboard shows errors?
**A**: Check [CLAUDE.md](CLAUDE.md) troubleshooting section. Common issues: CAN wiring, message timing, immobilizer handshake, or vehicle-specific variations (e.g., ABS byte 7 value).

### Q: Can I tune my factory ECU instead of replacing it?
**A**: Yes! See [Tools/ECU_Definitions](Tools/ECU_Definitions/) for ECUFlash/RomRaider tuning definitions. This is an alternative approach that preserves factory reliability.

### Q: Where can I get help?
**A**: Start with module README files and documentation. For specific issues, check the original repository links in [docs/related_projects.md](docs/related_projects.md).

---

## Roadmap

### Planned Improvements

- [ ] Unified serial protocol for inter-module communication
- [ ] CAN message library (shared header file for all modules)
- [ ] Web-based configuration via ESP8266
- [ ] Centralized data logging (SD card + WiFi export)
- [ ] RaceChrono integration examples
- [ ] Series 2 RX8 support (different CAN protocol)
- [ ] Additional module integrations (if community requests)

---

## Acknowledgments

This repository would not exist without the **RX8 Arduino community**:

- Forums: RX8Club, RX7Club, and various Arduino communities
- Reverse engineers who documented the CAN protocol
- Hardware hackers who damaged Arduinos so we don't have to
- Open source developers who shared their work

**Thank you** to everyone who contributed knowledge, code, and time to this ecosystem.

---

*Last Updated: 2025-11-15*
*Repository: https://github.com/michaelprowacki/MazdaRX8Arduino*
*License: See individual module directories*

**⚠️ Use at your own risk. This code controls safety-critical vehicle systems. ⚠️**
