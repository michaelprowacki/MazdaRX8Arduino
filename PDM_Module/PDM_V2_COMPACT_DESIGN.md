# PDM V2 - Compact PROFET-Based Design

True electronic circuit breaker PDM. No fuses. Compact single PCB.

## Design Goals

- Fuse-less operation (electronic circuit breakers only)
- Compact (4" x 6" max PCB)
- Auto-retry after fault clears
- Professional thermal management
- Cost: $400-600 DIY

## Hardware Architecture

### High-Side Switch ICs (Not Discrete MOSFETs)

**High Current Channels (2x 21A)**:
- IC: Infineon BTS7002-1EPP (21A continuous, 2.6mΩ)
- Features: Integrated overcurrent protection, thermal shutdown, current sensing
- Auto-retry: Automatic restart after fault clears
- Package: PG-TO252-5-11 (thermal pad for heat transfer)

**Medium Current Channels (6x 7.5A)**:
- IC: Infineon BTS7008-2EPA (dual channel, 7.5A per channel, 9mΩ)
- Features: Same integrated protection as BTS7002
- Quantity: 3x dual-channel ICs = 6 channels
- Package: PG-DSO-14-41 (thermal pad)

### Microcontroller

**MCU**: STM32F103C8T6 (not Arduino Mega)
- Why: Automotive temperature range (-40°C to 85°C), faster SPI, smaller footprint
- CAN: Built-in CAN controller (no MCP2515 needed)
- Size: 48-pin LQFP (7mm x 7mm)
- Cost: $2-3 (vs $18 Arduino Mega)

Alternative: Arduino Nano Every ($10, smaller than Mega, 5V I/O compatible with PROFET)

### Power Supply

**Input**: 12V vehicle power (8-18V operating range)
**MCU Power**: TPS54331 buck converter (3.3V, 3A, $1.50)
**PROFET Logic Power**: Direct 12V (PROFET has built-in 5V regulator for logic)

### PCB Layout

**Size**: 4" x 6" (100mm x 150mm) - same as credit card + 50%
**Layers**: 4-layer
  - Layer 1: Signal + PROFET control traces
  - Layer 2: Ground plane (thermal management)
  - Layer 3: Power plane (12V distribution)
  - Layer 4: Signal + status outputs

**Copper Weight**: 2oz (70μm) on all layers for thermal + current handling

### Connectors

**Main Power**: Anderson Powerpole 45A (IN: 12V, GND)
**CAN Bus**: Deutsch DT04-2P (CANH, CANL)
**Outputs**: 8x Deutsch DT04-2P (switched 12V + current sense feedback)

Alternative: Amp Superseal 1.5 (cheaper, IP67 rated)

## Channel Configuration (RX8 Engine Swap)

| Channel | IC | Rating | Load | Priority |
|---------|-----|--------|------|----------|
| CH1 | BTS7002 | 21A | Fuel Pump | CRITICAL |
| CH2 | BTS7002 | 21A | Radiator Fan 1 | CRITICAL |
| CH3 | BTS7008 | 7.5A | Radiator Fan 2 | ESSENTIAL |
| CH4 | BTS7008 | 7.5A | Water Pump | ESSENTIAL |
| CH5 | BTS7008 | 7.5A | A/C Clutch | COMFORT |
| CH6 | BTS7008 | 7.5A | Trans Cooler | ESSENTIAL |
| CH7 | BTS7008 | 7.5A | Aux Lights | AUXILIARY |
| CH8 | BTS7008 | 7.5A | Accessory | AUXILIARY |

Total capacity: 63A (fuel pump + both fans at full load)

## Circuit Breaker Features

### Overcurrent Protection (Hardware)

PROFET chips have integrated protection:
- **kILIS current limitation**: Limits output current before shutting down
- **Thermal shutdown**: Shuts down at 150°C junction temperature
- **Auto-retry**: Chip automatically retries every 10-30ms after thermal shutdown clears

No software needed - this is hardware-based protection inside the IC.

### Retry Logic (Software + Hardware Combined)

**Level 1: Hardware Auto-Retry**
- PROFET chip retries automatically after fault clears
- Retry interval: ~10-30ms (chip-dependent)
- Continues forever if fault is intermittent (OK for inrush current)

**Level 2: Software Smart Retry**
```cpp
if (channel.faultCount > 3 in last 1 second) {
    channel.permanentDisable = true;  // Hard fault, stop retrying
    // Requires manual reset via CAN command
}
```

**Level 3: Intelligent Profiling**
```cpp
if (channel.current > limit for 50ms) {
    // Short inrush = allow (starter, motor, fan spin-up)
} else if (channel.current > limit for 500ms) {
    // Sustained overcurrent = real fault, disable
}
```

