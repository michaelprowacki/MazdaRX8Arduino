# MCU Comparison for PDM V2

Arduino Nano is easy but not optimal for safety-critical automotive applications.

## Requirements for Automotive PDM

**Must Have**:
- Temperature range: -40°C to 125°C (engine bay extremes)
- Hardware watchdog timer (auto-reset if code freezes)
- CAN bus controller (built-in, not external MCP2515)
- 12-bit ADC (current sensing accuracy)
- Multiple timers (PWM generation)
- Sufficient GPIO (8 outputs + 8 analog inputs minimum)

**Nice to Have**:
- Automotive-grade qualification (AEC-Q100)
- ESD protection (human body model >2kV)
- EMI tolerance (ISO 11452-2)
- Dual-core for safety redundancy
- Error correction code (ECC) on flash/RAM

## MCU Options

### Option 1: Arduino Nano Every (Current Choice)

**Specs**:
- MCU: ATmega4809 (Microchip)
- Clock: 20 MHz
- Flash: 48KB, RAM: 6KB
- Temperature: 0°C to 85°C (NOT automotive grade)
- CAN: NO (requires external MCP2515)
- Watchdog: Yes (software, not hardware-enforced)
- ADC: 10-bit
- Cost: $10

**Pros**:
- Easy to program (Arduino IDE)
- 5V logic (direct PROFET compatibility)
- Large community support
- No external tools needed

**Cons**:
- Temperature range inadequate (engine bay = -40°C to 120°C)
- No built-in CAN (requires MCP2515 + extra cost)
- 10-bit ADC (lower current sensing accuracy)
- Not automotive-qualified
- No hardware-enforced watchdog

**Verdict**: OK for prototyping, NOT for production use.

---

### Option 2: STM32F103C8T6 "Blue Pill" (Recommended)

**Specs**:
- MCU: STM32F103C8T6 (STMicroelectronics)
- Clock: 72 MHz
- Flash: 64KB, RAM: 20KB
- Temperature: -40°C to 85°C (industrial grade)
- CAN: YES (built-in bxCAN controller)
- Watchdog: YES (hardware IWDG, cannot be disabled by software)
- ADC: 2x 12-bit (1 μs conversion, 16 channels)
- Cost: $2.50 (AliExpress), $4 (Mouser genuine ST)
- Automotive variant: STM32F103C8Tx (extended temp -40°C to 105°C)

**Pros**:
- Built-in CAN controller (no MCP2515 needed, saves $8)
- Hardware watchdog (code freeze = auto-reset within 26ms)
- 12-bit ADC (better current sensing, 0.024% resolution)
- 3.3V logic (lower power, PROFET compatible)
- 72 MHz (3.6× faster than Nano)
- Industrial/automotive temp range
- Cheap ($2.50-4)

**Cons**:
- Requires ST-Link programmer ($10)
- Steeper learning curve (STM32CubeIDE or Arduino STM32 core)
- 3.3V logic (PROFET IN pins need >2V, so OK, but less margin than 5V)

**Programming Options**:
1. STM32CubeIDE (free, professional, ST official)
2. Arduino IDE + STM32 core (Roger Clark, easier)
3. PlatformIO + Arduino framework (best of both)

**Verdict**: Best balance of cost, features, safety. **RECOMMENDED**.

---

### Option 3: STM32F405RGT6 (Advanced)

**Specs**:
- MCU: STM32F405RGT6 (STMicroelectronics)
- Clock: 168 MHz
- Flash: 1MB, RAM: 192KB
- Temperature: -40°C to 85°C (automotive variants available: STM32F405xx-105°C)
- CAN: YES (2x bxCAN controllers - dual CAN!)
- Watchdog: YES (hardware IWDG + WWDG)
- ADC: 3x 12-bit (2.4 MSPS, DMA capable)
- FPU: YES (floating point unit for fast calculations)
- Cost: $8-12 (Mouser/Digikey)

