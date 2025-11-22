# PDM V2 Bill of Materials (BOM)

Complete parts list with Mouser/Digikey part numbers.

## Power Management ICs

| Qty | Part Number | Description | Price | Source | Notes |
|-----|-------------|-------------|-------|--------|-------|
| 2 | BTS7002-1EPP | Infineon PROFET, 21A, single channel | $4.50 | Mouser 726-BTS7002-1EPP | CH1-2: Fuel pump, Fan 1 |
| 3 | BTS7008-2EPA | Infineon PROFET, 7.5A, dual channel | $3.20 | Mouser 726-BTS7008-2EPA | CH3-8: Fans, pumps, lights |

**Alternative Sources**:
- Digikey: BTS7002-1EPP-ND, BTS7008-2EPA-ND
- Direct: Infineon.com (higher MOQ)

## Microcontroller

| Qty | Part Number | Description | Price | Source | Notes |
|-----|-------------|-------------|-------|--------|-------|
| 1 | Arduino Nano Every | ATmega4809, 5V, 20MHz | $10 | Arduino.cc | Pre-assembled module |

**Alternatives**:
- STM32F103C8T6 "Blue Pill" ($2.50, AliExpress) - requires STM32 toolchain
- Arduino Nano Clone ($3-5, Amazon) - ATmega328P, works but less RAM

## CAN Bus

| Qty | Part Number | Description | Price | Source | Notes |
|-----|-------------|-------------|-------|--------|-------|
| 1 | MCP2515 | CAN controller | $2 | Amazon/eBay | Pre-assembled module with MCP2515 + TJA1050 |

**Alternative**: Separate chips
- MCP2515-I/SO ($1.50, Mouser 579-MCP2515-I/SO)
- TJA1050T/CM ($0.80, Mouser 771-TJA1050T/CM,118)

## Power Supply

| Qty | Part Number | Description | Price | Source | Notes |
|-----|-------------|-------------|-------|--------|-------|
| 1 | TPS54331DR | Buck converter, 3.3V 3A | $1.50 | Mouser 595-TPS54331DR | SO-8 package |
| 1 | 10uH inductor | Power inductor, 3A, DO3316P | $0.40 | Mouser 80-SRN6045-100M | Shielded |
| 1 | SS34 | Schottky diode, 3A 40V | $0.15 | Mouser 625-SS34-E3 | DO-214AC (SMA) |

## Connectors

| Qty | Part Number | Description | Price | Source | Notes |
|-----|-------------|-------------|-------|--------|-------|
| 1 | Anderson Powerpole 45A | Main power input (12V) | $8 | Amazon | Red/black pair |
| 2 | Deutsch DT04-2P | 2-pin connector (CAN bus) | $3.50 | TE Connectivity | Waterproof |
| 8 | Deutsch DT04-2P | 2-pin connector (outputs) | $28 | TE Connectivity | 8x CH outputs |

**Alternative Connectors** (Budget Option):
- Amp Superseal 1.5mm ($1.50 each, TE Connectivity 282080-1)
- Total savings: $15

## Capacitors

| Qty | Part Number | Description | Price | Source | Notes |
|-----|-------------|-------------|-------|--------|-------|
| 2 | 100uF 25V | Electrolytic, main power | $0.25 | Mouser 647-UWT1E101MCL1GS | 6.3×5mm |
| 10 | 0.1uF | Ceramic, X7R 0805 | $1.00 | Mouser 80-C0805C104K5R | Decoupling |
| 5 | 10uF | Ceramic, X5R 0805 | $1.50 | Mouser 81-GRM21BR61A106KE9L | Bulk |
| 1 | 47uF 6.3V | Ceramic, output cap | $0.50 | Mouser 81-GRM31CR60J476ME9L | 3.3V rail |

## Resistors (0805 SMD, 1%)

