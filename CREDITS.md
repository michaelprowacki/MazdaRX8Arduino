# Credits and Attribution

This repository integrates work from **15+ RX8 Arduino projects** and represents hundreds of hours of reverse engineering, testing, and development by the RX8 community.

---

## Primary Contributors

### ECU_Module (Main ECU Replacement)
- **Author**: David Blackhurst
- **Contact**: dave@blackhurst.co.uk
- **Donation**: https://www.paypal.me/DBlackhurst
- **License**: As per original repository
- **Contribution**: Complete CAN bus ECU replacement for engine swaps

### EV_ECU_Module (Electric Vehicle Conversion)
- **Source**: https://github.com/EV8-Steve/RX8-dash-CAN-controller
- **Author**: EV8-Steve
- **License**: As per original repository
- **Contribution**: Electric vehicle ECU replacement with motor controller integration

### Dash_Controller_Module (Alternative Dashboard)
- **Source**: https://github.com/Antipixel/RX8-Dash
- **Author**: Antipixel
- **License**: As per original repository
- **Contribution**: Comprehensive CAN protocol documentation and PlatformIO reference implementation

### AC_Display_Module (Factory AC Display Controller)
- **Source**: https://github.com/michaelprowacki/S1-RX8-AC-Display-controller
- **Original Author**: NES-FM (https://github.com/NES-FM/S1-RX8-AC-Display-controller)
- **Fork Author**: Michael Prowacki
- **License**: MIT
- **Contribution**: Factory AC display control with custom menu system

### AC_Display_Module/ESP8266_Companion
- **Source**: https://github.com/michaelprowacki/S1-RX8-AC-Display-ESP8266-Companion
- **Author**: Michael Prowacki
- **License**: MIT
- **Contribution**: WiFi/Bluetooth companion for AC display data logging

### Aftermarket_Display_Module (OLED Gauge Display)
- **Source**: https://github.com/Radivv/arduino-display-obd2-can-mazda-rx8
- **Author**: Radivv (https://github.com/Radivv)
- **License**: As per original repository
- **Contribution**: Aftermarket RGB OLED gauge display with 15+ engine parameters

### Coolant_Monitor_Module
- **Source**: https://github.com/topolittle/rx8-coolant-monitor
- **Author**: topolittle
- **License**: BSD 3-Clause (code), Non-commercial (some 3D models)
- **Contribution**: Dedicated coolant temperature/pressure monitor with custom PCB and 3D-printed enclosure

### Wipers_Module
- **Source**: https://github.com/basilhussain/rx8-wipers
- **Author**: Basil Hussain
- **Website**: https://www.stasisleak.uk/rx8/wipers/
- **License**: BSD 3-Clause
- **Contribution**: Speed-sensitive intermittent wiper enhancement

### Sim_Racing_Module
- **Source**: https://github.com/Izekeal/rx8-arduino (fork)
- **Original Source**: https://gitlab.com/christiangroleau/rx8-arduino/
- **Original Author**: Christian Groleau
- **Fork Author**: Izekeal
- **License**: MIT
- **Contribution**: Sim racing cluster driver for Forza Horizon 5 and Dirt Rally 2.0

---

## Tools and Reverse Engineering

### Tools/PCM_Analysis (Ghidra Reverse Engineering)
- **Source**: https://github.com/equinox311/Mazda_RX8_PCM_ReverseEngineering
- **Author**: equinox311
- **Donation**: [![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=GA2ATM7VC5LZL&currency_code=USD&source=url)
- **License**: As per original repository
- **Contribution**: Ghidra-based ECU firmware reverse engineering and ROM analysis

### Tools/ECU_Definitions (ECUFlash/RomRaider Definitions)
- **Source**: https://github.com/Rx8Man/RX8Defs
- **Author**: Rx8Man
- **Support**: https://www.buymeacoffee.com/RX8Man
- **License**: As per original repository
- **Contribution**: ECUFlash/RomRaider tuning definitions for 8+ ECU calibration IDs

---

## Documentation and Reference Data

### Documentation/CAN_PID_Reference.md
- **Sources**:
  - https://github.com/topolittle/RX8-CAN-BUS (racing telemetry testing)
  - https://github.com/rnd-ash/rx8-reverse-engineering (CAN database)
  - MazdaRX8Arduino project testing
- **Contributors**: topolittle, rnd-ash
- **License**: CC BY-SA 4.0 (documentation)
- **Contribution**: Comprehensive CAN protocol reference with formulas and telemetry configuration

### Documentation/rx8_can_database.dbc
- **Source**: https://github.com/rnd-ash/rx8-reverse-engineering
- **Author**: rnd-ash
- **License**: As per original repository
- **Contribution**: DBC format CAN signal definitions for SavvyCAN, Kayak, Wireshark

---

## Referenced Projects (Not Included)

These projects are referenced in documentation but not directly integrated:

### rx8-ecu-dump (ECU Backup Tool)
- **Source**: https://github.com/ConnorRigby/rx8-ecu-dump
- **Author**: ConnorRigby
- **License**: MIT
- **Purpose**: Free J2534 ECU backup tool (strongly recommended before ECU replacement)

### GROM_RomRaider (Tuning Software)
- **Source**: https://github.com/equinox311/GROM_RomRaider
- **Author**: equinox311 (fork of RomRaider)
- **License**: GPL
- **Purpose**: RomRaider tuning software with Mazda CAN support

### Rx8Man (ECU Reflashing Tool)
- **Source**: https://github.com/Rx8Man/Rx8Man
- **Author**: Rx8Man
- **License**: As per original repository
- **Purpose**: ECU reflashing tool for Tactrix OpenPort

---

## Third-Party Libraries

This repository uses the following open-source libraries:

### Arduino Libraries
- **MCP_CAN** - Seed Studio / Cory J. Fowler (CAN bus communication)
- **Adafruit RTClib** - Adafruit Industries (Real-time clock, BSD License)
- **Adafruit BusIO** - Adafruit Industries (I2C/SPI communication, BSD License)
- **Adafruit GFX** - Adafruit Industries (Graphics library, BSD License)
- **Adafruit SSD1306** - Adafruit Industries (OLED display driver, BSD License)
- **Encoder** - Paul Stoffregen (Rotary encoder reading)
- **Smoothed** - Matthew Fryer (Data smoothing)

### Fonts
- **FreeSans18pt7bNum.h** - Adafruit GFX Library (BSD 2-Clause License)

---

## Integration and Curation

### MazdaRX8Arduino Repository
- **Repository**: https://github.com/michaelprowacki/MazdaRX8Arduino
- **Curator**: Michael Prowacki
- **License**: See individual module directories
- **Purpose**: Integrate 15+ RX8 Arduino projects into comprehensive electronics repository

**Integration Work (2025)**:
- Repository reorganization and module structure
- CAN message decoder library (lib/RX8_CAN_Messages.h)
- Integration documentation and cross-reference guides
- Performance benchmarks and comparisons
- Build system unification (Arduino IDE + PlatformIO)
- AI assistant documentation (CLAUDE.md)

---

## Special Thanks

### Community Contributors
- **RX8Club Forums** - Community knowledge and testing
- **RX7Club Forums** - Rotary engine expertise
- **Arduino Community** - Libraries and support
- **OpenInverter Community** - EV conversion guidance

### Mazda Motor Corporation
- Factory service manuals (copyrighted, included for reference)
- RX-8 logo and trademarks (used for identification purposes)

---

## Licensing Summary

This repository contains code from multiple sources with varying licenses:

| Module | License | Notes |
|--------|---------|-------|
| ECU_Module | As per original | See module directory |
| EV_ECU_Module | As per original | See module directory |
| Dash_Controller_Module | As per original | See module directory |
| AC_Display_Module | MIT | See LICENSE file |
| Aftermarket_Display_Module | As per original | See module directory |
| Coolant_Monitor_Module | BSD 3-Clause | Code only; some 3D models non-commercial |
| Wipers_Module | BSD 3-Clause | See firmware/LICENSE.txt |
| Sim_Racing_Module | MIT | See LICENSE file |
| Tools/PCM_Analysis | As per original | See module directory |
| Tools/ECU_Definitions | As per original | See module directory |

**General Guidance**: Most modules use permissive licenses (BSD, MIT, GPL). Always check individual module directories for specific license information.

**Attribution Requirement**: If you redistribute or modify this code, maintain original attribution and license notices.

---

## How to Support the Authors

If this repository has saved you time or helped your project, please consider supporting the original authors:

- **David Blackhurst** (ECU_Module): https://www.paypal.me/DBlackhurst
- **equinox311** (PCM Analysis): [PayPal donation link in Tools/PCM_Analysis]
- **Rx8Man** (ECU Definitions): https://www.buymeacoffee.com/RX8Man
- **Other Authors**: See individual module READMEs for support links

**This repository represents hundreds of hours of work**:
- Reverse engineering CAN bus protocols
- Trial and error testing in vehicles
- Hardware debugging and damaged equipment
- Documentation and knowledge sharing
- Community collaboration and support

Your support helps fund future projects like electric vehicle conversions, additional module development, and continued open-source contributions.

---

## Contributing

We welcome contributions! When contributing:

1. **Maintain Attribution**: Keep original author credits intact
2. **Document Changes**: Explain what you changed and why
3. **Test Thoroughly**: Bench test and vehicle test before committing
4. **Follow Licenses**: Respect original project licenses
5. **Update Documentation**: Keep READMEs and CLAUDE.md current

See main README.md for complete contribution guidelines.

---

## Disclaimer

This code controls **safety-critical vehicle systems**. Use at your own risk. The authors and contributors are not responsible for vehicle damage, personal injury, or death resulting from use of this code.

**Always**:
- Bench test before vehicle installation
- Backup factory ECU before replacement
- Understand the safety implications
- Test in controlled environments
- Keep factory ECU for restoration

---

*Last Updated: 2025-11-15*
*For complete project information, see README.md and individual module documentation*
