# Hardware Wiring Guide - Advanced RX8 ECU
**Complete Hardware Integration for 17 Advanced Features**

## Overview

This guide covers the complete hardware implementation of the RX8 Arduino ECU with all 17 advanced features. This is a **professional-grade installation** requiring automotive connectors, proper wire gauge, signal conditioning, and safety interlocks.

**⚠️ WARNING**: This is a complex automotive electrical system. Improper wiring can damage components or create safety hazards. Professional installation strongly recommended.

---

## System Architecture

### Three-Tier Hardware Design

```
┌─────────────────────────────────────────────────────────┐
│                    TIER 1: CORE ECU                     │
│  STM32F407 or Automotive MCU (recommended upgrade)      │
│  - Engine control (fuel, ignition, OMP)                │
│  - CAN bus communication                                │
│  - Safety-critical functions                            │
└─────────────────────────────────────────────────────────┘
                            │
                    CAN Bus / Serial
                            │
┌─────────────────────────────────────────────────────────┐
│              TIER 2: ADVANCED FEATURES MCU              │
│  Arduino Mega 2560 or ESP32                             │
│  - Boost control, launch control, traction control     │
│  - Water/meth injection, nitrous control               │
│  - Map switching, torque management                     │
│  - GP PWM outputs                                       │
└─────────────────────────────────────────────────────────┘
                            │
                    CAN Bus / I2C
                            │
┌─────────────────────────────────────────────────────────┐
│           TIER 3: DISPLAY & TELEMETRY MCU               │
│  ESP32 (WiFi/Bluetooth integrated)                      │
│  - WiFi dashboard, data logging                         │
│  - AC display control                                   │
│  - OBD2 interface                                       │
└─────────────────────────────────────────────────────────┘
```

**Why Three Tiers?**
- Safety isolation (engine control separate from accessories)
- Easier troubleshooting
- Upgrade path (swap Tier 1 to automotive MCU without redoing everything)
- Reduced wiring complexity

---

## Pin Assignments - Tier 2 (Advanced Features MCU)

**Recommended: Arduino Mega 2560** (54 digital + 16 analog pins)

### Core Sensors & CAN Bus

| Pin | Function | Connection | Wire Gauge | Connector |
|-----|----------|------------|------------|-----------|
| **0 (RX)** | Serial to Tier 1 | Tier 1 TX | 22 AWG | 2-pin Molex |
| **1 (TX)** | Serial to Tier 1 | Tier 1 RX | 22 AWG | 2-pin Molex |
| **2** | CAN INT (MCP2515) | CAN interrupt | 24 AWG | - |
| **10** | CAN CS (MCP2515) | SPI chip select | 24 AWG | - |
| **11-13** | SPI (MOSI/MISO/SCK) | CAN module | 24 AWG | - |
| **18-19** | I2C (SDA/SCL) | Expansion sensors | 22 AWG | - |

### Boost Control (Pins 22-25)

| Pin | Function | Connection | Wire Gauge | Connector Type |
|-----|----------|------------|------------|----------------|
| **22** | Wastegate Solenoid | Solenoid +ve (12V PWM) | 18 AWG | Bosch EV1 / Denso |
| **23** | Boost Pressure Sensor | Analog 0-5V | 22 AWG | 3-pin Deutsch |
| **24** | Overboost Warning | LED/Buzzer output | 20 AWG | 2-pin |
| **25** | Boost Safety Cut | Fuel cut relay | 18 AWG | Relay socket |

**Wastegate Solenoid**: 3-port solenoid (MAC valve)
- Port 1: Atmosphere
- Port 2: Wastegate actuator
- Port 3: Boost source
- **Recommended**: Turbosmart MAC 3-port ($80)

### Launch Control (Pins 26-28)

| Pin | Function | Connection | Wire Gauge | Connector Type |
|-----|----------|------------|------------|----------------|
| **26** | Launch Enable Switch | Ground to activate | 20 AWG | Toggle switch |
| **27** | Clutch Switch Input | OEM clutch switch | 20 AWG | Tap OEM harness |
| **28** | Launch Active LED | Warning light | 20 AWG | LED + resistor |

### Traction Control (Pins 29-32)

| Pin | Function | Connection | Wire Gauge | Connector Type |
|-----|----------|------------|------------|----------------|
| **29** | TC Mode Switch | Rotary switch | 20 AWG | 4-pos rotary |
| **30** | TC Active LED | Warning light | 20 AWG | LED |
| **31** | Wheel Speed FL | CAN (from ABS) | - | - |
| **32** | Wheel Speed FR | CAN (from ABS) | - | - |