| Qty | Value | Description | Price | Source | Notes |
|-----|-------|-------------|-------|--------|-------|
| 16 | 10kΩ | PROFET pull-down | $0.80 | Mouser 603-RC0805FR-0710KL | 8x IN + 8x IS |
| 2 | 47kΩ | Feedback divider (top) | $0.10 | Mouser 603-RC0805FR-0747KL | TPS54331 |
| 2 | 10kΩ | Feedback divider (bottom) | $0.10 | Mouser 603-RC0805FR-0710KL | TPS54331 |
| 1 | 100kΩ | EN pull-up | $0.05 | Mouser 603-RC0805FR-07100KL | TPS54331 |
| 1 | 120Ω | CAN termination (optional) | $0.05 | Mouser 603-RC0805FR-07120RL | If end node |

## PCB

| Qty | Specification | Description | Price | Source | Notes |
|-----|---------------|-------------|-------|--------|-------|
| 1 | 4-layer, 2oz copper | 100mm × 150mm | $50 | JLCPCB | 5pcs minimum ($10/pc) |
|  |  |  | $100 | OSH Park | 3pcs ($33/pc) |
|  |  |  | $30 | PCBWay | 5pcs ($6/pc) |

**PCB Specs**:
- Layers: 4 (Signal/GND/Power/Signal)
- Copper: 2oz (70μm) on all layers
- Surface Finish: ENIG (gold) preferred
- Soldermask: Black
- Silkscreen: White
- Min trace/space: 0.2mm/0.2mm (8mil/8mil)
- Min via: 0.3mm drill, 0.6mm pad

## Enclosure & Mounting

| Qty | Part Number | Description | Price | Source | Notes |
|-----|-------------|-------------|-------|--------|-------|
| 1 | Hammond 1590BB | Aluminum enclosure, 4.7"×3.7"×1.2" | $12 | Mouser 546-1590BB | Unpainted |
| 4 | M3×10mm standoffs | PCB standoffs, aluminum | $2 | Amazon | 4-pack |
| 4 | M3×6mm screws | Socket head cap screws | $1 | Amazon | Stainless |
| 1 | Thermal paste | Arctic MX-4, 4g tube | $8 | Amazon | For thermal pads |

**Alternative Enclosure**: Waterproof IP65 ABS ($25, Amazon) for engine bay

## Wire & Terminals

| Qty | Specification | Description | Price | Source | Notes |
|-----|---------------|-------------|-------|--------|-------|
| 10ft | 14 AWG | Stranded copper wire, red | $5 | Amazon | Output wiring |
| 10ft | 14 AWG | Stranded copper wire, black | $5 | Amazon | Ground wiring |
| 5ft | 18 AWG | Stranded copper wire, red | $3 | Amazon | Signal wiring |
| 20 | Ring terminals | 14 AWG, 1/4" hole | $5 | Amazon | 20-pack |
| 1 | Heat shrink kit | Assorted sizes | $10 | Amazon | Wire protection |

## Tools (If Needed)

| Qty | Item | Description | Price | Source | Notes |
|-----|------|-------------|-------|--------|-------|
| 1 | Soldering station | Temp controlled, 60W | $40 | Amazon | Hakko FX-888D |
| 1 | Hot air rework | SMD rework station | $50 | Amazon | For PROFET ICs |
| 1 | Multimeter | Basic DMM | $20 | Amazon | Fluke 115 or similar |
| 1 | Wire stripper | 14-18 AWG | $15 | Amazon | Automatic |
| 1 | Crimper | Deutsch terminals | $30 | Amazon | DMC crimper |

## BOM Summary

### By Category

| Category | Subtotal |
|----------|----------|
| PROFET ICs (5 chips) | $18.60 |
| Microcontroller (Nano Every) | $10.00 |
| CAN module (MCP2515+TJA1050) | $2.00 |
| Power supply (TPS54331 + inductor + diode) | $2.05 |
| Connectors (Anderson + 10x Deutsch) | $39.00 |
| Passives (caps, resistors) | $5.00 |
| PCB (JLCPCB, 5pcs) | $50.00 |
| Enclosure + mounting | $23.00 |
| Wire + terminals | $28.00 |
| **Total (per unit, 5 PCB order)** | **$177.65** |

### Single Unit (OSH Park PCB)

| PCB (OSH Park, 3pcs) | $100.00 |
|----------------------|---------|
| Components | $127.65 |
| **Total (single unit)** | **$227.65** |

