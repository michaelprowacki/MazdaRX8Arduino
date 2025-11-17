# RX8 BCM CAN Messages - Research Summary

**Date**: 2025-11-16
**Status**: Partial documentation available, BCM messages need discovery

---

## What We Know (Confirmed CAN Messages)

### From Multiple Sources (GitHub, RusEFI Wiki, Community)

These messages are **well documented and confirmed**:

| CAN ID | Function | Data Format | Source | Status |
|--------|----------|-------------|--------|--------|
| **0x201** | Speed & RPM | RPM (16-bit × 3.85), Speed (16-bit, km/h × 0.01 - 10000) | Multiple | ✅ Implemented |
| **0x212** | ABS/DSC/TC warnings | Bit flags for warnings | RusEFI, Our code | ✅ Implemented |
| **0x420** | MIL, Temp, Oil warnings | Byte 0: Temp, Byte 5-6: Warning bits | RusEFI, Our code | ✅ Implemented |
| **0x300** | Steering warning light | Turn off steering warning (min 0.5s timing) | RusEFI | ⚠️ Not implemented |
| **0x231** | Transmission status | Neutral detection via bit flag | topolittle repo | ⚠️ Not implemented |
| **0x250** | Engine temperature | Single byte, offset -40°C | topolittle repo | ⚠️ Not implemented |
| **0x321** | Auto transmission gear | D0: Gear (1=P, 2=R, 3=N, 4=D), D1: Manual mode | Community | ⚠️ Not implemented |
| **0x081** | Steering angle | Signed 16-bit angle value | topolittle repo | ⚠️ Not implemented |
| **0x430** | Instrument cluster | Unknown signals | majbthrd/MazdaCANbus | ⚠️ Undocumented |

---

## What We DON'T Know (BCM Body Control)

**No public documentation found** for these functions:

- ❌ Door locks (lock/unlock commands)
- ❌ Power windows (up/down/auto)
- ❌ Interior lighting (dome/map lights)
- ❌ Exterior lighting (headlights/turn signals via CAN)
- ❌ HVAC fan control (override speed)
- ❌ Trunk/fuel door release
- ❌ Horn activation
- ❌ Wiper control (beyond basic speed-based in our code)

---

## Key Research Findings

### 1. GitHub Repositories

#### majbthrd/MazdaCANbus ⭐
- **URL**: https://github.com/majbthrd/MazdaCANbus
- **Format**: KCD files (can be converted with CANBabel, canmatrix)
- **Content**:
  - `rx8.kcd` - RX8 CAN database (partial)
  - `skyactiv.kcd` - Newer Mazda CAN database
- **Limitation**: Only has 0x201 (speed/RPM) and 0x430 (undefined) documented
- **Value**: Good starting template for DBC/KCD format

#### topolittle/RX8-CAN-BUS ⭐
- **URL**: https://github.com/topolittle/RX8-CAN-BUS
- **Focus**: Racing telemetry PIDs
- **Content**: RPM, speed, throttle, steering angle, temperature
- **Limitation**: No BCM body control messages
- **Value**: Confirms our existing implementations, adds some new sensors

#### Mrkvak/cx30-can
- **URL**: https://github.com/Mrkvak/cx30-can
- **Vehicle**: Mazda CX-30 (2019+, different generation)
- **Methodology**: Fuse-pulling to identify CAN IDs
- **Key Finding**: "BCM connects to Body CAN (500 kbaud) for lighting, climate, door locks, windows, liftgate"
- **Value**: Shows BCM architecture (may differ from RX8)

### 2. CAN Bus Architecture (From CX-30, Likely Similar for RX8)

**RX8 has TWO CAN buses**:

```
High-Speed CAN (500 kbps) - OBD2 Pins 6 & 14:
├── PCM/ECU (Engine Control)
├── TCM (Transmission)
├── ABS/DSC Module
├── Instrument Cluster
├── Power Steering Module
├── BCM (Body Control Module) ← Controls body functions
└── Immobilizer

Medium-Speed CAN (125 kbps) - Unknown pins:
├── Audio System
├── Navigation (if equipped)
└── Multi-function Display
```

**BCM likely controls** (based on CX-30 research):
- Door locks
- Power windows
- Interior/exterior lighting
- HVAC fan (climate control)
- Liftgate/trunk
- Wiper control (advanced)

### 3. Reverse Engineering Methodology

**From Mrkvak/cx30-can** - Fuse Pulling Method:

1. Record baseline CAN traffic
2. Pull fuse for specific system (e.g., F19 for power steering)
3. Compare CAN traffic - missing IDs are from that system
4. Document findings