**Note**: Wheel speeds read from CAN bus (0x4B1), no direct wiring needed.

### Flex Fuel Sensor (Pins 33-34)

| Pin | Function | Connection | Wire Gauge | Connector Type |
|-----|----------|------------|------------|----------------|
| **33** | Flex Fuel Signal | GM sensor signal | 22 AWG | Weatherpack 3-pin |
| **34** | Flex Fuel Power | 12V switched | 20 AWG | Weatherpack 3-pin |
| **GND** | Flex Fuel Ground | Chassis ground | 20 AWG | Weatherpack 3-pin |

**GM Flex Fuel Sensor**:
- Part: GM 13577429 or Continental 13577379
- Output: 50-150 Hz frequency
- Voltage: 12V
- Current: <50mA
- **Cost**: ~$50

### Idle Air Control (Pin 35)

| Pin | Function | Connection | Wire Gauge | Connector Type |
|-----|----------|------------|------------|----------------|
| **35** | IAC Valve PWM | IAC solenoid | 18 AWG | Bosch EV1 |

**IAC Valve Options**:
- OEM RX8 IAC valve (if retained)
- Bosch 0280140545 (universal)
- PWM frequency: 100 Hz

### Knock Detection (Pins 36-37)

| Pin | Function | Connection | Wire Gauge | Connector Type |
|-----|----------|------------|------------|----------------|
| **36** | Knock Sensor Analog | Knock sensor output | Shielded 22 AWG | OEM connector |
| **37** | Knock Event LED | Warning light | 20 AWG | LED |

**Knock Sensor**:
- Use OEM RX8 knock sensor
- Location: Engine block (follow FSM)
- **Critical**: Use shielded cable, ground shield at ECU end only

### Water/Methanol Injection (Pins 38-43)

| Pin | Function | Connection | Wire Gauge | Connector Type |
|-----|----------|------------|------------|----------------|
| **38** | WMI Pump PWM | Pump relay/controller | 18 AWG | Relay socket |
| **39** | WMI Solenoid 1 | Stage 1 nozzle | 18 AWG | Weatherpack |
| **40** | WMI Solenoid 2 | Stage 2 nozzle | 18 AWG | Weatherpack |
| **41** | Tank Level Sensor | Analog 0-5V | 22 AWG | 3-pin Deutsch |
| **42** | Flow Sensor | Pulse input | 22 AWG | 3-pin Deutsch |
| **43** | WMI Fault LED | Warning light | 20 AWG | LED |

**Water/Meth System**:
- **Pump**: AEM V2 or Snow Performance Stage 2 ($200-300)
- **Nozzles**: AEM progressive nozzles ($30-80 each)
- **Tank**: 1-2 gallon (4-8L) with level sensor
- **Flow Sensor**: Hall effect turbine sensor

### Nitrous Control (Pins 44-51)

| Pin | Function | Connection | Wire Gauge | Connector Type |
|-----|----------|------------|------------|----------------|
| **44** | Nitrous Arm Switch | Toggle switch | 20 AWG | Toggle |
| **45** | Purge Solenoid | Purge valve | 18 AWG | Weatherpack |
| **46** | Stage 1 Nitrous Sol. | N2O solenoid | 16 AWG | High-current |
| **47** | Stage 1 Fuel Sol. | Fuel solenoid | 16 AWG | High-current |
| **48** | Stage 2 Nitrous Sol. | N2O solenoid | 16 AWG | High-current |
| **49** | Stage 2 Fuel Sol. | Fuel solenoid | 16 AWG | High-current |
| **50** | Bottle Pressure | Analog 0-5V | 22 AWG | 3-pin Deutsch |
| **51** | Nitrous Active LED | Warning light | 20 AWG | LED |