**Pros**:
- Extremely powerful (168 MHz, FPU)
- Dual CAN bus (control + telemetry on separate buses)
- Massive flash/RAM (1MB/192KB)
- DMA-capable ADC (background current sensing, zero CPU overhead)
- Better EMI tolerance
- Automotive-grade variants available

**Cons**:
- More expensive ($12 vs $2.50)
- Overkill for simple PDM (but allows future expansion)
- Requires ST-Link programmer
- Larger package (LQFP64 vs LQFP48)

**Verdict**: Best for professional/commercial PDM. Overkill for DIY.

---

### Option 4: Texas Instruments TM4C123GH6PM (Tiva C)

**Specs**:
- MCU: TM4C123GH6PM (Texas Instruments)
- Clock: 80 MHz
- Flash: 256KB, RAM: 32KB
- Temperature: -40°C to 85°C (automotive variants: -40°C to 105°C)
- CAN: YES (2x CAN controllers)
- Watchdog: YES (hardware)
- ADC: 2x 12-bit (1 MSPS)
- Cost: $5-8

**Pros**:
- Dual CAN controllers
- Good ADC performance
- TI Code Composer Studio (free IDE)
- Automotive-qualified variants
- Popular in hobbyist community (Tiva LaunchPad $13)

**Cons**:
- Less popular than STM32 (smaller community)
- 3.3V logic only
- Requires programmer (can use LaunchPad as programmer)

**Verdict**: Good alternative to STM32, slightly more expensive.

---

### Option 5: NXP S32K144 (Professional Automotive)

**Specs**:
- MCU: S32K144 (NXP)
- Clock: 112 MHz
- Flash: 512KB, RAM: 64KB
- Temperature: -40°C to 150°C (AUTOMOTIVE GRADE)
- CAN: YES (2x FlexCAN - automotive CAN with enhanced features)
- Watchdog: YES (multiple, safety-compliant)
- ADC: 2x 12-bit (automotive-grade)
- ASIL: ISO 26262 ASIL-B capable (functional safety)
- Cost: $8-15

**Pros**:
- TRUE automotive-grade MCU
- 150°C junction temperature (engine bay safe)
- ISO 26262 ASIL-B safety certification path
- FlexCAN with enhanced error handling
- NXP S32 Design Studio (free)
- Used in production automotive ECUs

**Cons**:
- Most expensive ($15)
- Steeper learning curve (professional tool)
- Requires debugger (J-Link or P&E Micro)
- Overkill for DIY (but best for safety)

**Verdict**: Best for professional/commercial safety-critical systems.

---

### Option 6: Infineon AURIX TC275 (High-End)

**Specs**:
- MCU: TC275 (Infineon)
- Clock: 200 MHz (TriCore, 3× independent cores)
- Flash: 4MB, RAM: 472KB
- Temperature: -40°C to 150°C (automotive)
- CAN: YES (MultiCAN, up to 8 nodes)
- Watchdog: YES (safety watchdog, lockstep cores)
- ASIL: ISO 26262 ASIL-D capable (highest safety level)
- Cost: $30-50

**Pros**:
- Absolute best for safety-critical automotive
- Triple-core lockstep (one core verifies others)
- Used in production vehicle ECUs, ADAS systems
- Hardware memory protection
- ISO 26262 ASIL-D (highest safety integrity level)

**Cons**:
- Very expensive ($50)
- Complex to program (requires AURIX Studio)
- Massive overkill for PDM
- Requires expensive debugger (J-Link Ultra+)

**Verdict**: Professional motorsport only. Extreme overkill for DIY.

---

## Comparison Table