### Current Sensing (Analog Feedback)

PROFET chips have built-in current sensing:
- Analog output proportional to load current (kILIS ratio: typically 1:2000)
- MCU reads via ADC (12-bit, 0-3.3V)
- Calculate: `LoadCurrent = (SenseVoltage / kILIS_ratio)`
- Log to CAN for telemetry

No external ACS712 sensors needed - PROFET has this built-in.

## Thermal Management

### PCB Thermal Design

**Thermal Vias**:
- Under each PROFET IC thermal pad: 20x 0.3mm vias to ground plane
- Via pitch: 1mm grid
- Transfers heat from IC → ground plane → ambient

**Copper Planes**:
- Layer 2 (GND): Solid pour, acts as heat sink
- Layer 3 (12V): Solid pour, secondary heat sink
- Combined thermal mass: ~50g copper (2oz, 4" x 6" board)

**Heat Dissipation**:
- BTS7002 at 21A: ~1.1W per channel (RdsON × I² = 2.6mΩ × 21² = 1.1W)
- BTS7008 at 7.5A: ~0.5W per channel (9mΩ × 7.5² = 0.5W)
- Total worst-case: ~5W (needs no heatsink with proper PCB design)

**Enclosure**:
- Aluminum case with thermal pads
- PCB standoffs with thermal paste
- Passive cooling (no fan needed)

## Software Architecture

### Initialization

```cpp
void setup() {
    // Initialize STM32 CAN (or Arduino Nano CAN library)
    CAN.begin(500000);  // 500 kbps

    // Initialize PROFET control pins (digital output to IN pin)
    for (int i = 0; i < 8; i++) {
        pinMode(channelPin[i], OUTPUT);
        digitalWrite(channelPin[i], LOW);  // All off at startup
    }

    // Initialize ADC for current sensing
    analogReadResolution(12);  // 12-bit ADC (STM32)
}
```

### Main Loop (10ms cycle)

```cpp
void loop() {
    // Read CAN commands from ECU (0x600: enable/PWM)
    if (CAN.available()) {
        processCANCommand();
    }

    // Update channel states (with auto-retry logic)
    for (int i = 0; i < 8; i++) {
        updateChannel(&channels[i]);
    }

    // Read current sensing from PROFET ICs
    for (int i = 0; i < 8; i++) {
        channels[i].current = readCurrent(i);
    }

    // Send status to ECU (0x601: current, faults)
    sendCANStatus();

    delay(10);  // 100Hz update rate
}
```

### Auto-Retry Logic

```cpp
void updateChannel(Channel *ch) {
    // Check if channel should be enabled
    if (ch->enabled && !ch->permanentDisable) {
        digitalWrite(ch->pin, HIGH);  // PROFET handles retry automatically

        // Monitor for repeated faults (software layer)
        if (ch->faulted) {
            ch->faultCount++;
            ch->lastFaultTime = millis();
        }

        // Check for hard fault (too many retries)
        if (ch->faultCount > 3 && (millis() - ch->lastFaultTime < 1000)) {
            ch->permanentDisable = true;  // Stop trying
            digitalWrite(ch->pin, LOW);
        }
    } else {
        digitalWrite(ch->pin, LOW);
    }

    // Reset fault count after 5 seconds of stable operation
    if (millis() - ch->lastFaultTime > 5000) {
        ch->faultCount = 0;
    }
}
```

### Current Measurement

```cpp
float readCurrent(int channel) {
    int adcValue = analogRead(channel + 8);  // Sense pins offset by 8
    float senseVoltage = (adcValue / 4095.0) * 3.3;  // 12-bit ADC

    // kILIS ratio from datasheet (e.g., 1:2000 for BTS7002)
    float kILIS = (channel < 2) ? 2000.0 : 1850.0;  // Different per IC
    float loadCurrent = senseVoltage * kILIS / 3.3;  // Convert to amps

    return loadCurrent;
}
```

## Bill of Materials (BOM)

| Component | Qty | Unit Price | Total | Source |
|-----------|-----|------------|-------|--------|
| **Infineon BTS7002-1EPP** (21A) | 2 | $4.50 | $9 | Mouser/Digikey |
| **Infineon BTS7008-2EPA** (dual 7.5A) | 3 | $3.20 | $9.60 | Mouser/Digikey |
| **STM32F103C8T6** (MCU) | 1 | $2.50 | $2.50 | AliExpress |
| **TPS54331** (3.3V buck) | 1 | $1.50 | $1.50 | Mouser/Digikey |
| **PCB 4-layer 2oz** (4" x 6") | 1 | $50 | $50 | JLCPCB (5pcs) |
| **Deutsch DT04-2P connectors** | 10 | $3.50 | $35 | TE Connectivity |
| **Anderson Powerpole 45A** | 1 | $8 | $8 | Amazon |
| **Aluminum enclosure** (IP65) | 1 | $25 | $25 | Amazon |
| **Passives** (caps, resistors) | — | — | $15 | Mouser |
| **CAN transceiver** (TJA1050) | 1 | $1 | $1 | Mouser |
| **Misc** (wire, terminals, screws) | — | — | $20 | Amazon |