**Example**:
```
F35 fuse (ABS) pulled:
Missing IDs: 078, 079, 203, 211, 215, 217, 219, 223, 415, 596
→ These are ABS-related CAN messages
```

Could apply this to RX8:
- Pull BCM fuse (which one?)
- See which CAN IDs disappear
- Those are BCM messages

---

## Community Resources

### Forums

**RX8Club.com**:
- Lots of mechanical door lock/window repair threads
- **No CAN bus reverse engineering found**
- Users mostly discuss actuator replacements

**MSExtra.com**:
- Has thread on CAN to instrument cluster
- **No BCM body control reverse engineering**

### Blogs (Blocked from scraping)

**madox.net** - "Reverse Engineering the Mazda CAN Bus" (2008-2009)
- Part 1: http://www.madox.net/blog/2008/11/17/reverse-engineering-the-mazda-can-bus-part-1/
- Part 2: http://www.madox.net/blog/2009/10/24/reverse-engineering-the-mazda-can-bus-–-part-2/
- **Status**: Website blocked web scraping (403 error)
- **Recommendation**: Visit manually in browser

**chamberofunderstanding.co.uk** - "RX8 Project" series
- Part 6: https://www.chamberofunderstanding.co.uk/2017/12/02/rx8-project-part-6-canbus-2/
- Part 21: https://www.chamberofunderstanding.co.uk/2021/06/11/rx8-project-part-21-canbus-6-working-code/
- **Status**: Website blocked web scraping (403 error)
- **Recommendation**: Visit manually in browser

---

## Likely CAN IDs for BCM (Educated Guesses)

Based on typical automotive CAN bus architecture and Mazda patterns:

| Function | Likely ID Range | Confidence | Reasoning |
|----------|----------------|------------|-----------|
| Door Locks | 0x400-0x4FF | Medium | BCM typically uses 0x4xx range |
| Power Windows | 0x400-0x4FF | Medium | Same module as locks |
| Interior Lights | 0x400-0x4FF | Low | BCM functions grouped |
| HVAC Fan | 0x500-0x5FF | Low | Climate control separate module |
| Trunk Release | 0x400-0x4FF | Low | BCM function |
| Horn | 0x400-0x4FF | Very Low | Security/alarm related |

**⚠️ WARNING**: These are GUESSES. Must be verified through sniffing!

---

## Additional Confirmed Info

### From RusEFI Wiki

**Instrument Cluster Connector**:
- Blue/White: CAN High
- Green/Black: CAN Low
- Black: Ground
- Black/Yellow: +12V (power)
- Blue/Red: +12V (power)

**CAN Bus Locations**:
- HS-CAN: OBD2 pins 6 (CAN-H) and 14 (CAN-L)
- MS-CAN: OBD2 pins 3 and 11 (unknown which is H/L)

**Cross-Platform Compatibility**:
- RusEFI notes CAN protocols "likely apply to Mazda6 and MX-5 as well"
- This suggests some Mazda-wide standardization

---

## Checksum/Security

**From CX-30 research**:
- Some messages have checksums (e.g., ID 0x440)
- Checksum algorithm: Additive with constants (0xB4, 0x5B)
- May need to implement checksum calculation for BCM messages

**For RX8**:
- Unknown if BCM messages have checksums
- Unknown if BCM requires authentication for sensitive functions
- Need to test if replayed messages are accepted

---

## Action Items for Discovery

### Phase 1: Baseline Capture (1 hour)
- [ ] Connect CAN sniffer to OBD2
- [ ] Record 5 minutes of normal operation
- [ ] Save as `baseline.csv`
- [ ] Analyze message frequency and patterns

### Phase 2: Function Triggering (2-4 hours)
For each function:
- [ ] Clear buffer
- [ ] Start recording
- [ ] Trigger function (lock doors, open window, etc.)
- [ ] Stop recording
- [ ] Compare to baseline
- [ ] Document new/changed messages

Functions to test:
1. Door lock (key fob)
2. Door unlock (key fob)
3. Driver window up (switch)
4. Driver window down (switch)
5. Passenger window up
6. Passenger window down
7. Interior light on (door open)
8. Headlight flash (turn signal stalk)
9. Horn (button)
10. Trunk release (if accessible via switch)

### Phase 3: Message Replay (1 hour)
- [ ] Replay discovered messages from Arduino
- [ ] Verify function actually works
- [ ] Test for checksums (does it work without checksum?)
- [ ] Test for authentication (does BCM reject our messages?)

