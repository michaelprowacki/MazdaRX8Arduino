# RX8 BCM & CAN Bus Research Compilation

**Last Updated**: 2025-11-17
**Research Goal**: Discover BCM (Body Control Module) CAN messages for door locks, windows, lighting, and other body functions
**Status**: In Progress - New discoveries from Chamber of Understanding blog

---

## Table of Contents

1. [Research Summary](#research-summary)
2. [Confirmed CAN Messages](#confirmed-can-messages)
3. [New Discoveries (2025-11-17)](#new-discoveries-2025-11-17)
4. [Hardware Architecture](#hardware-architecture)
5. [Unknown BCM Functions](#unknown-bcm-functions)
6. [Discovery Methodology](#discovery-methodology)
7. [Source References](#source-references)

---

## Research Summary

### What We Know
- **High-Speed CAN Bus**: 500 kbps on OBD2 pins 6 & 14
- **Primary Messages**: Engine/cluster communication (0x201, 0x420, etc.)
- **Hardware**: MCP2515 CAN controller + MCP2551 line driver (industry standard)
- **Cluster Pinout**: CANL (Green/Black), CANH (Light Blue/White)

### What We Don't Know (Yet)
- **BCM Body Functions**: Door locks, power windows, interior lights, trunk release
- **Climate Control**: HVAC fan speeds, temperature set points
- **Advanced Lighting**: Headlight control, turn signals (via CAN)

### Recent Breakthroughs
✅ **Chamber of Understanding** blog (Parts 5 & 21) provided working code with:
- Immobilizer handshake stored in EEPROM
- Odometer increment logic (microsecond calculation)
- Megasquirt ECU integration (0x5F0, 0x5F2, 0x5F3)
- Bitwise operations for warning lights

---

## Confirmed CAN Messages

### Messages We Currently Use

| ID (Hex) | ID (Dec) | Function | Bytes | Our Implementation |
|----------|----------|----------|-------|-------------------|
| **0x201** | 513 | RPM, Speed, Throttle | 8 | ✅ RX8_CANBUS.ino:207-218 |
| **0x420** | 1056 | MIL, Temp, Odometer | 7 | ✅ RX8_CANBUS.ino:171-205 |
| **0x212** | 530 | ABS/DSC Warnings | 7 | ⚠️ Optional (line 268) |
| **0x620** | 1568 | ABS System Data | 7 | ✅ RX8_CANBUS.ino:92 |
| **0x630** | 1584 | ABS Config (AT/MT) | 8 | ✅ RX8_CANBUS.ino:93 |
| **0x047** | 71 | Immobilizer Request | 8 | ✅ RX8_CANBUS.ino:296-304 |
| **0x041** | 65 | Immobilizer Response | 8 | ✅ RX8_CANBUS.ino:298-303 |
| **0x4B0** | 1200 | Wheel Speed Data | 8 | ✅ RX8_CANBUS.ino:306-322 |

### Newly Discovered Messages (Chamber of Understanding)

| ID (Hex) | ID (Dec) | Function | Source | Integration Status |
|----------|----------|----------|--------|-------------------|
| **0x5F0** | 1520 | Megasquirt ECU Data 1 | chamberofunderstanding.co.uk Part 21 | ❌ Not in our code |
| **0x5F2** | 1522 | Megasquirt ECU Data 2 | chamberofunderstanding.co.uk Part 21 | ❌ Not in our code |
| **0x5F3** | 1523 | Megasquirt ECU Data 3 | chamberofunderstanding.co.uk Part 21 | ❌ Not in our code |

### Messages Mentioned But Not Implemented

| ID (Hex) | ID (Dec) | Function | Source | Priority |
|----------|----------|----------|--------|----------|
| **0x300** | 768 | Steering Warning | topolittle/RX8-CAN-BUS | Low |
| **0x231** | 561 | Transmission Status | Our code (line 91) | Medium |
| **0x081** | 129 | Steering Angle | topolittle/RX8-CAN-BUS | Low |

---

## New Discoveries (2025-11-17)

### 🔍 Chamber of Understanding Blog - Key Findings

**Source**: https://www.chamberofunderstanding.co.uk/
- **Part 5**: CAN bus basics, hardware, cluster pinout
- **Part 21**: Working code with immobilizer, odometer, Megasquirt integration

#### 1. Immobilizer Handshake Storage (EEPROM Method)

**Our Current Method**:
```cpp
// RX8_CANBUS.ino lines 296-304
// Live handshake response - responds to 0x047 in real-time
if (buf[1] == 127 && buf[2] == 2) {
    send41a = {7, 12, 48, 242, 23, 0, 0, 0};
    CAN0.sendMsgBuf(0x041, 0, 8, send41a);
}
```

**Chamber of Understanding Method**:
- **Stores handshake in EEPROM** after first successful authentication
- Replays stored handshake on subsequent starts
- **Advantage**: Faster startup, no live response needed
- **Disadvantage**: Requires initial "learning" phase

**Potential Integration**:
- Implement EEPROM storage as backup
- Use live response as primary (more flexible)
- Store last successful handshake for offline diagnostics

---

#### 2. Odometer Increment Logic

**Discovery**: Chamber blog implements **odometer increments based on wheel speed**

**Our Current Implementation**:
```cpp
// RX8_CANBUS.ino - We set engine temp but NOT odometer
send420[0] = engTemp;  // Byte 0: Engine temp (145 = normal)
send420[1] = 0;        // Byte 1: Odometer (static)
```

**Chamber of Understanding Implementation**:
```cpp
// Calculates microsecond intervals based on vehicle speed
// Increments odometer byte at correct rate to match actual distance
// Formula: microseconds_per_tenth_mile = function(vehicle_speed)
```

**How It Works**:
1. Read vehicle speed from wheel sensors (0x4B0)
2. Calculate time interval for 0.1 mile increment
3. Increment `send420[1]` at calculated rate
4. Cluster displays accurate odometer reading

**Implementation Priority**: **HIGH** ⭐
- This would make our ECU replacement display accurate mileage
- Critical for legal compliance (odometer tampering laws)
- Relatively simple to implement

**Action Item**: Create `updateOdometer()` function
```cpp
// Pseudo-code for odometer increment
void updateOdometer() {
    static unsigned long lastOdometerUpdate = 0;
    static byte odometerByte = 0;

    if (vehicleSpeed > 0) {
        // Calculate microseconds per 0.1 mile based on speed
        unsigned long interval = calculateOdometerInterval(vehicleSpeed);

        if (micros() - lastOdometerUpdate >= interval) {
            odometerByte++;
            send420[1] = odometerByte;
            lastOdometerUpdate = micros();
        }
    }
}

unsigned long calculateOdometerInterval(int mph) {
    // 0.1 mile = 528 feet
    // At 60 mph = 1 mile/minute = 0.1 mile/6 seconds = 6,000,000 microseconds
    // Formula: (6,000,000 * 60) / mph
    return (360000000UL) / mph;  // UL = unsigned long literal
}
```

---

#### 3. Megasquirt ECU Integration

**Discovery**: Chamber blog reads from **Megasquirt aftermarket ECU** via CAN bus

**CAN Message IDs**:
- **0x5F0 (1520)**: Megasquirt data channel 1
- **0x5F2 (1522)**: Megasquirt data channel 2
- **0x5F3 (1523)**: Megasquirt data channel 3

**Purpose**:
- Read RPM, throttle position, coolant temp from Megasquirt
- Display on RX8 cluster
- Alternative to our Arduino-based ECU

**Relevance to Our Project**:
- ✅ **Highly relevant** - Many RX8 engine swaps use Megasquirt
- ✅ Could add Megasquirt support as alternative to Arduino ECU
- ✅ Users could choose: Arduino ECU *or* Megasquirt + our cluster bridge

**Implementation Priority**: **MEDIUM** ⚠️
- Not critical for our current Arduino ECU
- But valuable for community (many use Megasquirt)
- Could create separate "Megasquirt Bridge" module

**Action Item**: Document Megasquirt CAN protocol
```cpp
// Add to RX8_CANBUS.ino (optional feature)
#ifdef USE_MEGASQUIRT
    if (ID == 0x5F0) {
        // Parse Megasquirt data format
        engineRPM = (buf[0] << 8) | buf[1];  // Example - verify actual format
    }
#endif
```

---

#### 4. Warning Light Bitwise Operations

**Discovery**: Chamber blog uses **bitwise operations** for individual warning lights

**Our Current Method**:
```cpp
// RX8_CANBUS.ino lines 186-204
if (checkEngineMIL == 1) {
    send420[5] = send420[5] | 0b01000000;  // Set bit 6
} else {
    send420[5] = send420[5] & 0b10111111;  // Clear bit 6
}
```

**Chamber Method**: **Same approach!** ✅
- Confirms our implementation is correct
- Uses same bit positions for warning lights
- Good validation of our reverse engineering

**Byte 5 (send420[5]) Bit Map**:
```
Bit 7: Check Engine Backlight
Bit 6: Check Engine MIL (Main Indicator Lamp)
Bit 5: Unknown
Bit 4: Unknown
Bit 3: Unknown
Bit 2: Unknown
Bit 1: Unknown
Bit 0: Unknown
```

**Byte 6 (send420[6]) Bit Map**:
```
Bit 7: Oil Pressure Warning
Bit 6: Battery Charge Warning
Bit 5: Unknown
Bit 4: Unknown
Bit 3: Unknown
Bit 2: Unknown
Bit 1: Low Water/Coolant Warning
Bit 0: Unknown
```

---

### 🔌 RX8 Cluster Pinout (Confirmed)

**Source**: chamberofunderstanding.co.uk Part 5

**Smaller Connector (CAN Bus + Power)**:

| Pin | Wire Color | Function |
|-----|------------|----------|
| Upper 1 | Black/Yellow (B/Y) | Ignition-switched +12V |
| Upper 2 | Black (B) | Ground |
| Upper 3 | Light Blue/Red (L/R) | Permanent +12V (memory) |
| Lower 1 | Green/Black (G/B) | **CANL** (CAN Bus Low) |
| Lower 2 | Light Blue/White (L/W) | **CANH** (CAN Bus High) |
| Lower 3 | Green/Yellow (G/Y) | Unknown |
| Lower 4 | Brown/Black (BR/B) | Unknown |
| Lower 5 | Red/Yellow (R/Y) | Unknown |

**Verification**:
- ✅ Matches our OBD2 connection (pins 6 & 14)
- ✅ Confirms 500 kbps High-Speed CAN
- ✅ Standard twisted-pair CAN wiring

---

### 📊 Hardware Recommendations (Validated)

**Chamber of Understanding** uses same hardware as us:

| Component | Our Choice | Their Choice | Status |
|-----------|------------|--------------|--------|
| CAN Controller | MCP2515 | MCP2515 | ✅ Match |
| CAN Transceiver | MCP2551 | MCP2551 | ✅ Match |
| Microcontroller | Arduino Leonardo | Arduino (unspecified) | ⚠️ Similar |

**Why These Chips?**
- DIP package availability (breadboard-friendly)
- Well-documented Arduino libraries
- Proven reliability in automotive environments

**Alternative Consideration** (from our AUTOMOTIVE_MCU_MIGRATION.md):
- **STM32F407**: Built-in CAN controller (-40°C to 125°C)
- **TI C2000**: Automotive-grade, motor control optimized
- **NXP S32K**: ISO 26262 ASIL-B safety certification

---

## Hardware Architecture

### CAN Bus Network Topology

```
OBD2 Port (Pins 6 & 14)
    │
    ├─ High-Speed CAN (500 kbps)
    │
    ├── PCM/ECU ──────────► 0x201, 0x420, 0x212, 0x231, 0x240
    ├── ABS/DSC ──────────► 0x620, 0x630, 0x650
    ├── Instrument Cluster ─► (Receives all above messages)
    ├── BCM ──────────────► ❓ Unknown messages (door locks, windows, lights)
    ├── Immobilizer ──────► 0x047 (request), 0x041 (response)
    └── Wheel Speed ──────► 0x4B0, 0x4B1

Medium-Speed CAN (125 kbps) - Separate bus, not connected to OBD2
    ├── Audio System
    ├── Navigation
    └── Climate Control Display
```

**Key Insight**: BCM functions may be on High-Speed CAN (accessible via OBD2) or Medium-Speed CAN (separate network, harder to access).

---

## Unknown BCM Functions

### High-Priority Targets (Likely on High-Speed CAN)

| Function | Likelihood | Estimated Difficulty | Safety Risk |
|----------|------------|----------------------|-------------|
| Door Locks (Lock/Unlock) | **90%** | Easy | Low |
| Interior Lighting (On/Off) | **80%** | Easy | None |
| Trunk Release | **70%** | Easy | Low |
| Power Windows (Up/Down) | **60%** | Medium | **High** (pinch) |
| Turn Signal Indicators | **50%** | Medium | Medium |

### Medium-Priority Targets

| Function | Likelihood | Estimated Difficulty | Safety Risk |
|----------|------------|----------------------|-------------|
| Fuel Door Release | **40%** | Easy | None |
| Wiper Speed Control | **30%** | Medium | Low |
| Horn Activation | **20%** | Hard | Medium (security) |

### Low-Priority / Unlikely (Probably Medium-Speed CAN or PWM)

| Function | Likelihood | Estimated Difficulty | Safety Risk |
|----------|------------|----------------------|-------------|
| HVAC Fan Speed | **10%** | Hard | Low |
| HVAC Temperature Set Point | **5%** | Very Hard | Low |
| Headlight Control | **10%** | Hard | **High** (safety-critical) |
| Remote Start | **0%** | Impossible | **Critical** (immobilizer security) |

---

## Discovery Methodology

### Phase 1: Baseline Capture (1 hour)

**Objective**: Record normal CAN traffic to identify existing patterns

**Tools Needed**:
- Arduino with MCP2515 CAN module
- Our BCM sniffer sketch (examples/BCM_Control_Example/)
- Serial monitor or CAN analyzer software

**Steps**:
1. Upload BCM sniffer sketch to Arduino
2. Connect to OBD2 port (pins 6 & 14)
3. Start vehicle (or ignition ON)
4. Press **'S'** for sniff mode
5. Record all CAN IDs for **10 minutes**:
   - Engine idling
   - Doors closed
   - Windows closed
   - Lights off
   - HVAC off

**Expected Results**:
- Continuous messages: 0x201, 0x420, 0x620, 0x630, 0x4B0 (engine/cluster/ABS)
- Intermittent messages: 0x047/0x041 (immobilizer, every 30-60 seconds)
- **Unknown messages**: Document frequency and byte patterns

**Data Format**:
```
Baseline capture - 2025-11-17 14:30:00
ID: 0x201, Freq: 10 Hz, Bytes: [00 05 FF FF 27 10 00 FF]
ID: 0x420, Freq: 10 Hz, Bytes: [91 00 00 00 01 00 00]
ID: 0x????, Freq: ?, Bytes: [? ? ? ? ? ? ? ?]  ← New discovery!
```

---

### Phase 2: Function Triggering (2-4 hours)

**Objective**: Identify CAN messages that change when specific functions are activated

**High-Priority Tests** (in order):

#### Test 1: Door Lock Detection (30 minutes)
1. **Baseline**: Record CAN traffic with doors unlocked
2. **Action**: Lock doors with key fob
3. **Observe**: Which CAN IDs appear or change?
4. **Action**: Unlock doors with key fob
5. **Observe**: Do messages reverse or disappear?
6. **Document**: New CAN ID(s), byte patterns, timing

**Expected Discovery**:
- New CAN ID (e.g., 0x3XX or 0x4XX range)
- Single byte or bit change (e.g., Byte 0, Bit 0 = lock state)
- Message sent **once** on state change (not continuous)

#### Test 2: Interior Lights (20 minutes)
1. **Baseline**: Lights off
2. **Action**: Press dome light button
3. **Observe**: New or changed CAN messages?
4. **Repeat**: With door open (automatic light)
5. **Repeat**: With dimmer knob (if applicable)

#### Test 3: Trunk Release (15 minutes)
1. **Baseline**: Trunk closed
2. **Action**: Press trunk release button (dashboard or key fob)
3. **Observe**: CAN message spike?
4. **Verify**: Is it momentary (pulse) or sustained?

#### Test 4: Power Windows (45 minutes)
1. **Baseline**: Windows closed
2. **Action**: Driver window down (button press)
3. **Observe**: Message changes?
4. **Repeat**: Passenger, rear left, rear right windows
5. **Test**: Auto-down feature (single press vs. hold)
6. **Test**: Auto-up feature (pinch detection active!)

**⚠️ Safety Warning**: Do NOT test window control in vehicle without supervision. Pinch detection failure could cause injury.

#### Test 5: Turn Signals (30 minutes)
1. **Baseline**: Turn signals off
2. **Action**: Left turn signal
3. **Observe**: Flashing CAN message at ~1 Hz?
4. **Repeat**: Right turn signal
5. **Repeat**: Hazard lights (both)

**Comparison Table** (example):
| Function | New CAN ID | Changed Bytes | Frequency | Type |
|----------|-----------|---------------|-----------|------|
| Door Lock | 0x??? | Byte 0, Bit 0 | Once on change | State |
| Door Unlock | 0x??? | Byte 0, Bit 0 | Once on change | State |
| Dome Light | 0x??? | Byte 1 | Continuous | State |
| Trunk Release | 0x??? | Byte 2 | Pulse (100ms) | Event |

---

### Phase 3: Message Replay (1 hour)

**Objective**: Confirm discovered messages by replaying them

**Tools**:
- Arduino CAN transmitter (our existing code)
- Vehicle with doors locked/unlocked

**Steps**:
1. **Capture Door Lock Message**:
   - Example: `ID: 0x3B5, Bytes: [01 00 00 00 00 00 00 00]`
2. **Add to Arduino Code**:
   ```cpp
   byte doorLock[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
   CAN0.sendMsgBuf(0x3B5, 0, 8, doorLock);
   ```
3. **Test in Vehicle**:
   - Upload code
   - Send message
   - **Observe**: Do doors lock?
4. **Verify Repeatability**:
   - Test 10 times
   - Document success rate
   - Check for side effects (warning lights, error codes)

**Potential Issues**:
- **Checksums**: Message may have calculated checksum byte (trial and error)
- **Sequence Numbers**: Some messages include incrementing counter
- **Authentication**: BCM may require specific sequence (unlock before lock)
- **Timing**: Message may need specific interval (e.g., max 1/second)

**Mitigation**:
- Try different byte values (0x00, 0x01, 0xFF)
- Increment suspected checksum bytes
- Compare successful key fob messages to manual sends
- Add delays between commands

---

### Phase 4: Documentation (30 minutes)

**Objective**: Share discoveries with RX8 community

**Deliverables**:
1. **Update this document** (RX8_BCM_CANBUS_RESEARCH.md)
2. **Create message reference** (Documentation/CAN_PID_Reference.md)
3. **Update DBC file** (Documentation/rx8_can_database.dbc)
4. **Post to GitHub** (create issue or pull request)
5. **Share on RX8Club.com** (with credit to contributors)

**Template for New Discovery**:
```markdown
## CAN ID 0xXXX - Door Lock Control

**Function**: Lock/Unlock doors
**Discovered By**: [Your Name]
**Date**: 2025-11-17
**Confirmed On**: 2004 RX8 Series 1 (adjust for your car)

**Message Structure**:
| Byte | Bit(s) | Function | Values |
|------|--------|----------|--------|
| 0 | 0 | Lock State | 0 = Unlocked, 1 = Locked |
| 0 | 1-7 | Unknown | 0 (static) |
| 1-7 | All | Unknown | 0 (static) |

**Example Messages**:
- Lock: `[01 00 00 00 00 00 00 00]`
- Unlock: `[00 00 00 00 00 00 00 00]`

**Replay Test**: ✅ Confirmed working (10/10 success)
**Side Effects**: None observed
**Update Frequency**: Once on state change (not continuous)

**Code Example**:
```cpp
void lockDoors() {
    byte msg[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    CAN0.sendMsgBuf(0xXXX, 0, 8, msg);
}
```
```

---

## Source References

### Primary Sources (Working Code & Documentation)

#### ✅ Chamber of Understanding Blog (David Blackhurst - Same Author!)
- **URL**: https://www.chamberofunderstanding.co.uk/
- **Relevance**: **CRITICAL** - Same author as our project!
- **Content**:
  - Part 5: CAN basics, cluster pinout, hardware choices
  - Part 21: Working code (immobilizer EEPROM, odometer, Megasquirt)
- **Key Contributions**:
  - Odometer increment logic
  - Immobilizer handshake storage
  - Megasquirt integration (0x5F0, 0x5F2, 0x5F3)
- **Status**: ✅ Partially accessible (Parts 5 & 21)
- **Action Item**: Visit Parts 1-4, 6-20, 22+ manually to find more CAN IDs

#### ✅ Autosport Labs - RX8 Series 1 CAN Mapping
- **URL**: https://www.autosportlabs.com/mazda-rx8-series-1-direct-can-mapping/
- **Relevance**: High - Racing telemetry preset
- **Content**:
  - 9 channels of engine data
  - Bypasses OBDII for faster streaming
  - **Important**: "Disable all OBDII channels to prevent high RPM rev limit" (manual transmission)
- **Status**: ⚠️ Limited info (no detailed message IDs published)
- **Action Item**: Contact Autosport Labs for full preset file

#### ⚠️ Collins Performance Technologies - RX8 Wiring Emulator
- **URL**: https://collinsperformancetechnologies.com/products/rx-8-wiring-emulator-for-canbus
- **Relevance**: Medium - Commercial product (may have proprietary CAN knowledge)
- **Content**: Product page structure only (technical details not scraped)
- **Status**: ❌ Inaccessible via web scraping
- **Action Item**: Call 803-792-7189 for technical specifications

---

### Secondary Sources (GitHub Repositories)

#### ✅ topolittle/RX8-CAN-BUS
- **URL**: https://github.com/topolittle/RX8-CAN-BUS
- **Relevance**: Medium - Racing telemetry focus
- **Content**:
  - CAN message IDs for racing (RPM, speed, steering angle, temps)
  - Mentions 0x081 (steering angle), 0x300 (steering warning)
- **Status**: ✅ Documented in our CAN_PID_Reference.md

#### ✅ majbthrd/MazdaCANbus
- **URL**: https://github.com/majbthrd/MazdaCANbus
- **Relevance**: Low - Minimal documentation
- **Content**:
  - rx8.kcd file (CAN database, only 0x201 and 0x430)
  - Good template format
- **Status**: ✅ Reviewed, limited value

#### ✅ Mrkvak/cx30-can
- **URL**: https://github.com/Mrkvak/cx30-can
- **Relevance**: Medium - Newer Mazda (CX-30) but similar architecture
- **Content**:
  - **Fuse-pulling methodology** (clever!)
  - Confirms "BCM controls lighting, climate, door locks, windows"
  - Shows BCM is on High-Speed CAN (500 kbps)
- **Status**: ✅ Methodology applicable to RX8

---

### Community Forums (Blocked from Web Scraping)

#### ❌ RX8Club.com - CAN Bus Components Thread
- **URL**: https://www.rx8club.com/series-i-tech-garage-22/rx8-can-bus-components-266179/
- **Relevance**: **HIGH** - Community knowledge base
- **Status**: ❌ Blocked (403 Forbidden)
- **Action Item**: Visit manually, search for BCM/door lock mentions

#### ❌ Chamber of Understanding Blog (Parts 1-4, 6-20, 22+)
- **URLs**: https://www.chamberofunderstanding.co.uk/
  - Part 1-4: CAN basics, reverse engineering methodology
  - Part 6-20: Unknown (possibly BCM discoveries!)
  - Part 22+: Latest updates
- **Status**: ⚠️ Parts 5 & 21 accessible, rest blocked
- **Action Item**: Visit manually to read full series

---

### External Tools & Databases

#### ✅ rx8_can_database.dbc
- **Location**: `Documentation/rx8_can_database.dbc`
- **Source**: https://github.com/rnd-ash/rx8-reverse-engineering
- **Content**: CAN signal definitions (importable to SavvyCAN, Wireshark)
- **Status**: ✅ Integrated into our repository

#### ✅ CAN_PID_Reference.md
- **Location**: `Documentation/CAN_PID_Reference.md`
- **Source**: Our compilation of multiple sources
- **Content**: Comprehensive CAN message reference
- **Status**: ✅ Maintained and updated

---

## Next Steps

### Immediate Actions (You Can Do Today)

1. **Visit Chamber Blog Manually** 🌐
   - Read Parts 1-4, 6-20, 22+ at https://www.chamberofunderstanding.co.uk/
   - Look for BCM-specific CAN IDs (door locks, windows, lights)
   - Document any new message IDs found
   - **Est. Time**: 2-3 hours

2. **Implement Odometer Increment** ⚙️
   - Create `updateOdometer()` function (see "New Discoveries" section above)
   - Test on bench (verify calculation with wheel speed simulation)
   - Test in vehicle (compare to actual mileage)
   - **Est. Time**: 1-2 hours
   - **Priority**: HIGH ⭐

3. **Visit RX8Club.com Thread** 🔍
   - Manually open https://www.rx8club.com/series-i-tech-garage-22/rx8-can-bus-components-266179/
   - Search thread for "door" "lock" "window" "BCM" "body"
   - Document any CAN IDs mentioned
   - **Est. Time**: 30 minutes

---

### Short-Term (This Week)

4. **Deploy BCM Sniffer** 📡
   - Upload `examples/BCM_Control_Example/` sketch
   - Perform Phase 1: Baseline Capture (see "Discovery Methodology")
   - Document unknown CAN IDs
   - **Est. Time**: 1 hour
   - **Priority**: MEDIUM ⚠️

5. **Test Door Lock Discovery** 🚗
   - Perform Phase 2: Function Triggering (door locks only)
   - Compare locked vs. unlocked CAN traffic
   - Identify door lock CAN ID
   - **Est. Time**: 30 minutes
   - **Priority**: MEDIUM ⚠️

6. **Contact Autosport Labs** 📧
   - Request full RX8 Series 1 CAN preset file
   - Ask about BCM message support
   - Share our findings in exchange
   - **Est. Time**: 15 minutes
   - **Priority**: LOW

---

### Long-Term (Community Effort)

7. **Map All BCM Functions** (10-20 hours)
   - Systematic testing of all body functions
   - Create comprehensive BCM CAN message library
   - Update DBC file with new discoveries
   - **Priority**: MEDIUM (but time-intensive)

8. **Create BCM Control Library** (5-10 hours)
   - `bcm_control.h` / `bcm_control.cpp`
   - Functions: `lockDoors()`, `unlockDoors()`, `openTrunk()`, etc.
   - Integrate with WiFi dashboard (optional)
   - **Priority**: LOW (depends on discovery success)

9. **Share with RX8 Community** (1 hour)
   - Post to RX8Club.com
   - Update GitHub repository
   - Write blog post or YouTube video
   - **Priority**: HIGH (after discoveries)

---

## Safety & Legal Considerations

### ⚠️ Safety Warnings

1. **Power Windows**:
   - DO NOT disable pinch detection
   - Test with supervision only
   - Risk of injury if manual control fails

2. **Door Locks**:
   - Ensure manual override works (mechanical key)
   - Risk of lockout if BCM control fails
   - Test unlock before testing lock

3. **Horn/Lights**:
   - Unintended activation could violate traffic laws
   - Horn stuck ON could drain battery
   - Test in private area only

4. **Immobilizer**:
   - DO NOT modify without backup key
   - Risk of vehicle immobilization
   - EEPROM storage untested in our code

### ⚙️ Legal Considerations

1. **Odometer Tampering**:
   - Federal law prohibits odometer rollback (49 USC § 32703)
   - Our implementation INCREMENTS only (legal)
   - Document mileage before/after installation
   - Disclose ECU replacement if selling vehicle

2. **Emissions Testing**:
   - OBD2 readiness monitors may fail
   - Check local emissions requirements
   - May not pass inspection in strict states (CA, NY, etc.)

3. **Warranty Voidance**:
   - Any ECU replacement voids factory warranty
   - Disclose to insurance company (may affect coverage)
   - Document all modifications

4. **Liability**:
   - This is experimental automotive code
   - Use at your own risk
   - Author not liable for vehicle damage or injury

---

## Acknowledgments

### Contributors
- **David Blackhurst** (dave@blackhurst.co.uk) - Original RX8_CANBUS.ino author
- **David Blackhurst** - Chamber of Understanding blog (Parts 1-21+)
- **topolittle** - RX8-CAN-BUS racing telemetry repository
- **Autosport Labs** - RX8 Series 1 CAN mapping preset
- **Mrkvak** - CX-30 fuse-pulling methodology
- **RX8Club.com community** - Years of collective reverse engineering

### Tools Used
- **Arduino IDE** - Development environment
- **MCP2515/MCP2551** - CAN hardware (industry standard)
- **SavvyCAN** - CAN analysis software
- **Wireshark** - Protocol analysis (with DBC import)

---

## Version History

| Date | Version | Changes |
|------|---------|---------|
| 2025-11-17 | 1.0 | Initial research compilation from Chamber blog findings |

---

**This document is a living resource. Please contribute discoveries!**

**Contact**: michaelprowacki (GitHub) or dave@blackhurst.co.uk (original author)

---

*End of RX8 BCM & CAN Bus Research Document*
