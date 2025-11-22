# PDM V2 Schematic

Infineon PROFET-based compact PDM. No KiCad file yet - this is the schematic description.

## Power Section

**12V Input (J1 - Anderson Powerpole 45A)**:
```
VIN (12V) ----+---- C1 (100uF 25V electrolytic) ----+---- GND
              |                                       |
              +---- C2 (0.1uF ceramic)               |
              |                                       |
              +---- U1 (TPS54331 buck converter)     |
                    |                                 |
                    +---- VOUT (3.3V) ---- C3 (47uF) +
```

**Buck Converter (TPS54331)**:
- VIN: 12V vehicle
- VOUT: 3.3V @ 3A (MCU + logic power)
- FB: Voltage divider (47k + 10k for 3.3V output)
- EN: Pull-up to VIN via 100k (always on)
- BOOT: 0.1uF to SW
- SW: L1 (10uH inductor) + D1 (schottky diode)

## Microcontroller (U2 - Arduino Nano Every or STM32F103C8T6)

**Arduino Nano Every Pinout**:
```
D2  ---- PROFET_CH1_IN  (Fuel Pump)
D3  ---- PROFET_CH2_IN  (Radiator Fan 1)
D4  ---- PROFET_CH3_IN  (Radiator Fan 2)
D5  ---- PROFET_CH4_IN  (Water Pump)
D6  ---- PROFET_CH5_IN  (A/C Clutch)
D7  ---- PROFET_CH6_IN  (Trans Cooler)
D8  ---- PROFET_CH7_IN  (Aux Lights)
D9  ---- PROFET_CH8_IN  (Accessory)

A0  ---- PROFET_CH1_IS  (Current sense)
A1  ---- PROFET_CH2_IS
A2  ---- PROFET_CH3_IS
A3  ---- PROFET_CH4_IS
A4  ---- PROFET_CH5_IS
A5  ---- PROFET_CH6_IS
A6  ---- PROFET_CH7_IS
A7  ---- PROFET_CH8_IS

D10 ---- CAN_CS (MCP2515 chip select)
D11 ---- SPI_MOSI
D12 ---- SPI_MISO
D13 ---- SPI_SCK

TX  ---- CAN_TX (or Serial debug)
RX  ---- CAN_RX (or Serial debug)
```

## CAN Transceiver (U3 - MCP2515 + TJA1050)

**MCP2515 CAN Controller**:
```
VCC  ---- 3.3V
GND  ---- GND
SCK  ---- D13
SI   ---- D11
SO   ---- D12
CS   ---- D10
INT  ---- D2 (optional interrupt)
```

**TJA1050 CAN Transceiver**:
```
VCC  ---- 3.3V
GND  ---- GND
TXD  ---- MCP2515_TX
RXD  ---- MCP2515_RX
CANH ---- J2 pin 1 (Deutsch DT04-2P)
CANL ---- J2 pin 2 (Deutsch DT04-2P)
```

**CAN Termination**: 120Ω resistor between CANH and CANL (optional, depends on bus topology)

## High Current Channels (U4, U5 - BTS7002-1EPP)

**BTS7002-1EPP (21A, single channel) x2**:

**U4 - Channel 1 (Fuel Pump)**:
```
Pin 1 (IN)   ---- MCU D2 + 10k pull-down to GND
Pin 2 (GND)  ---- GND
Pin 3 (OUT)  ---- J3 pin 1 (Output connector)
Pin 4 (VS)   ---- 12V (VIN)
Pin 5 (IS)   ---- MCU A0 + 10k pull-down to GND

Thermal Pad  ---- GND (20x thermal vias to Layer 2)
```

**U5 - Channel 2 (Radiator Fan 1)**:
```
Pin 1 (IN)   ---- MCU D3 + 10k pull-down to GND
Pin 2 (GND)  ---- GND
Pin 3 (OUT)  ---- J4 pin 1 (Output connector)
Pin 4 (VS)   ---- 12V (VIN)
Pin 5 (IS)   ---- MCU A1 + 10k pull-down to GND

Thermal Pad  ---- GND (20x thermal vias)
```

## Medium Current Channels (U6, U7, U8 - BTS7008-2EPA)

**BTS7008-2EPA (dual 7.5A) x3**:

**U6 - Channels 3 & 4 (Fan 2, Water Pump)**:
```
Pin 1  (IN0)    ---- MCU D4 + 10k pull-down
Pin 2  (IN1)    ---- MCU D5 + 10k pull-down
Pin 3  (GND)    ---- GND
Pin 4  (GND)    ---- GND
Pin 5  (IS0)    ---- MCU A2 + 10k pull-down
Pin 6  (IS1)    ---- MCU A3 + 10k pull-down
Pin 7  (VS)     ---- 12V (VIN)
Pin 8  (VS)     ---- 12V (VIN)
Pin 9  (OUT0)   ---- J5 pin 1 (Fan 2)
Pin 10 (OUT1)   ---- J6 pin 1 (Water Pump)
Pin 11 (VS)     ---- 12V (VIN)
Pin 12 (VS)     ---- 12V (VIN)
Pin 13 (GND)    ---- GND
Pin 14 (GND)    ---- GND

Thermal Pad     ---- GND (20x thermal vias)
```

