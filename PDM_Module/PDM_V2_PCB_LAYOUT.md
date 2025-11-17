# PDM V2 PCB Layout Guidelines

4-layer PCB design with proper thermal management for PROFET ICs.

## Board Specifications

**Dimensions**: 100mm × 150mm (4" × 6")
**Layers**: 4
**Copper Weight**: 2oz (70μm) on all layers
**Board Thickness**: 1.6mm standard

**Layer Stack**:
```
Layer 1 (Top):    Signal + component pads + PROFET thermal pads
Layer 2 (Inner):  Ground plane (solid pour, thermal heatsink)
Layer 3 (Inner):  Power plane (12V distribution, solid pour)
Layer 4 (Bottom): Signal + connectors
```

## Component Placement

**Left Side (Power Input)**:
- J1: Anderson Powerpole (12V input, top edge)
- U1: TPS54331 buck converter (near input)
- C1, C2: Input caps (close to TPS54331)

**Center (Microcontroller)**:
- U2: Arduino Nano Every (center of board)
- U3: MCP2515 CAN module (right of MCU)
- J2: CAN bus connector (right edge)

**Right Side (PROFET ICs)**:
- U4: BTS7002 CH1 (top right)
- U5: BTS7002 CH2 (below U4)
- U6: BTS7008 CH3-4 (below U5)
- U7: BTS7008 CH5-6 (below U6)
- U8: BTS7008 CH7-8 (bottom right)

**Output Connectors (Bottom Edge)**:
- J3-J10: Deutsch DT04-2P (8x outputs, spaced 15mm apart)

## Thermal Management (Critical)

### PROFET Thermal Pad Layout

Each PROFET IC has an exposed thermal pad on the bottom:

**BTS7002-1EPP** (PG-TO252-5-11):
- Thermal pad: 3.0mm × 5.8mm
- Thermal vias: 20× 0.3mm diameter
- Via grid: 4 rows × 5 columns, 1mm pitch
- Via configuration: TENTED (no soldermask, exposed to Layer 2)

**BTS7008-2EPA** (PG-DSO-14-41):
- Thermal pad: 5.4mm × 6.4mm
- Thermal vias: 30× 0.3mm diameter
- Via grid: 5 rows × 6 columns, 1mm pitch
- Via configuration: TENTED (exposed to Layer 2)

### Thermal Via Details

**Via Specification**:
- Drill size: 0.3mm (12 mil)
- Pad diameter: 0.6mm (24 mil)
- Plating: Standard (copper-plated through-hole)
- Mask: TENTED (soldermask removed, via exposed)

**Why tented vias**: Allow thermal paste between IC and PCB, better heat transfer to GND plane.

### Copper Pour (Layer 2 - GND)

**Ground Plane**:
- Solid pour over entire board (no voids under PROFET ICs)
- Connects all thermal vias from PROFET pads
- Acts as heatsink (70μm × 100mm × 150mm = ~150g copper mass)

**Thermal Impedance Calculation**:
- PCB thermal conductivity: ~1 W/(m·K) (FR4 + copper)
- 2oz copper thermal resistance: ~10°C/W (with proper via stitching)
- Aluminum enclosure: ~5°C/W (with thermal pads)
- **Total thermal resistance**: ~5-10°C/W (excellent for 5W dissipation)

## Power Routing

### 12V Power Distribution (Layer 3)