### Cost Comparison

| Option | Cost per Unit | MOQ | Total Outlay |
|--------|---------------|-----|--------------|
| **JLCPCB (5 PCBs)** | $177.65 | 5 | $888 |
| **OSH Park (3 PCBs)** | $227.65 | 3 | $683 |
| **PCBWay (5 PCBs)** | $157.65 | 5 | $788 |

**Recommendation**: JLCPCB for lowest per-unit cost, OSH Park for single prototype.

## Where to Buy

**ICs (Infineon PROFET)**:
- Mouser Electronics (mouser.com) - Best availability
- Digikey (digikey.com) - Alternative
- Arrow Electronics (arrow.com) - Bulk discounts

**Arduino Nano Every**:
- Arduino Store (arduino.cc) - Official
- Amazon - Fast shipping
- AliExpress - Budget ($5-7, longer shipping)

**CAN Module (MCP2515)**:
- Amazon - MCP2515 CAN module ($8-12 pre-assembled)
- eBay - Cheaper ($3-5, longer shipping)
- AliExpress - Bulk ($2-3 each)

**PCB Fabrication**:
- JLCPCB (jlcpcb.com) - Cheapest, 1-2 week delivery
- OSH Park (oshpark.com) - USA-made, 2 week delivery
- PCBWay (pcbway.com) - Good quality, fast turnaround

**Connectors**:
- TE Connectivity (direct or through Mouser/Digikey)
- Amazon - Anderson Powerpole, Superseal alternatives

**Misc (wire, terminals, enclosures)**:
- Amazon - Fast delivery, free shipping with Prime
- McMaster-Carr - Industrial quality

## Assembly Options

**Option 1: Full DIY** ($177.65)
- Order PCB + components separately
- Hand solder all components
- Tools required: Soldering iron, hot air station
- Time: 3-4 hours

**Option 2: JLCPCB SMT Assembly** ($177.65 + $50 assembly)
- Upload BOM + pick-and-place file
- JLCPCB assembles SMD components
- You solder connectors (through-hole)
- Time: 1 hour (just connectors)

**Option 3: Pre-assembled (dingoPDM reference)** ($350-400)
- Buy from dingo-electronics.square.site
- Fully assembled and tested
- No soldering required

## Notes

**Lead Times**:
- PCBs: 5-10 days (JLCPCB/PCBWay), 12-15 days (OSH Park)
- Components (Mouser): 2-3 days shipping
- Amazon: 1-2 days (Prime)
- AliExpress: 2-4 weeks

**Shipping Costs**:
- Mouser: Free over $50
- JLCPCB: $15-25 (DHL express)
- OSH Park: $10-15 (USPS)
- Amazon: Free (Prime)

**Currency**: All prices in USD

**Price Volatility**: PROFET IC prices fluctuate. Check current pricing before ordering.

**Minimum Order Quantities (MOQ)**:
- Most components: No MOQ (1 piece OK)
- PCBs: 5pcs (JLCPCB), 3pcs (OSH Park)
- If ordering 5 PCBs, total outlay ~$900, but you get 5 working PDMs ($180/each)

## Shopping Cart Links

**Mouser Cart** (copy/paste into Mouser cart):
```
BTS7002-1EPP (qty 2)
BTS7008-2EPA (qty 3)
TPS54331DR (qty 1)
SRN6045-100M (qty 1)
SS34-E3 (qty 1)
UWT1E101MCL1GS (qty 2)
C0805C104K5R (qty 10)
GRM21BR61A106KE9L (qty 5)
RC0805FR-0710KL (qty 18)
RC0805FR-0747KL (qty 2)
RC0805FR-07100KL (qty 1)
```

**Amazon Search Terms**:
- "Arduino Nano Every"
- "MCP2515 CAN module"
- "Anderson Powerpole 45A"
- "Hammond 1590BB"
- "14 AWG stranded wire kit"
- "Heat shrink tube kit"

---

**Total Cost: $177-227 depending on PCB option**

**Next: Order parts, assemble prototype, test with bench loads**
