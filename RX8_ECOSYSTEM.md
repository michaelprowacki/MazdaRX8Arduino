# RX8 Arduino Ecosystem - Repository Integration Analysis

## Overview

This document analyzes 14 related RX8 projects and provides integration recommendations for the MazdaRX8Arduino project. Each repository has been evaluated for compatibility, usefulness, and integration strategy.

**Analysis Date**: 2025-11-15
**Base Project**: MazdaRX8Arduino (ECU/PCM Replacement + AC Display Module)

---

## Repository Categories

### 1. Direct Integration Candidates (High Priority)
Repositories that should be integrated or closely referenced.

### 2. Reference Documentation
Valuable information to include as references or data files.

### 3. Separate Module Candidates
Projects that could be added as optional modules.

### 4. External Tool References
Tools that complement but don't integrate directly.

---

## Detailed Analysis

### CATEGORY 1: SISTER PROJECTS (Nearly Identical)

#### 1.0 RX8-dash-CAN-controller - EV Conversion (REFERENCE - VERY HIGH RELEVANCE)

**Repository**: https://github.com/EV8-Steve/RX8-dash-CAN-controller

**Purpose**: RX8 dashboard integration for electric vehicle conversions

**Languages**: C++ (100%)

**Key Features**:
- Arduino Nano based CAN controller (vs our Leonardo)
- Integrates with Open Inverter (popular open-source EV motor controller)
- Uses exact same CAN messages (0x201, 0x420, etc.)
- Maps electric motor data → ICE-equivalent dashboard signals
- 8kHz PWM for motor control
- Odometer pulse generation
- Wheel speed monitoring
- ABS system integration
- Power Assisted Steering support

**Integration Strategy**: 📚 **REFERENCE FOR EV FEATURES**

```
Recommendation: Reference, potentially merge EV-specific features

Rationale:
- Solves the SAME problem but for EV instead of ICE
- Nearly identical code structure to ours
- Same CAN messages, different data sources
- Could add "EV mode" option to our project

What We Can Learn:
- Odometer pulse generation code (we don't have this)
- Open Inverter CAN integration
- Motor → engine signal mapping
- 8kHz PWM techniques

Integration Options:
A. Reference only - link in docs (RECOMMENDED)
B. Extract odometer pulse code
C. Add optional EV mode to RX8_CANBUS.ino
D. Create separate EV_ECU_Module/ (if demand exists)

Relevance: ⭐⭐⭐⭐⭐ EXTREMELY HIGH - Sister project
```

**Status**: REFERENCE - Document for EV conversion users

---

### CATEGORY 2: DIRECT INTEGRATION CANDIDATES

#### 2.1 ESP8266 Companion (INTEGRATE - HIGH PRIORITY)

**Repository**:
- https://github.com/michaelprowacki/S1-RX8-AC-Display-ESP8266-Companion
- https://github.com/NES-FM/S1-RX8-AC-Display-ESP8266-Companion (original)

**Purpose**: Expands AC Display functionality with WiFi/Bluetooth capabilities

**Languages**: C++ (52%), Python (34.3%), C (13.7%)

**Key Features**:
- Persistent data storage
- Bluetooth communication bridge
- ESP-Now protocol support
- Data logging capabilities
- OBD-II data integration

**Integration Strategy**: ✅ **INTEGRATE AS SUBMODULE**

```
Recommendation: Add as AC_Display_Module/ESP8266_Companion/

Rationale:
- Already designed for AC Display Module
- Same author (michaelprowacki)
- Complementary functionality
- Uses PlatformIO (matches AC Display Module)

Integration Steps:
1. Add as git submodule or copy into AC_Display_Module/
2. Update AC_Display_Module/README.md with ESP8266 section
3. Document Serial3 communication protocol
4. Add wiring diagrams for ESP8266 connection
5. Update platformio.ini with ESP8266 environment

Files to integrate:
- src/: ESP8266 firmware code
- lib/: ESP8266 libraries
- platformio.ini: Build configuration
- platformio_upload.py: Upload script
```

**Status**: RECOMMENDED - Direct companion to existing AC Display Module

---

#### 2.2 RX8 Reverse Engineering Wiki Data (REFERENCE - HIGH PRIORITY)