**Input Trace** (J1 to power plane):
- Width: 5mm (0.2")
- Current capacity: 50A (2oz copper)
- Pour connects to Layer 3 power plane via multiple vias

**Power Plane (Layer 3)**:
- Solid pour over right half of board (PROFET area)
- Feeds all PROFET VS pins
- Clearance from GND plane: 0.3mm

**PROFET Power Connection**:
- Via from Layer 3 to Layer 1 (near each VS pin)
- Via size: 0.6mm drill, 1.2mm pad (high current)
- Multiple vias per IC (2-3 vias minimum)

### GND Return (Layer 2)

**Ground Plane**:
- Solid pour over entire board
- Connects all GND pins (MCU, PROFET, connectors)
- Also serves as thermal heatsink

**GND Trace** (J1 to plane):
- Width: 5mm
- Multiple vias to Layer 2 ground plane

## Signal Routing

### PROFET Control Signals (IN pins)

**MCU to PROFET**:
- Route on Layer 1 (top)
- Trace width: 0.25mm (10 mil)
- Pull-down resistor (10k) close to PROFET IN pin
- Keep traces short (<50mm preferred)

**Signal Integrity**:
- Digital signals (ON/OFF), not critical
- No impedance matching required
- Route away from 12V power traces

### PROFET Current Sense (IS pins)

**PROFET to MCU ADC**:
- Route on Layer 1 (top) or Layer 4 (bottom)
- Trace width: 0.25mm
- Pull-down resistor (10k) close to PROFET IS pin
- Keep traces short (<100mm)
- Route away from noisy signals (PWM, switching)

**Analog Considerations**:
- Ground guard traces around analog signals (optional)
- Keep away from SPI clock (D13) and CAN signals

### SPI Bus (MCU to MCP2515)

**Signals**: SCK, MOSI, MISO, CS
- Route on Layer 1
- Trace width: 0.25mm
- Keep traces parallel and equal length (matched)
- Max length: 100mm (short = better signal integrity)

**SPI Clock (SCK)**:
- Most critical signal (8 MHz)
- Route first, then route data lines
- Ground pour around traces (coplanar waveguide)

### CAN Bus (MCP2515 to J2)

**Signals**: CANH, CANL
- Route as differential pair
- Trace width: 0.5mm
- Spacing: 0.3mm (differential impedance ~120Ω)
- Keep traces equal length (±5mm)
- No vias if possible (straight route to connector)

## Decoupling & Filtering

### Power Supply Decoupling

**12V Input (VIN)**:
- 100uF electrolytic (bulk capacitance, near J1)
- 0.1uF ceramic (high-frequency filtering, close to TPS54331 VIN)

**3.3V Output (TPS54331)**:
- 47uF ceramic (output cap, close to VOUT pin)
- 10uF ceramic (bulk, near MCU VCC)
- 0.1uF ceramic (decoupling, at each IC VCC pin)

**PROFET VS Pins** (12V):
- 0.1uF ceramic at each PROFET VS pin (close to IC)
- Suppresses switching noise

### IC Decoupling

**Placement Rule**: Decoupling cap <5mm from IC power pin

**MCU (Arduino Nano Every)**:
- Module has built-in decoupling, but add 0.1uF near VCC pin

**MCP2515**:
- 0.1uF ceramic at VCC pin

**Each PROFET IC**:
- 0.1uF ceramic between VS and GND pins

## PCB Design Checklist

### Schematic Review
- [ ] All PROFET IN pins have 10k pull-down
- [ ] All PROFET IS pins have 10k pull-down
- [ ] All ICs have decoupling caps (0.1uF minimum)
- [ ] CAN bus has 120Ω termination (if end node)
- [ ] TPS54331 feedback divider correct (47k + 10k for 3.3V)

### Layout Review
- [ ] PROFET thermal pads have 20-30× thermal vias
- [ ] Thermal vias are tented (no soldermask)
- [ ] Ground plane solid (no voids under PROFET)
- [ ] 12V power plane feeds all PROFET VS pins
- [ ] Decoupling caps <5mm from IC power pins
- [ ] CAN signals routed as differential pair
- [ ] SPI traces equal length (±10mm)
- [ ] No acute angles in traces (use 45° or curved)

### Clearances
- [ ] Trace-to-trace: 0.3mm minimum (0.5mm preferred)
- [ ] Trace-to-pour: 0.3mm minimum
- [ ] Via-to-trace: 0.3mm minimum
- [ ] High voltage (12V) to low voltage (3.3V): 1mm clearance

### Silkscreen
- [ ] All connectors labeled (J1: 12V IN, J2: CAN, J3-J10: CH1-CH8)
- [ ] Polarity marks on connectors (+/-)
- [ ] IC designators (U1-U8)
- [ ] Test points labeled (TP1: 3.3V, TP2: 12V, etc.)
- [ ] Board name + version (e.g., "PDM V2.0")

### Soldermask
- [ ] Thermal vias: NO soldermask (tented)
- [ ] Test points: NO soldermask (exposed copper)
- [ ] All other areas: Soldermask (standard)

### Board Finish
- [ ] ENIG (gold) preferred for thermal pads + longevity
- [ ] Alternative: HASL (cheaper, acceptable)

## Manufacturing Notes

### JLCPCB Order

**PCB Specs**:
- Base Material: FR-4
- Layers: 4
- Dimensions: 100mm × 150mm
- PCB Qty: 5
- PCB Thickness: 1.6mm
- PCB Color: Black
- Silkscreen: White
- Surface Finish: ENIG
- Outer Copper Weight: 2oz
- Inner Copper Weight: 2oz
- Via Covering: Tented
- Min Track/Spacing: 6/6mil (0.15mm)
- Min Hole Size: 0.3mm

**Cost**: ~$50 for 5 PCBs (1-2 week delivery)

### Gerber Files Checklist

Required files:
- [ ] Top layer (GTL)
- [ ] Inner layer 1 (G2L - GND)
- [ ] Inner layer 2 (G3L - Power)
- [ ] Bottom layer (GBL)
- [ ] Top soldermask (GTS)
- [ ] Bottom soldermask (GBS)
- [ ] Top silkscreen (GTO)
- [ ] Bottom silkscreen (GBO)
- [ ] Drill file (TXT or XLN)
- [ ] Board outline (GKO or GML)

### KiCad Export Settings

**Gerber Export**:
- Format: Gerber RS-274X
- Include extended attributes: YES
- Use drill/place file origin: Absolute
- Subtract soldermask from silkscreen: YES

**Drill File**:
- Format: Excellon
- Units: mm
- Zeros format: Decimal (0.3)
- Merge PTH and NPTH: NO (separate files)

## Testing After Assembly

### Visual Inspection
- [ ] All ICs oriented correctly (pin 1 marked)
- [ ] No solder bridges
- [ ] Thermal vias not clogged with solder
- [ ] All connectors secure

### Continuity Test (Multimeter)
- [ ] GND plane continuous (test between any two GND points)
- [ ] 12V power plane not shorted to GND
- [ ] Each PROFET OUT pin isolated from GND (when OFF)

### Power-Up Test (No Load)
- [ ] Apply 12V to J1 (Anderson Powerpole)
- [ ] Measure 3.3V at MCU VCC (should be 3.25-3.35V)
- [ ] Measure current draw (<100mA idle)
- [ ] Check for hot components (TPS54331 should be warm, not hot)

### Functional Test (With Test Loads)
- [ ] Enable each channel via Serial command
- [ ] Measure voltage at each output (should be ~12V when ON)
- [ ] Measure current with 5W light bulb load (~0.4A @ 12V)
- [ ] Verify current sensing reports accurate value (±10%)

### Thermal Test
- [ ] Load all channels to 80% rated current
- [ ] Run for 30 minutes
- [ ] Measure IC temperature with IR thermometer
- [ ] Should be <100°C (thermal shutdown at 150°C)

## PCB Design Tools

**Recommended**: KiCad (free, open-source)
- Download: kicad.org
- Tutorials: youtube.com/c/DigiKey (KiCad tutorial series)

**Alternative**: EasyEDA (free, web-based, integrates with JLCPCB)
- URL: easyeda.com

**Professional**: Altium Designer ($$$, industry standard)

## Next Steps

1. Create KiCad project from schematic (PDM_V2_SCHEMATIC.md)
2. Import footprints for PROFET ICs (download from Infineon website)
3. Layout PCB following guidelines above
4. Run DRC (Design Rule Check)
5. Generate Gerber files
6. Order PCBs from JLCPCB
7. Order components from Mouser (PDM_V2_BOM.md)
8. Assemble and test

**Timeline**: 2-3 weeks from design to working prototype

---

**PCB Layout is critical for thermal performance. Follow thermal via guidelines carefully.**