**Total DIY Cost**: $176.60 (per unit, using JLCPCB 5pc minimum)

**If ordering 1 PCB from OSH Park**: Add $50, total = $226.60

**Pre-assembled**: dingoPDM sells for $350-400 (reference price)

## Compact Size Comparison

| PDM | Size | Fuses | Current Capacity |
|-----|------|-------|------------------|
| **Our V1 (discrete MOSFETs)** | 8" x 6" | YES (8x) | 240A total |
| **Our V2 (PROFET)** | 4" x 6" | NO | 63A total |
| **Haltech Nexus R3** | ~6" x 4" | NO | 100A (4x25A) |
| **dingoPDM** | ~4" x 5" | NO | 75A (2x13A + 6x8A) |

## True Circuit Breaker Operation

**Fault Sequence**:
1. Overcurrent detected by PROFET IC (hardware, <1μs)
2. PROFET enters current limit mode (kILIS limiting)
3. If current persists, thermal shutdown activates (~150°C)
4. PROFET automatically retries after cooldown (~10-30ms)
5. Software monitors fault count, disables after 3 faults in 1s
6. Manual reset via CAN command: `0x602 byte 0 = RESET_FAULTS`

**No fuses needed** - PROFET IC is the circuit breaker.

## Advantages Over V1

| Feature | V1 (Discrete) | V2 (PROFET) |
|---------|---------------|-------------|
| **Fuses** | Required (8x) | None |
| **Size** | 8" x 6" | 4" x 6" |
| **Auto-retry** | No (manual reset) | Yes (hardware + smart software) |
| **Current sense** | 8x ACS712 ($32) | Built-in ($0) |
| **Protection speed** | Software (~10ms) | Hardware (<1μs) |
| **Thermal mgmt** | External heatsinks | Integrated into PCB |
| **Cost** | $230 | $177-227 |
| **Reliability** | DIY breadboard | Automotive-grade IC |

## Implementation Plan

**Phase 1**: PCB Design (KiCad)
- Schematic with PROFET ICs
- 4-layer PCB layout with thermal vias
- Gerber files for JLCPCB

**Phase 2**: Firmware (Arduino IDE or STM32CubeIDE)
- Port existing PDM_Module.ino to work with PROFET control
- Implement current sensing via analog inputs
- Add smart retry logic
- CAN protocol unchanged (0x600-0x603)

**Phase 3**: Assembly
- Order PCBs ($50 for 5pcs)
- Order components from Mouser/Digikey
- Hand solder or order SMT assembly from JLCPCB (+$50)

**Phase 4**: Testing
- Bench test with resistive loads
- Verify auto-retry with intentional short circuit
- Current sensing calibration
- Thermal testing (load all channels to 80% for 1 hour)

**Phase 5**: Vehicle Integration
- Same as V1 (CAN protocol compatible)
- Mount in engine bay
- No fuse box needed

## Next Steps

1. Create KiCad schematic (based on dingoPDM reference + BTS7002/7008 datasheets)
2. PCB layout with proper thermal management
3. Port firmware to STM32 or Arduino Nano Every
4. Order prototype PCBs ($50)
5. Assemble and test

**Timeline**: 2-3 weeks from design to working prototype

**Cost per unit**: $177 (if ordering 5 PCBs), $227 (single PCB from OSH Park)

## References

- Infineon BTS7002-1EPP datasheet: https://www.infineon.com/cms/en/product/power/smart-power-switches/high-side-switches/profet-plus-2-12v-automotive-smart-high-side-switch/bts7002-1epp/
- Infineon BTS7008-2EPA datasheet: https://www.infineon.com/cms/en/product/power/smart-power-switches/high-side-switches/profet-plus-2-12v-automotive-smart-high-side-switch/bts7008-2epa/
- dingoPDM reference: https://github.com/corygrant/dingoPDM
- Automotive PCB thermal design: https://www.protoexpress.com/blog/10-automotive-pcb-design-guidelines/

---

**This is a true PDM. No fuses. Electronic circuit breakers only. Compact. Professional.**