**Repository**: https://github.com/rnd-ash/rx8-reverse-engineering

**Purpose**: Comprehensive CAN bus reverse engineering documentation

**Languages**: Documentation + rx8.dbc file

**Key Features**:
- CAN bus DBC file (database) with signal definitions
- Extensive wiki documentation
- Community reverse engineering findings

**Integration Strategy**: ✅ **REFERENCE + EXTRACT DBC FILE**

```
Recommendation: Add to Documentation/ folder

Rationale:
- Authoritative CAN bus definitions
- Complements existing CAN implementation
- DBC file can be used by CAN analysis tools

Integration Steps:
1. Download rx8.dbc file to Documentation/rx8_can_database.dbc
2. Add link to wiki in CLAUDE.md references section
3. Cross-reference CAN message IDs with current implementation
4. Validate our CAN messages against DBC definitions

Files to integrate:
- rx8.dbc → Documentation/rx8_can_database.dbc
- Wiki link in README and CLAUDE.md
```

**Status**: RECOMMENDED - Essential reference material

---

#### 2.3 RX8 CAN Bus PID Documentation (REFERENCE - HIGH PRIORITY)

**Repository**: https://github.com/topolittle/RX8-CAN-BUS

**Purpose**: Documented CAN bus PIDs for telemetry/racing applications

**Languages**: Markdown documentation

**Key Features**:
- 9 documented CAN PIDs with formulas
- Tested with OBDLink MX+ scanner
- RaceChrono configuration examples
- Includes RPM, throttle, brake, steering angle, temperatures

**Integration Strategy**: ✅ **REFERENCE DOCUMENTATION**

```
Recommendation: Add as Documentation/CAN_PID_Reference.md

Rationale:
- Validates existing CAN message structures
- Provides additional PIDs we may not use yet
- Racing/telemetry perspective useful for future features

Integration Steps:
1. Copy README content to Documentation/CAN_PID_Reference.md
2. Cross-reference with our 0x201, 0x420 implementations
3. Add PIDs we're missing (steering angle, brake pressure)
4. Update CLAUDE.md with PID reference section

Notable PIDs to consider adding:
- 0x4BE: Brake pressure
- 0x4B0: Steering wheel angle
- 0x075: Handbrake status
```

**Status**: RECOMMENDED - Complements existing CAN implementation

---

#### 2.4 RX8-Dash CAN Protocol Implementation (REFERENCE - MEDIUM PRIORITY)

**Repository**: https://github.com/Antipixel/RX8-Dash

**Purpose**: Complete CAN protocol for RX8 instrument cluster

**Languages**: C++ (81.5%), C (18.5%)

**Key Features**:
- Detailed message format documentation (0x420, 0x201, 0x650, 0x231, 0x300, 0x212)
- Bit-level flag definitions
- Warning light control mappings
- PlatformIO project structure

**Integration Strategy**: ✅ **CODE REFERENCE + CROSS-VALIDATION**

```
Recommendation: Use as reference, extract useful code snippets

Rationale:
- Overlaps significantly with our RX8_CANBUS.ino
- More detailed bit-field documentation
- Good for validating our implementation
- C++ class structure could inspire refactoring

Integration Steps:
1. Compare their message definitions with ours
2. Extract any missing warning light controls
3. Add their bit-field documentation as comments
4. Link repository in CLAUDE.md references
5. Consider code structure for future refactoring

Cross-validation:
- Our 0x201 vs their 0x201 (RPM/Speed) ✓
- Our 0x420 vs their 0x420 (MIL/Temp) ✓
- Our 0x212 vs their 0x212 (ABS/DSC) ✓
- Check 0x650, 0x231, 0x300 (we don't use these yet)
```

**Status**: RECOMMENDED - Excellent reference for validation

---

### CATEGORY 3: SEPARATE MODULE CANDIDATES

#### 3.1 Arduino Display OBD2/CAN (OPTIONAL MODULE - MEDIUM PRIORITY)

**Repository**: https://github.com/Radivv/arduino-display-obd2-can-mazda-rx8

**Purpose**: Aftermarket OBD2 display for RX8

**Languages**: C++ (100%)

**Key Features**:
- 15 monitored parameters
- 1.5" RGB OLED displays
- MCP2515 CAN interface
- Real-time engine diagnostics