**Nitrous System** (if installed):
- **Kit**: NOS, Nitrous Express, or ZEX 6-cylinder kit
- **Solenoids**: High-flow (0.093" orifice minimum)
- **Bottle**: 10 lb minimum with heater
- **⚠️ CRITICAL**: Professional installation required

### General Purpose PWM Outputs (Pins A8-A15)

| Pin | Function | Typical Use | Wire Gauge | Connector Type |
|-----|----------|-------------|------------|----------------|
| **A8** | GP PWM 1 | Radiator fan | 18 AWG | Relay socket |
| **A9** | GP PWM 2 | Fuel pump stage 2 | 18 AWG | Relay socket |
| **A10** | GP PWM 3 | Shift light | 20 AWG | LED circuit |
| **A11** | GP PWM 4 | Oil cooler fan | 18 AWG | Relay socket |
| **A12** | GP PWM 5 | Boost gauge | 22 AWG | Gauge driver |
| **A13** | GP PWM 6 | Warning light | 20 AWG | LED |
| **A14** | GP PWM 7 | Custom output | 20 AWG | User defined |
| **A15** | GP PWM 8 | Custom output | 20 AWG | User defined |

### Data Logging (Pins 52-53 + SPI)

| Pin | Function | Connection | Wire Gauge | Connector Type |
|-----|----------|------------|------------|----------------|
| **52** | SD Card CS | SD module | 24 AWG | SPI header |
| **53** | Data Log LED | Activity LED | 20 AWG | LED |
| **50-52** | SPI (shared) | SD card module | 24 AWG | SPI header |

**SD Card Module**:
- Use SPI-based SD card breakout
- 3.3V logic level (use level shifter or 3.3V tolerant Mega)
- Class 10 SD card minimum (16-32 GB recommended)

---

## Automotive Connectors & Harness

### Connector Recommendations

**Why Automotive Connectors?**
- Vibration resistant
- Weatherproof (IP67 rated)
- Reliable in harsh environments
- Professional appearance

### Primary Connector Types

#### 1. **Deutsch DT Series** (Recommended for sensors)
- **Use**: All analog sensors (boost, temp, pressure)
- **Ratings**: IP67, -40°C to 150°C
- **Sizes**: 2, 3, 4, 6, 8, 12 pin
- **Cost**: $3-8 per connector
- **Where**: Amazon, Waytek, Mouser

**Example Part Numbers**:
- 2-pin: DT06-2S (receptacle) + DT04-2P (plug)
- 3-pin: DT06-3S + DT04-3P
- 4-pin: DT06-4S + DT04-4P

#### 2. **Weatherpack Series** (GM-style)
- **Use**: Solenoids, switches, medium current
- **Ratings**: IP67, up to 15A per pin
- **Sizes**: 1, 2, 3, 4, 6 pin
- **Cost**: $2-5 per connector
- **Where**: Amazon, eBay (very common)

**Example Part Numbers**:
- 2-pin: 12010973 (housing) + 12059168 (terminals)
- 3-pin: 12015792 (housing) + 12059168 (terminals)

#### 3. **Bosch EV1 / Denso** (For solenoids)
- **Use**: Wastegate, IAC, water/meth solenoids
- **Ratings**: High temperature, up to 10A
- **Sizes**: 2-pin (standard)
- **Cost**: $3-6 per connector
- **Where**: Fuel Injector Clinic, Amazon

#### 4. **AMP Superseal** (High current)
- **Use**: Power distribution, relays, pumps
- **Ratings**: Up to 50A per pin, IP67
- **Sizes**: 1, 2, 3, 4, 6 pin
- **Cost**: $4-10 per connector
- **Where**: Mouser, Digikey, Waytek

#### 5. **D-Sub / ECU Connectors** (Main ECU harness)
- **Use**: Main ECU connection (30-100+ pins)
- **Options**:
  - Ampseal 35-pin (Molex)
  - Deutsch HD-30 (30-pin)
  - Custom PCB edge connector
- **Cost**: $30-100 for connector + pins
- **Recommendation**: Use breakout board with screw terminals for prototyping, then proper connector for final install

### Connector Kit Recommendation

**Budget Starter Kit** (~$100):
- Deutsch DT 2/3/4-pin kit (Amazon "Deutsch Connector Kit")
- Weatherpack 2/3/4-pin kit
- Heat shrink tube assortment
- Crimping tool (IWISS SN-025 or Engineer PA-09)

**Professional Kit** (~$300):
- Full Deutsch DT assortment
- Weatherpack assortment
- AMP Superseal kit
- Professional crimper (Molex 63819-0000 or Deutsch HDT-48-00)
- Heat shrink with adhesive
- Wire labels

---

## Wiring Harness Design

### Wire Gauge Guide

**Power Distribution**:
- Main 12V supply: 10 AWG (or larger)
- Relay power: 14-16 AWG
- Solenoid power: 16-18 AWG
- Sensor power: 20-22 AWG

**Signal Wires**:
- Analog sensors: 22 AWG (shielded for noise-sensitive)
- Digital signals: 22-24 AWG
- PWM outputs: 18-20 AWG
- CAN bus: Twisted pair 22 AWG (120Ω termination)

**Ground**:
- Sensor grounds: 20-22 AWG (star ground at ECU)
- Power grounds: Match power wire gauge
- Chassis ground: 10 AWG minimum

### Harness Construction

#### Main ECU Harness (60+ wires)

**Construction Steps**:

1. **Plan the harness** - Create wiring diagram in CAD
2. **Cut wires to length** - Add 20% extra for routing
3. **Label everything** - Use wire labels or heat shrink markers
4. **Bundle by function**:
   - Power bundle (red/black)
   - Sensor bundle (various colors)
   - Solenoid bundle (various colors)
   - CAN bus (twisted pair, separate)
5. **Use proper sleeving**:
   - Split loom: Budget option ($20 for 100ft)
   - Braided sleeve: Professional look ($50 for 100ft)
   - Heat shrink tubing: Ends and branches

#### Sub-Harnesses

Create separate sub-harnesses for:
- **Engine Bay**: Sensors, solenoids, boost control
- **Interior**: Switches, displays, data logging
- **Nitrous System**: If installed, separate harness
- **Water/Meth**: If installed, separate harness

### Routing & Protection

**Engine Bay Routing**:
- Route away from exhaust (>6 inches clearance)
- Use heat-resistant sleeving near hot components
- Secure every 12 inches with zip ties or P-clips
- Avoid sharp edges (use grommets)
- Keep away from moving parts (alternator, belts)

**Firewall Penetration**:
- Use large grommet (2-3 inch diameter)
- Apply dielectric grease
- Support weight of harness on both sides
- Label wires before and after firewall

**Interior Routing**:
- Follow OEM harness paths where possible
- Secure to avoid rattles
- Protect from foot traffic
- Keep away from HVAC vents (heat)

---

## Power Distribution

### Power Architecture

```
                    Battery (12V)
                         │
                    Main Fuse (30A)
                         │
              ┌──────────┴──────────┐
              │                     │
         Relay 1 (ECU)         Relay 2 (Accessories)
              │                     │
    ┌─────────┴─────────┐          │
    │                   │          │
ECU Power         Sensors      Solenoids/Pumps
 (Mega)          (5V/12V)      (High Current)
    │
 Internal
  5V Reg
    │
 5V Sensors
```

### Required Relays

| Relay | Purpose | Rating | Trigger Source |
|-------|---------|--------|----------------|
| **1** | ECU Main Power | 20A | Ignition switch |
| **2** | Accessories | 30A | ECU output |
| **3** | Fuel Pump Stage 2 | 30A | GP PWM output |
| **4** | Radiator Fan | 40A | GP PWM output |
| **5** | Water/Meth Pump | 20A | WMI output |
| **6** | Nitrous Solenoids | 20A | Nitrous arm switch |

**Relay Type**: Bosch-style automotive relays (30A/40A, 12V coil)
**Cost**: $5-10 each
**Mounting**: Relay panel or use vehicle's existing relay box

### Fuse Panel

**Recommended**: Add-a-fuse panel or dedicated fuse block

| Circuit | Fuse Rating | Wire Gauge |
|---------|-------------|------------|
| ECU main power | 10A | 14 AWG |
| Sensors | 5A | 20 AWG |
| Solenoids | 15A | 16 AWG |
| Pumps (WMI, fuel) | 20A | 14 AWG |
| Fans | 30A | 12 AWG |
| Nitrous (per stage) | 15A | 16 AWG |
| GP PWM outputs | 10A | 18 AWG |

---

## Signal Conditioning

### Analog Sensors

**Problem**: Many sensors output 0-5V but need noise filtering and overvoltage protection.

**Solution**: Add RC filter to each analog input

```
Sensor Output ──┬──── 1kΩ resistor ────┬──── Arduino Analog Pin
                │                      │
               5.1V                  0.1µF
              Zener                   Cap
              Diode                   │
                │                     │
                └──────── GND ────────┘
```

**Parts Needed (per sensor)**:
- 1kΩ resistor (1/4W)
- 0.1µF ceramic capacitor
- 5.1V Zener diode (1N4733A)

**Cost**: ~$0.50 per sensor channel

### Digital Inputs (Switches)

**Problem**: Mechanical switches bounce and can false-trigger.

**Solution**: Use internal pullup resistors + software debouncing (already in code)

```
Switch ──── Arduino Digital Pin (pinMode INPUT_PULLUP)
   │
  GND
```

No external components needed!

### High-Current Outputs (Solenoids, Pumps)

**Problem**: Arduino pins can only source 40mA. Solenoids draw 1-5A.

**Solution**: Use MOSFETs or relay drivers

**MOSFET Driver Circuit** (for each solenoid):

```
Arduino PWM Pin ──── 1kΩ ──── Gate (MOSFET)
                              │
                            IRLB8721
                         (N-channel FET)
                              │
                          Drain │ Source
                              │     │
                             Load  GND
                              │
                          +12V (via fuse)
```

**Add Flyback Diode** across inductive loads:
```
        Solenoid
           ║
    ┌──────╫──────┐
    │      ║      │
  +12V   1N4007   GND
           │
       (Cathode to +12V)
```

**Parts**:
- MOSFET: IRLB8721 or IRFZ44N ($1-2 each)
- Diode: 1N4007 (50¢ each)
- Resistor: 1kΩ (10¢)

**Pre-built Option**: 8-channel relay module ($15-25 on Amazon)

---

## Sensor Integration

### Critical Sensors (Required)

| Sensor | Part Number | Location | Connection |
|--------|-------------|----------|------------|
| **Boost Pressure** | AEM 30-2130-50 | Intake manifold | Analog A0 |
| **Flex Fuel** | GM 13577429 | Fuel line | Interrupt pin 33 |
| **Knock Sensor** | OEM RX8 | Engine block | Analog A1 (shielded) |
| **Coolant Temp** | OEM RX8 | Coolant passage | CAN bus (Tier 1) |
| **Oil Pressure** | OEM RX8 | Oil gallery | CAN bus (Tier 1) |
| **TPS** | OEM RX8 | Throttle body | CAN bus (Tier 1) |
| **MAF/MAP** | OEM RX8 | Intake | CAN bus (Tier 1) |

### Optional Sensors (Feature-Dependent)

| Feature | Sensor Needed | Part Example | Cost |
|---------|---------------|--------------|------|
| **Water/Meth** | Tank level | Resistive float sensor | $20 |
| **Water/Meth** | Flow sensor | Hall effect turbine | $30 |
| **Nitrous** | Bottle pressure | 0-1500 PSI (0-5V) | $40 |
| **Torque Mgmt** | Wheel speed | CAN bus (existing ABS) | $0 |
| **Traction Control** | Wheel speed | CAN bus (existing ABS) | $0 |
| **Flex Fuel** | Ethanol content | GM flex sensor | $50 |

---

## ECU Enclosure

### Requirements

- **Waterproof**: IP65 minimum (IP67 recommended)
- **Heat resistant**: Up to 125°C (engine bay can get hot)
- **EMI shielding**: Aluminum or steel (not plastic)
- **Vibration mounting**: Rubber grommets

### Recommended Enclosures

**Budget Option** (~$30):
- Hammond 1590DD aluminum die-cast box (7.4" × 4.7" × 2.2")
- Add rubber grommets for cable glands
- Drill holes for connectors

**Professional Option** (~$100-200):
- Bud Industries NBF-32026 (NEMA 4X, IP66)
- Pre-punched for connectors
- Integrated cable glands
- Larger size for better cooling

**Ultimate Option** (~$300-500):
- Custom CNC aluminum enclosure
- Integrated heatsinks
- Multiple mounting options
- Professional appearance

### Mounting Location

**Engine Bay** (if using automotive MCU):
- Firewall near ECU
- Fender wall (protected)
- Under hood (heat concerns)
- ⚠️ Requires heat-resistant enclosure

**Interior** (if using Arduino):
- Under passenger seat
- Behind dashboard (access required)
- Trunk (longer harness needed)
- ✅ Recommended for Arduino-based systems

---

## Cooling & Thermal Management

### Arduino Mega Thermal Considerations

**Problem**: Arduino Mega can overheat if enclosed without ventilation.

**Solutions**:
1. **Heatsinks**: Add to voltage regulator and microcontroller
2. **Ventilation**: Drill vent holes with mesh screens
3. **Fan**: Small 12V fan (40mm × 40mm) if needed
4. **Conformal coating**: Protect PCB from moisture/dust

**Parts**:
- TO-220 heatsinks: $5 for 10-pack
- 40mm 12V fan: $5-10
- Stainless mesh (vent screens): $5

---

## Professional Installation Tips

### 1. Test Bench Setup FIRST

Before vehicle installation:
- Build complete ECU on workbench
- Test ALL features with simulated inputs
- Use potentiometers to simulate sensors
- Verify ALL outputs with multimeter
- Run for 24 hours to check for glitches

### 2. Phased Installation

**Phase 1: Basic Integration**
- Install ECU enclosure
- Run main power & ground
- Install CAN bus connection
- Test basic communication with Tier 1 ECU

**Phase 2: Core Features**
- Boost control
- Launch control
- Traction control
- Test on dyno or controlled environment

**Phase 3: Advanced Features**
- Water/meth injection (if installed)
- Nitrous (if installed)
- GP PWM outputs
- Torque management

### 3. Documentation

**Create Installation Binder**:
- Wiring diagrams (as-built)
- Connector pin-outs
- Sensor calibrations
- Troubleshooting guide
- Parts list with sources

### 4. Safety Checks

**Before First Start**:
- [ ] All grounds verified continuous
- [ ] No shorts to power (multimeter check)
- [ ] Fuses installed and correct rating
- [ ] CAN bus termination resistors (120Ω each end)
- [ ] All connectors locked and secured
- [ ] Vibration test (shake harness gently)
- [ ] Visual inspection (no exposed wires)

---

## Bill of Materials (BOM)

### Core Components

| Item | Quantity | Cost Each | Total | Source |
|------|----------|-----------|-------|--------|
| **Arduino Mega 2560** | 1 | $40 | $40 | Amazon |
| **MCP2515 CAN Module** | 1 | $8 | $8 | Amazon |
| **SD Card Module** | 1 | $5 | $5 | Amazon |
| **Relay Module (8-ch)** | 1 | $20 | $20 | Amazon |
| **Enclosure (Hammond)** | 1 | $30 | $30 | Amazon/Mouser |

**Core Subtotal**: ~$100

### Connectors & Wire

| Item | Quantity | Cost | Source |
|------|----------|------|--------|
| **Deutsch DT Kit** | 1 set | $50 | Amazon |
| **Weatherpack Kit** | 1 set | $30 | Amazon |
| **Wire (assorted 16-22 AWG)** | 100 ft each | $40 | Amazon |
| **Braided sleeving** | 100 ft | $30 | Amazon |
| **Heat shrink** | Assortment | $15 | Amazon |
| **Crimp tool** | 1 | $40 | Amazon |

**Connectors Subtotal**: ~$205

### Sensors (Feature-Dependent)

| Sensor | Cost | Required For |
|--------|------|--------------|
| **GM Flex Fuel Sensor** | $50 | Flex fuel |
| **Boost Pressure Sensor** | $60 | Boost control |
| **Wide | O2 Sensor (Bosch LSU 4.9)** | $80 | Tuning |
| **Tank Level Sensor** | $20 | Water/meth |
| **Flow Sensor** | $30 | Water/meth |
| **Bottle Pressure** | $40 | Nitrous |

**Sensors Subtotal**: $100-280 (depending on features)

### Solenoids & Actuators

| Item | Cost | Required For |
|------|------|--------------|
| **Wastegate Solenoid (MAC 3-port)** | $80 | Boost control |
| **IAC Valve (Bosch)** | $40 | Idle control |
| **WMI Pump (AEM)** | $250 | Water/meth |
| **WMI Solenoids (2x)** | $60 | Water/meth |
| **Nitrous Solenoids (wet kit)** | $200 | Nitrous |

**Actuators Subtotal**: $80-630 (depending on features)

### **GRAND TOTAL**

| Configuration | Cost |
|---------------|------|
| **Minimum (Boost only)** | $385 |
| **Mid-level (Boost + TC + Water/Meth)** | $815 |
| **Maximum (All features)** | $1,215 |

Still 50-75% cheaper than Haltech Elite 2500 ($2,500)!

---

## Next Steps

1. **Review wiring diagram** - Understand all connections
2. **Order components** - See BOM above
3. **Build bench test setup** - Test before vehicle install
4. **Create wiring harness** - Take your time, label everything
5. **Bench test for 24 hours** - Validate all features
6. **Vehicle installation** - Phased approach
7. **Calibration & tuning** - On dyno or with professional tuner

---

**Last Updated**: 2025-11-16
**Revision**: 1.0
**Status**: Complete reference guide for hardware implementation

**For questions or clarifications, refer to:**
- PHASE5_COMPLETE.md (feature overview)
- SYSTEM_INTEGRATION.md (safety interlocks)
- HALTECH_FEATURES.md (feature comparison)
