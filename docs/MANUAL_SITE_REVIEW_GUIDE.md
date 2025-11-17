# Manual Site Review Guide - RX8 CAN Bus Research

**Goal**: Extract CAN bus message IDs and BCM control information from these sites

**Sites to Visit**:
1. RX8Club forum thread
2. Chamber of Understanding blog (Parts 5 & 21)
3. Collins Performance wiring emulator product page
4. AutosportLabs RX8 CAN mapping

---

## What to Look For

### CAN Message IDs

**Format to watch for**:
- Hexadecimal: `0x201`, `0x420`, `0x4B1`
- Decimal: `513`, `1056`, `1201`
- Text descriptions: "ID 201 for speed and RPM"

**Key BCM functions to search for**:
- Door lock/unlock
- Power windows (up/down/auto)
- Interior lights (dome, map)
- Exterior lights (headlights, turn signals)
- HVAC fan speed
- Trunk/fuel door release
- Horn/alarm
- Wiper control

### Data Formats

Look for byte-by-byte descriptions:
```
Example format to copy:
ID 0x410 - Door Locks
  Byte 0: Command (0x01 = lock, 0x02 = unlock)
  Byte 1-7: 0x00 (unused)
```

### Wiring Information

Look for:
- CAN High/Low wire colors
- Connector pin numbers
- OBD2 pin assignments
- BCM connector locations

### Code Examples

Look for:
- Arduino sketches
- CAN message sending examples
- Timing requirements (update rates)
- Checksum calculations

---

## Site-by-Site Review Checklist

### 1. RX8Club Forum
**URL**: https://www.rx8club.com/series-i-tech-garage-22/rx8-can-bus-components-266179/

**What to capture**:
- [ ] List of CAN bus components mentioned
- [ ] Any CAN IDs discussed in posts
- [ ] User-reported discoveries
- [ ] Links to other threads or resources
- [ ] Wiring diagrams (screenshot if possible)

**Search on page** (Ctrl+F):
- "0x"
- "ID"
- "BCM"
- "door"
- "lock"
- "window"

**Copy into**: `docs/RX8_BCM_RESEARCH.md` under "Forum Findings"

---

### 2. Chamber of Understanding - Part 5
**URL**: https://www.chamberofunderstanding.co.uk/2017/11/15/rx8-project-part-5-canbus/

**What to capture**:
- [ ] CAN message IDs discovered
- [ ] Data format descriptions
- [ ] Code snippets (copy entire code blocks)
- [ ] Timing information (update rates)
- [ ] Hardware setup used

**This is Part 5** - Likely covers basics
**Look for**:
- Introduction to RX8 CAN bus
- Initial message discovery
- Basic gauge control (RPM, speed)

**Copy into**: Text file or `docs/chamber_part5_notes.txt`

---

### 3. Chamber of Understanding - Part 21
**URL**: https://www.chamberofunderstanding.co.uk/2021/06/11/rx8-project-part-21-canbus-6-working-code/

**What to capture**:
- [ ] Complete working code (this is the key one!)
- [ ] All CAN message IDs
- [ ] Full Arduino sketch if available
- [ ] Message timing requirements
- [ ] Any BCM-specific findings

**This is Part 21** - "Working Code" suggests complete implementation
**Likely contains**:
- Complete CAN message library
- Tested and working examples
- Possibly BCM control

**Priority**: HIGH - Download/copy ALL code

**Copy into**: `examples/chamber_working_code/` (create new folder)

---

### 4. Collins Performance - Wiring Emulator
**URL**: https://collinsperformancetechnologies.com/products/rx-8-wiring-emulator-for-canbus

**What to capture**:
- [ ] Product description (what it does)
- [ ] Supported features list
- [ ] Wiring diagram (screenshot!)
- [ ] Technical specifications
- [ ] CAN message IDs mentioned
- [ ] Price (for comparison to our DIY cost)

**This is a commercial product** - They likely:
- Emulate PCM for engine swaps
- Control instrument cluster
- Handle immobilizer
- May have BCM functions

**Look for**:
- "Features" section
- Technical documentation
- Installation manual (PDF download?)
- Wiring diagrams

**Copy into**: `docs/commercial_solutions.md`

**Compare to our implementation**:
- What features do they have that we don't?
- What do we have that they don't?
- Price comparison

---

### 5. AutosportLabs - Direct CAN Mapping
**URL**: https://www.autosportlabs.com/mazda-rx8-series-1-direct-can-mapping/

**What to capture**:
- [ ] Complete CAN message mapping
- [ ] Direct CAN connection details
- [ ] Racing telemetry configuration
- [ ] Message IDs for logging
- [ ] Update rates

**AutosportLabs** makes RaceCapture data loggers
**Focus**: Racing telemetry via CAN bus

**Likely contains**:
- RPM, speed, throttle, temps (we have these)
- Wheel speeds (we have)
- Possibly boost pressure
- Possibly gear position
- Unlikely to have BCM (not racing-related)

**Look for**:
- Configuration file download
- JSON/XML with CAN IDs
- Wiring diagrams

**Copy into**: `docs/racing_telemetry_mapping.md`

---