**Integration Strategy**: 🔶 **OPTIONAL REFERENCE / ALTERNATIVE DISPLAY**

```
Recommendation: Add as optional reference in docs/alternatives/

Rationale:
- Different use case (aftermarket display vs. ECU replacement)
- Overlapping functionality with AC Display Module
- Good reference for additional sensor monitoring
- Different hardware (OLED vs. factory AC display)

Integration Steps:
1. Create docs/alternatives/ folder
2. Add link and description
3. Extract sensor reading algorithms if useful
4. Could inspire additional menu pages for AC Display Module

Potential uses:
- Alternative display implementation ideas
- Sensor smoothing algorithms (Smoothed library - already used!)
- Parameter monitoring list for features
```

**Status**: OPTIONAL - Reference only, different use case

---

#### 3.2 RX8 Coolant Monitor (OPTIONAL MODULE - MEDIUM PRIORITY)

**Repository**: https://github.com/topolittle/rx8-coolant-monitor

**Purpose**: Dedicated coolant temperature and pressure monitoring system

**Languages**: C (49.9%), C++ (45.1%)

**Key Features**:
- 3x 128x32 OLED displays
- Arduino Pro Micro based
- Steinhart-Hart thermistor equation
- Custom PCB with 3D-printed enclosure
- Pressure sensor integration
- Warning thresholds

**Integration Strategy**: 🔶 **SEPARATE OPTIONAL MODULE**

```
Recommendation: Add as optional Coolant_Monitor_Module/ (if desired)

Rationale:
- Addresses specific RX8 weakness (poor coolant monitoring)
- Well-documented hardware design
- Professional PCB and 3D models
- Separate hardware system (doesn't integrate with main ECU)
- Different target (dashboard addon vs. ECU replacement)

Integration Options:
A. Reference only - link in documentation
B. Add as separate module for users who want it
C. Extract sensor algorithms for AC Display Module

Best approach: REFERENCE ONLY
- Most users doing ECU replacement want simple setup
- Those interested can follow original project
- Add link in CLAUDE.md under "Related Projects"

What to extract:
- Steinhart-Hart thermistor code (if we add temp sensors)
- Warning threshold logic
- Display layout ideas for AC module
```

**Status**: OPTIONAL - Reference for users wanting this feature

---

#### 3.3 RX8 Wipers Speed-Sensitive System (OPTIONAL MODULE - LOW PRIORITY)

**Repository**: https://github.com/basilhussain/rx8-wipers

**Purpose**: Add speed-sensitive intermittent wipers

**Languages**: C (79.7%), JavaScript (12.8%), HTML (7.5%)

**Key Features**:
- Speed-sensitive wiper control
- Firmware + hardware design
- Reads vehicle speed from CAN bus
- Modifies wiper timing based on speed

**Integration Strategy**: 🔷 **REFERENCE ONLY**

```
Recommendation: Link in documentation as related project

Rationale:
- Completely separate system (wiper control)
- Not related to ECU replacement use case
- Well-documented standalone project
- Users interested can follow original

Integration: NONE - Just reference
- Add link in CLAUDE.md under "Community Projects"
- Mention as example of CAN bus usage
- No code integration needed

Useful for:
- Example of reading vehicle speed from CAN (already do this)
- Automotive-grade design practices
- PCB design reference
```

**Status**: REFERENCE ONLY - Out of scope for ECU replacement

---

### CATEGORY 4: EXTERNAL TOOL REFERENCES

#### 4.1 RX8 ECU Dump Tool (EXTERNAL TOOL - HIGH PRIORITY REFERENCE)

**Repository**: https://github.com/ConnorRigby/rx8-ecu-dump

**Purpose**: Dump and flash RX8 ECU firmware via J2534

**Languages**: C (88.6%), C++ (10.2%)

**Key Features**:
- ROM extraction (~2 min)
- RAM snapshots (~5 sec)
- J2534 interface
- Free alternative to expensive commercial tools
- Work in progress (flashing not complete)

**Integration Strategy**: 📚 **DOCUMENTATION REFERENCE ONLY**

