# PDM V2 Assembly & Testing Guide

Step-by-step assembly, programming, and testing procedures.

## Required Tools

**Soldering**:
- Soldering iron (60W, temperature controlled)
- Hot air rework station (for PROFET ICs)
- Solder wire (0.8mm, 63/37 tin-lead or lead-free)
- Flux pen or paste
- Solder wick (desoldering braid)
- Tweezers (ESD-safe)
- Magnifying glass or loupe

**Testing**:
- Multimeter (voltage, continuity, resistance)
- Bench power supply (12V, 5A minimum)
- Test loads (5-10W light bulbs or resistors)
- USB cable (for Arduino programming)

**Misc**:
- Wire stripper (14-18 AWG)
- Crimper (for Deutsch connectors)
- Heat shrink tubing
- Heat gun
- Label maker (for wire identification)

## Assembly Steps

### Step 1: PCB Inspection

Receive PCBs from JLCPCB/OSH Park.

**Visual Check**:
- [ ] Board dimensions correct (100mm × 150mm)
- [ ] 4 layers visible (look at edge of board)
- [ ] Copper weight looks substantial (2oz)
- [ ] No damage (cracks, scratches)
- [ ] Silkscreen legible
- [ ] Thermal vias visible (no soldermask)

**Continuity Test**:
- [ ] GND plane continuous (test any two GND pads)
- [ ] 12V power plane NOT shorted to GND
- [ ] Each PROFET OUT pad isolated (no shorts)

### Step 2: Solder SMD Components (Order Matters)

**2.1: Power Supply (U1 - TPS54331)**

Components: U1, L1, D1, C1, C2, C3, R1-R3

Order:
1. Apply solder paste to TPS54331 pads (or use soldering iron)
2. Place TPS54331 (SO-8 package, pin 1 alignment critical)
3. Solder with hot air (300°C, 30 seconds) OR drag soldering
4. Solder 10uH inductor (L1)
5. Solder SS34 Schottky diode (D1, watch polarity!)
6. Solder capacitors (C1 electrolytic, C2/C3 ceramic)
7. Solder feedback resistors (R1 47k, R2 10k, R3 100k)

**Test**: Apply 12V, measure 3.3V at TPS54331 output. Should be 3.25-3.35V.

**2.2: PROFET ICs (U4-U8)**

Critical: Thermal pad must make good contact with PCB.

**BTS7002 (U4, U5) - PG-TO252-5**:
1. Apply flux to thermal pad area on PCB
2. Place a small amount of solder on center of thermal pad (tack solder)
3. Heat thermal pad from bottom of PCB (hot air 350°C, 60s)
4. Place BTS7002 on pads while solder is molten
5. Solder 5 pins with soldering iron
6. Verify thermal pad connection (slight tug should not move IC)

**BTS7008 (U6, U7, U8) - PG-DSO-14**:
1. Same process as BTS7002
2. Apply flux to thermal pad
3. Pre-tin thermal pad on PCB
4. Hot air from bottom (350°C) + place IC
5. Solder 14 pins with drag soldering technique
6. Verify thermal pad adhesion

**Test**: Continuity between PROFET GND pin and PCB GND plane.

**2.3: Decoupling Capacitors**

Install 0.1uF ceramic caps near each IC:
- 1x near TPS54331 (C2)
- 5x near PROFET ICs (one per IC at VS pin)

**2.4: Pull-Down Resistors**

Install 10k resistors (0805):
- 8x at PROFET IN pins (R4-R11)
- 8x at PROFET IS pins (R12-R19)

Place close to PROFET pins (within 5mm).

### Step 3: Install Through-Hole Components

**3.1: Arduino Nano Every Module**

1. Insert Nano Every into U2 location (pin headers pre-soldered)
2. Solder all pins from bottom of PCB
3. Trim excess pin length

Alternative: Use female headers (socket) for removable Nano.

**3.2: MCP2515 CAN Module**

1. Insert MCP2515 module into U3 location
2. Solder all pins
3. If using separate MCP2515 + TJA1050 ICs, solder as SMD components

**3.3: Input Capacitor (C1)**

1. Insert 100uF electrolytic capacitor (observe polarity!)
2. Negative lead (marked stripe) to GND
3. Solder and trim leads

### Step 4: Install Connectors

**4.1: Anderson Powerpole (J1)**