## Extraction Template

For each CAN message you find, document in this format:

```markdown
### CAN ID: 0xXXX (Decimal: XXX)

**Function**: [Door locks, windows, etc.]

**Source**: [Site name, post #, date]

**Data Format**:
- Byte 0: [Description]
- Byte 1: [Description]
- ...

**Update Rate**: [100ms, 500ms, on-demand, etc.]

**Example**:
```cpp
byte msg[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
CAN0.sendMsgBuf(0xXXX, 0, 8, msg);
```

**Testing Notes**: [What happened when you tried it]

**Year/Model Tested**: [2004 MT, 2006 AT, etc.]
```

---

## Priority Order

**Visit in this order**:

1. **HIGHEST**: Chamber Part 21 (working code!)
2. **HIGH**: AutosportLabs (complete mapping)
3. **MEDIUM**: Chamber Part 5 (foundations)
4. **MEDIUM**: Collins Performance (commercial comparison)
5. **LOW**: RX8Club (forum discussion, may be scattered)

---

## Quick Reference - What We Already Know

**Don't waste time re-documenting these** (we already have them):

| CAN ID | Function | Status |
|--------|----------|--------|
| 0x201 | Speed & RPM | ✅ Have |
| 0x212 | ABS/DSC warnings | ✅ Have |
| 0x420 | MIL, temp, oil | ✅ Have |
| 0x4B1 | Wheel speeds | ✅ Have |
| 0x041 | Immobilizer response | ✅ Have |
| 0x047 | Immobilizer request | ✅ Have |

**Focus on finding**:
- Any IDs in 0x400-0x4FF range (likely BCM)
- Any IDs in 0x500-0x5FF range (likely HVAC)
- Door lock commands
- Window commands
- Lighting commands

---

## Tips for Efficient Extraction

### Browser Tools

1. **Page Search** (Ctrl+F / Cmd+F):
   - Search: "0x" (finds hex IDs)
   - Search: "byte" (finds data formats)
   - Search: "door", "lock", "window"

2. **Copy Code Blocks**:
   - Right-click code → Inspect Element
   - Look for `<code>` or `<pre>` tags
   - Copy entire block

3. **Screenshot Diagrams**:
   - Use Snipping Tool (Windows)
   - Use Screenshot (Mac: Cmd+Shift+4)
   - Save to `docs/images/`

### Note-Taking

**Create a working document**:
```
docs/manual_research_YYYYMMDD.md

Site: Chamber Part 21
Date visited: 2025-11-16
Key findings:
- ID 0x??? for door locks
- Code snippet: [paste here]
- Tested: [yes/no]

Site: AutosportLabs
...
```

### Code Extraction

When you find Arduino code:

1. **Copy entire sketch** (don't skip setup/loop)
2. **Note library dependencies** (what #includes are used)
3. **Copy to** `examples/[source_name]/`
4. **Try to compile** (check for errors)
5. **Document** what it does

---

## After Site Review

Once you've visited all sites, create a summary:

### Summary Document Template

```markdown
# Manual Research Summary - [Date]

## Sites Visited: 5/5

### Key Findings

**BCM CAN Messages Discovered**:
- 0x??? - Door locks (Source: [site])
- 0x??? - Windows (Source: [site])
- etc.

**Code Libraries Found**:
- [Site] has complete CAN library (downloaded: yes/no)
- [Site] has working examples (tested: yes/no)

**Wiring Information**:
- BCM connector: [location]
- CAN wires: [colors/pins]

### Priority Actions

1. [ ] Test door lock message ID 0x??? on vehicle
2. [ ] Integrate code from Chamber Part 21
3. [ ] Compare to Collins Performance feature set
4. [ ] Update our DBC file with new IDs

### Next Steps

[What to do with this information]
```

---

## Integration Workflow

After finding CAN messages:

1. **Document** in `docs/RX8_BCM_RESEARCH.md`
2. **Test** with our sniffer (`examples/BCM_Control_Example/`)
3. **Implement** in `lib/bcm_control/` (if it works)
4. **Share** findings with community (GitHub, RX8Club)

---

## Questions to Answer

As you review sites, try to answer:

- [ ] What CAN IDs does Collins Performance emulate?
- [ ] Does Chamber's code include BCM control?
- [ ] What's the complete AutosportLabs mapping?
- [ ] Are there any security/checksum requirements?
- [ ] Do messages vary by year (2004-2011)?
- [ ] What hardware did they use (Arduino model, CAN module)?

---

## Success Criteria

You've successfully extracted the info when you can:

✅ List at least 5 new CAN IDs we don't have
✅ Have data format for at least one BCM function
✅ Downloaded complete working code from Chamber
✅ Know what Collins Performance product does
✅ Have complete racing telemetry mapping

---

**Estimated Time**: 2-3 hours for thorough review of all 5 sites

**Expected Outcome**: 10-20 new CAN message IDs, possibly including BCM control

**Next Steps After Manual Review**:
1. Share findings here
2. I'll help integrate into our codebase
3. Create implementation plan
4. Test on vehicle

---

**Good luck with the manual extraction!** 🔍

Let me know what you find and I'll help process and integrate it!