```
Recommendation: Reference in CLAUDE.md for users wanting to backup original ECU

Rationale:
- External tool, not code to integrate
- Useful for users BEFORE installing Arduino ECU replacement
- Good practice: backup original before modification
- Different use case (reads factory ECU, we replace it)

Documentation update:
Add to CLAUDE.md:

## Pre-Installation Recommendations

### Backup Your Original ECU

Before installing the Arduino ECU replacement, strongly consider backing up
your original ECU firmware using tools like:

- rx8-ecu-dump (https://github.com/ConnorRigby/rx8-ecu-dump) - Free J2534 tool
- Commercial options: Tactrix, EcuTek

This allows you to:
1. Restore factory operation if needed
2. Reference original calibrations
3. Contribute to community reverse engineering efforts

Note: This Arduino replacement does NOT read/modify original ECU.
It completely replaces the ECU functionality.
```

**Status**: REFERENCE - Pre-installation tool for users

---

#### 4.2 RX8Defs - ECU Definitions (EXTERNAL REFERENCE - MEDIUM PRIORITY)

**Repository**: https://github.com/Rx8Man/RX8Defs

**Purpose**: Open-source ECU definitions for tuning software

**Languages**: XML definitions

**Key Features**:
- ECUFlash definitions
- RomRaider definitions
- 8 supported ECU calibrations (2004-2005 RX8s)
- European, US, JDM variants

**Integration Strategy**: 📚 **REFERENCE FOR TUNERS**

```
Recommendation: Link in documentation for users who want to tune

Rationale:
- For users keeping original ECU and tuning it
- Our project REPLACES the ECU, so definitions don't apply
- Useful for users with hybrid setups
- Good reference for understanding ECU tables

Documentation note:
Add to CLAUDE.md or README:

## Relationship to ECU Tuning

This project **replaces** the factory ECU entirely. If you want to:
- TUNE your factory ECU → See RX8Defs for tuning definitions
- REPLACE your factory ECU → This project (MazdaRX8Arduino)

These are different approaches:
- Tuning: Modify factory calibrations (requires J2534 interface)
- Replacement: Arduino controls engine directly (this project)
```

**Status**: REFERENCE - Different use case (tuning vs. replacement)

---

#### 4.3 Rx8Man Reflash Tool (EXTERNAL TOOL - MEDIUM PRIORITY)

**Repository**: https://github.com/Rx8Man/Rx8Man

**Purpose**: Minimal ECU reflashing tool for Series 1 RX8

**Languages**: Not specified (binary release)

**Key Features**:
- Tactrix OpenPort interface
- ECU firmware flashing
- Simple GUI tool
- Latest version: 1.05 (Oct 2023)

**Integration Strategy**: 📚 **REFERENCE AS ALTERNATIVE APPROACH**

```
Recommendation: Mention as alternative to Arduino replacement

Rationale:
- Tool for reflashing factory ECU
- Alternative to our replacement approach
- Users should know both options exist

Add to README.md:

## Alternative Approaches

This project replaces the ECU with an Arduino. Other approaches include:

1. **ECU Tuning/Reflashing** (keep factory ECU, modify software)
   - Tools: Rx8Man, EcuTek, Tactrix
   - Pros: Factory reliability, proven tuning
   - Cons: Limited by factory hardware, expensive tools

2. **Arduino ECU Replacement** (this project)
   - Pros: Full control, cheap hardware, engine swaps
   - Cons: Requires extensive testing, DIY risk

Choose based on your needs.
```

**Status**: REFERENCE - Alternative approach

---

#### 4.4 Mazda RX8 PCM Reverse Engineering (EXTERNAL REFERENCE - LOW PRIORITY)

**Repository**: https://github.com/equinox311/Mazda_RX8_PCM_ReverseEngineering

**Purpose**: Reverse engineering ECU firmware using Ghidra

**Languages**: Python (100%)

**Key Features**:
- Ghidra project archives
- Stock ROM files
- Live data captures
- Memory table identification

**Integration Strategy**: 📚 **REFERENCE FOR ADVANCED USERS**