### Phase 4: Documentation (30 min)
- [ ] Update `docs/rx8_can_database.dbc` with findings
- [ ] Create message format documentation
- [ ] Submit to this repo
- [ ] Share on RX8Club forums

---

## Tools Needed

**Hardware** (Already Have):
- ✅ Arduino Mega 2560
- ✅ MCP2515 CAN module
- ✅ OBD2 cable/splitter

**Software** (Free):
- ✅ SavvyCAN (https://github.com/collin80/SavvyCAN)
- ✅ CANBabel (format conversion)
- ✅ Our CAN sniffer sketch (`examples/BCM_Control_Example/`)

**Optional**:
- USB-CAN adapter ($50-100) - Faster than Arduino serial
- Professional CAN tools (Vector, Kvaser) - Overkill for hobbyist

---

## Realistic Expectations

### What's Likely to Work
- ✅ **Door locks** - Simple on/off command, no security
- ✅ **Interior lights** - Read-only status, control unlikely
- ⚠️ **Power windows** - May work, but safety (pinch detection) concerns
- ⚠️ **Wiper control** - May work, but need to understand rain sensor integration

### What's Unlikely to Work
- ❌ **Remote start** - Requires immobilizer bypass (security risk)
- ❌ **HVAC temperature** - Likely proprietary HVAC module protocol
- ❌ **Headlight control** - Safety-critical, may have authentication
- ❌ **Horn** - Security function, may have authentication

### Unknown
- ❓ **Trunk release** - Could be simple command or require security
- ❓ **Fuel door release** - Mechanical vs. electronic (year dependent)

---

## Safety & Legal Notes

**Before Attempting**:
- ⚠️ Test on YOUR vehicle only (hacking others = illegal)
- ⚠️ Don't interfere with safety systems (ABS, airbags, immobilizer)
- ⚠️ Ensure manual overrides still work (door lock/unlock handles)
- ⚠️ Don't control while driving (distraction hazard)
- ⚠️ Understand warranty implications (if applicable to old RX8s)

**Best Practices**:
- Test in controlled environment (empty parking lot)
- Have helper monitor laptop while driving
- Document all changes
- Keep original ECU as backup
- Implement authentication for unlock commands

---

## Conclusion

**What We Have**:
- ✅ Complete implementation of engine/cluster CAN messages (0x201, 0x212, 0x420, etc.)
- ✅ Working CAN sniffer tool
- ✅ Methodology for discovery
- ✅ Example code for replay
- ✅ Safety guidelines

**What We Need**:
- 🔍 **Community effort** to discover BCM CAN message IDs
- 🔍 Testing on real RX8 vehicles (multiple years/models)
- 🔍 Validation of message formats
- 🔍 Documentation of findings

**Estimated Time to Discover**:
- Door locks: 1-2 hours
- Power windows: 1-2 hours
- Full BCM mapping: 10-20 hours

**Recommendation**:
1. **Manual visit** to madox.net and chamberofunderstanding.co.uk blogs (sites blocked scraping)
2. **Start discovery** with door locks (easiest, lowest risk)
3. **Document findings** and share with community
4. **Build library** once enough messages discovered

---

## References

**GitHub Repositories**:
- https://github.com/majbthrd/MazdaCANbus - RX8 CAN database (KCD format)
- https://github.com/topolittle/RX8-CAN-BUS - Racing telemetry PIDs
- https://github.com/Mrkvak/cx30-can - CX-30 reverse engineering (methodology)

**Wikis**:
- https://github.com/rusefi/rusefi/wiki/Mazda-RX8-2004 - Confirmed CAN IDs

**Blogs** (visit manually):
- http://www.madox.net/blog/projects/mazda-can-bus/
- https://www.chamberofunderstanding.co.uk (search "RX8 CAN")

**Forums**:
- https://www.rx8club.com/ (search "CAN bus")
- https://www.msextra.com/forums/ (search "RX8 CAN")

**Our Documentation**:
- `docs/BCM_CONTROL_GUIDE.md` - Full methodology guide
- `docs/CAN_PID_Reference.md` - Known CAN messages
- `docs/rx8_can_database.dbc` - DBC file for known messages
- `examples/BCM_Control_Example/` - CAN sniffer sketch

---

**Last Updated**: 2025-11-16
**Contributors**: Community research compilation
**Status**: Research phase - discovery needed
**Next Steps**: Visit blocked blogs manually, start discovery with door locks

**Help Wanted**: If you discover BCM CAN messages, please contribute to this repo!