| MCU | Cost | CAN | Watchdog | ADC | Temp Range | Safety | Ease | Recommended |
|-----|------|-----|----------|-----|------------|--------|------|-------------|
| **Arduino Nano Every** | $10 | NO | Soft | 10-bit | 0-85°C | NO | Easy | Prototype only |
| **STM32F103** | $2.50 | YES | HW | 12-bit | -40-85°C | Good | Medium | **YES** |
| **STM32F405** | $12 | 2× YES | HW | 12-bit | -40-105°C | Better | Medium | Advanced |
| **TM4C123** | $8 | 2× YES | HW | 12-bit | -40-105°C | Good | Medium | Alternative |
| **NXP S32K144** | $15 | 2× YES | HW | 12-bit | -40-150°C | ASIL-B | Hard | Professional |
| **AURIX TC275** | $50 | 8× YES | HW | 12-bit | -40-150°C | ASIL-D | Very Hard | Motorsport |

## Recommendation for PDM V2

**For DIY/Hobbyist**: STM32F103C8T6 "Blue Pill"
- Cost: $2.50-4
- Built-in CAN (saves $8 vs Nano + MCP2515)
- Hardware watchdog (safety)
- 12-bit ADC (accuracy)
- -40°C to 85°C (adequate for most installs)
- Easy to program with Arduino IDE + STM32 core
- **Total savings: $8 + better reliability**

**For Professional/Commercial**: NXP S32K144
- Cost: $15
- -40°C to 150°C (true engine bay rating)
- ISO 26262 ASIL-B capable
- Production automotive ECU quality
- Worth the extra cost for safety-critical applications

## STM32F103 vs Arduino Nano (PDM V2)

**PCB Changes Required**:

| Feature | Arduino Nano | STM32F103 |
|---------|--------------|-----------|
| **MCU Package** | Through-hole module | LQFP48 SMD |
| **Size** | 18mm × 45mm | 7mm × 7mm |
| **CAN Transceiver** | MCP2515 + TJA1050 | TJA1050 only (CAN built-in) |
| **Components Saved** | — | MCP2515, SPI wiring, crystal |
| **Programming** | USB (built-in) | SWD header (4-pin) |
| **Power** | 5V regulator (built-in) | 3.3V from TPS54331 |
| **GPIO** | 5V tolerant | 3.3V (but OK for PROFET) |

**Firmware Changes**:
- Replace `mcp_can.h` with STM32 HAL CAN functions
- Port Arduino Serial to USART1
- Rest of code identical (same logic, timers, ADC)

**Cost Comparison**:
- Arduino Nano Every + MCP2515 module: $10 + $8 = $18
- STM32F103C8T6 + TJA1050: $2.50 + $0.80 = $3.30
- **Savings: $14.70 per unit**

**Safety Comparison**:

| Safety Feature | Arduino Nano | STM32F103 |
|----------------|--------------|-----------|
| Watchdog | Software (can be disabled by bug) | Hardware (cannot be disabled, auto-reset <26ms) |
| CAN Bus | External (failure = lost communication) | Built-in (hardware error handling) |
| ADC Accuracy | 10-bit (0.1% res) | 12-bit (0.024% res) |
| Error Correction | None | Built-in (brown-out detection) |
| EMI Tolerance | Consumer-grade | Industrial-grade |

**Recommendation**: Update PDM V2 design to use STM32F103C8T6.

## Implementation Plan

**Phase 1**: Create STM32F103 variant
- New schematic with STM32F103 + TJA1050
- Remove MCP2515 section
- Add SWD programming header
- Update PCB layout (smaller footprint)

**Phase 2**: Port firmware
- Use STM32CubeMX to generate project template
- Port Arduino code to STM32 HAL
- Or use Arduino STM32 core (easier)

**Phase 3**: Test & validate
- Same bench testing procedure
- Verify watchdog triggers on code freeze
- Validate CAN communication
- Thermal testing

**Timeline**: +1 week to redesign for STM32F103

---

**Verdict**: STM32F103C8T6 is significantly better than Arduino Nano Every for automotive PDM applications. Cheaper, safer, more reliable.

**Next Step**: Create PDM_V2.1 design with STM32F103 instead of Arduino Nano.