```
Recommendation: Link in docs for those interested in ECU internals

Rationale:
- Advanced reverse engineering resource
- Not directly applicable to Arduino replacement
- Useful for understanding what factory ECU does
- Could inspire feature parity

Add to CLAUDE.md references:

## Understanding Factory ECU Behavior

For those interested in how the factory ECU works internally:
- Mazda_RX8_PCM_ReverseEngineering - Ghidra analysis
- GROM_RomRaider - Modified tuning software

These help understand what algorithms/logic the factory uses.
```

**Status**: REFERENCE - Advanced users only

---

#### 4.5 GROM RomRaider (EXTERNAL TOOL - LOW PRIORITY)

**Repository**: https://github.com/equinox311/GROM_RomRaider

**Purpose**: RomRaider modified for Mazda RX8 CAN support

**Languages**: Java (99.7%)

**Key Features**:
- Subaru tuning suite adapted for RX8
- Data logging capabilities
- ECU viewing and calibration
- User-friendly interface

**Integration Strategy**: 📚 **REFERENCE AS COMPLEMENTARY TOOL**

```
Recommendation: Mention for users with hybrid setups

Rationale:
- Some users may keep factory ECU for certain functions
- Could be used alongside Arduino for data logging
- Educational tool to see factory ECU behavior

No integration needed - reference only
```

**Status**: REFERENCE - Tuning tool, different use case

---

#### 4.6 RX8-Arduino Sim Racing (REFERENCE - LOW PRIORITY)

**Repository**: https://github.com/Izekeal/rx8-arduino

**Purpose**: Drive RX8 instrument cluster from sim racing games

**Languages**: Python (40.2%), C++ (35.6%), C (24.2%)

**Key Features**:
- Forza Horizon 5 integration
- Dirt Rally 2.0 support
- Telemetry data to CAN bus conversion
- Handbrake button support

**Integration Strategy**: 📚 **REFERENCE AS INTERESTING PROJECT**

```
Recommendation: Link as related project (entertainment value)

Rationale:
- Fun project but completely different use case
- Shows CAN protocol usage from different angle
- Not applicable to vehicle ECU replacement
- Could inspire test bench ideas

Potential use:
- Testing CAN implementation on bench
- Simulating vehicle behavior without running engine
- Educational/demonstration purposes

Add to CLAUDE.md:

## Testing and Simulation

For bench testing without a running vehicle:
- rx8-arduino - Drives cluster from sim racing games
- Could be adapted to simulate ECU behavior for testing
```

**Status**: REFERENCE - Fun project, limited applicability

---

## Integration Priority Summary

### ✅ HIGH PRIORITY - INTEGRATE NOW

1. **ESP8266 Companion** → Add as `AC_Display_Module/ESP8266_Companion/`
   - Direct companion to AC Display Module
   - Same author, designed to work together
   - Adds WiFi/Bluetooth/logging capabilities

2. **rx8-reverse-engineering DBC file** → Add to `Documentation/rx8_can_database.dbc`
   - Authoritative CAN bus definitions
   - Validates our implementation
   - Tool compatibility

3. **RX8-CAN-BUS PID docs** → Add to `Documentation/CAN_PID_Reference.md`
   - Additional PIDs not currently used
   - Formula validation
   - Racing/telemetry perspective

4. **RX8-Dash protocol** → Cross-reference with our code
   - Validate our CAN messages
   - Extract missing bit definitions
   - Add as comments in RX8_CANBUS.ino

---

### 🔶 MEDIUM PRIORITY - REFERENCE / OPTIONAL

5. **Arduino OBD2 Display** → docs/alternatives/aftermarket_display.md
   - Different use case but good reference
   - Sensor algorithm ideas

6. **Coolant Monitor** → Link in related projects
   - Well-designed but separate system
   - Users wanting this can build it separately

7. **ECU Dump Tool** → Pre-installation recommendation
   - Backup original ECU before replacement

8. **RX8Defs** → Reference for tuners
   - Alternative approach (tuning vs. replacement)

9. **Rx8Man** → Reference as alternative approach

---

### 🔷 LOW PRIORITY - REFERENCE ONLY

10. **Wipers** → Related projects link
    - Out of scope for ECU replacement

11. **PCM Reverse Engineering** → Advanced reference
    - Educational resource

12. **GROM RomRaider** → Tuning tool reference

13. **RX8-Arduino Sim** → Testing/simulation ideas

---

## Recommended Integration Steps