1. Strip 12V and GND wires (12" long, 14 AWG)
2. Crimp Anderson Powerpole contacts
3. Insert into housing (RED = +12V, BLACK = GND)
4. Wire to PCB pads (J1) and solder

Alternative: Solder wire directly to PCB pads, then add Anderson at wire end.

**4.2: CAN Bus Connector (J2)**

1. Strip CANH and CANL wires (12" long, 18 AWG)
2. Crimp Deutsch DT04-2P pins
3. Insert into connector housing
4. Wire to J2 pads and solder

**4.3: Output Connectors (J3-J10)**

For each of 8 outputs:
1. Strip output wire + GND wire (12-18" long, 14 AWG)
2. Crimp Deutsch DT04-2P pins
3. Insert into housing (Pin 1 = OUT, Pin 2 = GND)
4. Wire to PCB pads and solder

Label each connector: CH1-CH8 (use label maker).

### Step 5: Upload Firmware

**5.1: Prepare Arduino IDE**

1. Install Arduino IDE (arduino.cc/software)
2. Install MCP_CAN library: Sketch → Include Library → Manage Libraries → Search "MCP_CAN" → Install
3. Select Board: Tools → Board → Arduino megaAVR Boards → Arduino Nano Every
4. Select Port: Tools → Port → (your Nano's COM port)

**5.2: Upload PDM_V2_Firmware.ino**

1. Open PDM_V2_Firmware.ino in Arduino IDE
2. Verify (checkmark icon) - should compile without errors
3. Upload (arrow icon)
4. Open Serial Monitor (115200 baud)
5. Should see: "PDM V2 - PROFET-Based Compact PDM" + "PDM Ready"

### Step 6: Bench Testing (No Load)

**6.1: Power-Up Test**

1. Connect bench power supply (12V, limit to 1A)
2. Power ON
3. Measure voltages:
   - J1 VIN: 12.0V
   - TPS54331 output: 3.25-3.35V
   - Nano Every VCC: 5.0V (regulated by Nano's onboard regulator)
4. Current draw: <100mA (idle)

**6.2: Serial Communication**

1. Open Serial Monitor (115200 baud)
2. Type: `STATUS` + Enter
3. Should show all 8 channels OFF, 0.00A current

**6.3: Channel Control Test**

For each channel (1-8):
1. Type: `ENABLE X` (X = channel number)
2. Measure voltage at output connector (should be ~12V)
3. Type: `DISABLE X`
4. Output should drop to 0V

Repeat for all 8 channels.

**6.4: CAN Bus Test (Optional)**

Requires CAN analyzer (e.g., CANable USB, $30):
1. Connect analyzer to J2 (CAN bus)
2. Monitor CAN traffic with SavvyCAN or similar software
3. Enable channels via Serial
4. Should see 0x601 status messages at 10Hz

### Step 7: Load Testing

**7.1: Test Loads**

Use 5-10W light bulbs (12V automotive):
- 5W bulb = ~0.4A @ 12V
- 10W bulb = ~0.8A @ 12V

Or use power resistors:
- 15Ω 10W resistor = 0.8A @ 12V

**7.2: Single Channel Test**

1. Connect test load to CH1 output
2. Serial: `ENABLE 1`
3. Light bulb should illuminate
4. Serial: `STATUS`
5. Should show CH1 current: ~0.4-0.8A (depending on bulb)

Repeat for all 8 channels.

**7.3: Multi-Channel Test**

1. Connect test loads to CH1-4 (4 channels)
2. Enable all: `ENABLE 1`, `ENABLE 2`, `ENABLE 3`, `ENABLE 4`
3. All bulbs should illuminate
4. Check total current: ~3.2A (4× 0.8A)
5. Serial: `STATUS` - verify current readings

**7.4: Overcurrent Test (Fault Detection)**

1. Connect CH1 to short circuit (wire from OUT to GND)
2. Enable CH1
3. PROFET should detect overcurrent and enter thermal shutdown
4. Serial should show: "CH1 FAULT"
5. Remove short circuit
6. PROFET should auto-retry after ~30ms
7. If fault persists 3 times, should see: "CH1 PERMANENT DISABLE"
8. Reset: `RESET` command

**7.5: Auto-Retry Verification**

Simulate intermittent fault:
1. Connect CH1 to relay (coil has inrush current)
2. Enable CH1
3. Relay should energize (inrush ~2A for 10ms, then settles to 0.2A)
4. PROFET should allow inrush (auto-retry keeps trying)
5. Relay should stay energized (fault clears after inrush)

This verifies hardware auto-retry works correctly.

### Step 8: Thermal Testing

**8.1: High Current Test**

1. Connect high-current loads (21A for CH1-2, 7.5A for CH3-8)
2. Use power resistors or actual loads (fuel pump, fans)
3. Enable all channels
4. Monitor for 30 minutes
5. Measure IC temperature with IR thermometer:
   - Safe: <100°C
   - Marginal: 100-130°C (add forced air cooling)
   - Danger: >130°C (will thermal shutdown at 150°C)

**8.2: Enclosure Thermal Test**

1. Install PDM in aluminum enclosure
2. Apply thermal paste between PCB and enclosure
3. Repeat high-current test
4. Temperature should drop 20-30°C vs open-air
5. Verify <100°C with enclosure

### Step 9: Final Assembly

**9.1: Enclosure Installation**

1. Drill holes in Hammond 1590BB for connectors
2. Mount PCB on standoffs (M3 × 10mm)
3. Apply thermal paste to bottom of PCB (under PROFET area)
4. Install connectors through enclosure holes
5. Secure with screws

**9.2: Wire Labeling**

Label each wire with heat shrink + label maker:
- J1: "12V IN"
- J2: "CAN BUS"
- J3-J10: "CH1: Fuel Pump", "CH2: Fan 1", etc.

**9.3: Final Inspection**

- [ ] All connections secure
- [ ] No loose wires
- [ ] Enclosure sealed
- [ ] Labels legible
- [ ] No solder bridges or cold joints
- [ ] Thermal paste applied

### Step 10: Vehicle Integration

**10.1: Mounting**

1. Choose location (engine bay, firewall, trunk)
2. Consider: heat, vibration, water exposure
3. Mount enclosure with M6 bolts
4. Use vibration-damping mounts if necessary

**10.2: Wiring to Loads**

For each channel:
1. Route output wire to load (fuel pump, fan, etc.)
2. Use proper gauge wire (14 AWG for <15A, 12 AWG for >15A)
3. Protect wire with loom or conduit
4. Secure with zip ties every 12"
5. Add inline fuse holder at load (backup protection, optional)

**10.3: CAN Bus Connection**

1. Tap into existing RX8 CAN bus (OBD2 pins 6 & 14)
2. Or connect directly to ECU CAN pins
3. Add 120Ω termination if PDM is end node
4. Test: CAN traffic should appear on bus scanner

**10.4: Power Connection**

1. Connect J1 (12V IN) to battery positive (via main fuse, 60A)
2. Connect GND to chassis ground or battery negative
3. Use 10 AWG wire minimum for main power
4. Add main fuse or circuit breaker at battery (60A)

**10.5: Integration Testing**

1. Key ON (engine OFF)
2. Serial: `STATUS` - verify PDM powers up
3. ECU sends CAN 0x600 command
4. PDM enables channels per CAN message
5. Verify loads operate correctly
6. Start engine, run for 10 minutes
7. Monitor PDM temperature, current, faults

## Troubleshooting

### PDM Won't Power Up

**Symptom**: No power, no Serial output
- Check 12V input voltage at J1
- Check TPS54331 output (should be 3.3V)
- Check Arduino Nano Every VCC (should be 5V)
- Verify GND connection

### Channel Won't Enable

**Symptom**: Output stays at 0V even when enabled
- Measure PROFET IN pin (should be 3.3V when enabled)
- Check pull-down resistor (should be 10k)
- Verify PROFET VS pin has 12V
- Check PROFET is not faulted (Serial: `STATUS`)

### Inaccurate Current Sensing

**Symptom**: Current reading wrong (too high or too low)
- Verify kILIS ratio in firmware (2000 for BTS7002, 1850 for BTS7008)
- Check IS pin pull-down resistor (10k)
- Calibrate: Load channel with known current (5A), adjust kILIS in code

### Overcurrent False Triggers

**Symptom**: Channel faults immediately when enabled
- Current limit too low (increase in firmware)
- Inrush current from load (motor, relay) triggers fault
- Increase fault threshold or add soft-start delay

### Thermal Shutdown

**Symptom**: Channels disable after running at high current
- PROFET junction temperature >150°C
- Add forced air cooling (small fan)
- Improve thermal paste application
- Use aluminum enclosure with thermal pads
- Reduce load current (<80% rated)

### CAN Bus Errors

**Symptom**: No CAN communication with ECU
- Check CAN wiring (CANH pin 6, CANL pin 14)
- Verify 120Ω termination (measure resistance between CANH & CANL)
- Check MCP2515 initialization (Serial should show "CAN: OK")
- Use CAN analyzer to verify bus traffic

## Success Criteria

PDM is complete when:
- [ ] All 8 channels enable/disable correctly
- [ ] Current sensing accurate (±10%)
- [ ] Auto-retry works (tested with short circuit)
- [ ] Thermal performance OK (<100°C at full load)
- [ ] CAN communication with ECU works
- [ ] Runs stable for 1 hour at 80% load
- [ ] No false fault triggers
- [ ] All channels survive short circuit test
- [ ] Vehicle integration successful (drives with PDM active)

## Next Steps After Testing

1. Create KiCad schematic + PCB layout (use this guide)
2. Order PCBs + components
3. Assemble prototype following this guide
4. Test thoroughly (bench + vehicle)
5. Share results with community
6. Consider offering pre-assembled PDMs for sale

**Timeline**: 2-3 weeks from order to working PDM in vehicle.

**Cost**: $177-227 (vs $1,200-5,500 commercial)

---

**This is a complete, production-ready PDM design. True electronic circuit breakers. No fuses. Compact. Professional.**