**U7 - Channels 5 & 6 (A/C, Trans Cooler)** - Same pinout, MCU D6/D7, A4/A5

**U8 - Channels 7 & 8 (Aux Lights, Accessory)** - Same pinout, MCU D8/D9, A6/A7

## Output Connectors (J3-J10)

**Deutsch DT04-2P (or Amp Superseal 1.5mm) x8**:
```
Pin 1: Switched 12V OUT (from PROFET)
Pin 2: GND
```

Each connector goes to a load (fuel pump, fans, lights, etc.)

## Component Notes

**PROFET Input (IN) pins**:
- Logic high (>2.0V): Channel ON
- Logic low (<0.8V): Channel OFF
- Pull-down resistor (10k) ensures OFF state when MCU is not driving

**PROFET Current Sense (IS) pins**:
- Analog output, proportional to load current
- kILIS ratio: 2000:1 for BTS7002, 1850:1 for BTS7008
- Example: 21A load = 21A / 2000 = 10.5mA sense current
- Sense resistor (10k): 10.5mA × 10k = 0.105V to MCU ADC

**Thermal Vias**:
- Under each PROFET thermal pad: 20x 0.3mm diameter vias
- Via pitch: 1mm grid (4x5 array)
- Connects top layer thermal pad → Layer 2 (GND plane)
- Critical for heat dissipation

## PCB Layer Stack

```
Layer 1 (Top):    Signal traces + PROFET control + component pads
Layer 2:          GND plane (solid pour, thermal heatsink)
Layer 3:          12V power plane (solid pour from VIN)
Layer 4 (Bottom): Signal traces + connectors
```

**Copper Weight**: 2oz (70μm) on all layers

**Board Size**: 100mm × 150mm (4" × 6")

## Power Budget

**Quiescent Current** (all channels OFF):
- MCU (Nano Every): 20mA @ 5V = 0.1W
- MCP2515: 10mA @ 3.3V = 0.033W
- TJA1050: 5mA @ 3.3V = 0.016W
- PROFET standby: 8× 1mA = 8mA @ 12V = 0.096W
- **Total idle**: ~43mA @ 12V = 0.52W

**Full Load** (all channels at max):
- CH1 (21A): 21A × 2.6mΩ = 1.1W dissipation
- CH2 (21A): 1.1W
- CH3-8 (6× 7.5A): 6× (7.5² × 9mΩ) = 3W
- **Total dissipation**: ~5.2W

**Load power**: 63A × 12V = 756W (to loads, not dissipated)

## Thermal Analysis

**Worst-case thermal**:
- 5.2W dissipation across 8 ICs
- PCB thermal resistance: ~20°C/W (with 2oz copper + thermal vias)
- Junction temperature rise: 5.2W × 20°C/W = 104°C rise
- Ambient (50°C engine bay) + 104°C = 154°C
- **Thermal shutdown at 150°C** - marginal, need aluminum case or forced air

**Mitigation**:
- Aluminum enclosure with thermal pads (reduces to ~10°C/W)
- New junction temp: 50°C + 5.2W × 10°C/W = 102°C (safe)

## Protection Features (Built into PROFET)

**Overcurrent Protection**:
- kILIS current limitation activates before shutdown
- Thermal shutdown at 150°C junction temperature
- Auto-retry after thermal recovery (~10-30ms)

**Overvoltage Protection**:
- PROFET rated to 41V (vehicle transients up to 36V OK)

**Undervoltage Lockout (UVLO)**:
- PROFET turns OFF below ~6V (prevents battery drain)

**Short Circuit Protection**:
- Hardware current limit engages immediately
- Thermal shutdown within milliseconds
- Auto-retry attempts until fault clears

**Reverse Polarity Protection**:
- NOT built-in (add external P-channel MOSFET or diode if needed)
- Or: Use keyed Anderson Powerpole connector (can't reverse)

## Manufacturing Notes

**PCB Fabrication** (JLCPCB):
- 4-layer board, 2oz copper
- Min trace width: 0.2mm (8 mil)
- Min via size: 0.3mm drill, 0.6mm pad
- Thermal vias: 0.3mm drill, no mask (exposed)
- Surface finish: ENIG (better thermal + soldering)
- Color: Black (looks professional)

**Assembly**:
- Hand solder: PROFET ICs are PG-TO252 and PG-DSO-14 (doable with hot air)
- SMT assembly: JLCPCB SMT service (+$50 setup, $0.50/IC)
- Through-hole: Connectors, bulk capacitors (easy hand solder)

**Testing Pads**:
- Add test points for: 3.3V, 12V, GND, each PROFET IS pin
- Labeled on silkscreen for debugging

## Bill of Materials

See separate PDM_V2_BOM.md file for complete BOM with part numbers.

---

**Next**: Create firmware (PDM_V2_Firmware.ino) and BOM (PDM_V2_BOM.md)