### Phase 1: Documentation Integration (Do First)

1. Add CAN references to Documentation/
   ```bash
   wget https://raw.githubusercontent.com/rnd-ash/rx8-reverse-engineering/master/rx8.dbc \
     -O Documentation/rx8_can_database.dbc
   ```

2. Create Documentation/CAN_PID_Reference.md
   - Copy content from topolittle/RX8-CAN-BUS
   - Format with our existing message IDs

3. Update CLAUDE.md with:
   - Related Projects section
   - External Tools section
   - Pre-Installation Recommendations

### Phase 2: ESP8266 Companion Integration (High Value)

1. Add ESP8266 companion as submodule:
   ```bash
   git submodule add https://github.com/michaelprowacki/S1-RX8-AC-Display-ESP8266-Companion.git \
     AC_Display_Module/ESP8266_Companion
   ```

2. Update AC_Display_Module/README.md:
   - Add ESP8266 Companion section
   - Document Serial3 communication
   - Wiring diagrams

3. Update AC_Display_Module/docs/integration.md:
   - Add Option 4: ESP8266 WiFi/Bluetooth Bridge
   - Installation instructions
   - Configuration guide

### Phase 3: Code Cross-Validation (Quality Improvement)

1. Compare RX8-Dash implementation with ours:
   - Line-by-line CAN message validation
   - Add missing bit-field comments
   - Extract any missing warning light controls

2. Add TODO comments for missing PIDs:
   ```cpp
   // TODO: Consider adding from CAN_PID_Reference.md:
   // - 0x4BE: Brake pressure
   // - 0x4B0: Steering wheel angle
   // - 0x075: Handbrake status
   ```

3. Cross-reference with rx8.dbc:
   - Validate signal names
   - Check multipliers/offsets
   - Ensure compatibility with CAN tools

### Phase 4: Documentation Expansion (User Value)

1. Create docs/related_projects.md:
   - Categorize all 14 repositories
   - Brief description of each
   - When to use which

2. Create docs/alternatives.md:
   - ECU Tuning vs. Replacement
   - When to choose each approach
   - Tools for each method

3. Update README.md:
   - Add "Related Projects" section
   - Link to ecosystem document
   - Clarify project scope

---

## File Structure After Integration

```
MazdaRX8Arduino/
├── RX8_CANBUS.ino
├── README.md
├── CLAUDE.md
├── RX8_ECOSYSTEM.md              # This file
│
├── AC_Display_Module/
│   ├── README.md                 # Updated with ESP8266 section
│   ├── src/
│   ├── include/
│   ├── lib/
│   ├── docs/
│   │   └── integration.md        # Updated with ESP8266 option
│   └── ESP8266_Companion/        # NEW: Git submodule
│       ├── src/
│       ├── lib/
│       └── platformio.ini
│
├── Documentation/
│   ├── rx8_can_database.dbc      # NEW: From rnd-ash/rx8-reverse-engineering
│   ├── CAN_PID_Reference.md      # NEW: From topolittle/RX8-CAN-BUS
│   ├── 08_Steering.pdf
│   ├── 13Electrical.pdf
│   └── ...
│
└── docs/                          # NEW: Additional documentation
    ├── related_projects.md        # All 14 repos categorized
    ├── alternatives.md            # Tuning vs. Replacement
    └── pre_installation.md        # Backup recommendations
```

---

## Conclusion

**Immediate Actions (High Value)**:
1. ✅ Add ESP8266 Companion as submodule
2. ✅ Download rx8.dbc to Documentation/
3. ✅ Copy CAN PID docs to Documentation/
4. ✅ Update CLAUDE.md with references

**Optional Actions (Medium Value)**:
- Cross-validate code with RX8-Dash
- Add related projects documentation
- Create pre-installation guide

**Reference Only (Low Priority)**:
- Link remaining projects in documentation
- No code integration needed

**Integration Philosophy**:
- Only integrate what adds direct value
- Maintain focus on ECU replacement use case
- Reference alternatives clearly
- Keep project scope manageable
- Credit original authors appropriately

---

*Analysis Date: 2025-11-15*
*Analyst: Claude (AI Assistant)*
*Base Project: MazdaRX8Arduino by David Blackhurst*
