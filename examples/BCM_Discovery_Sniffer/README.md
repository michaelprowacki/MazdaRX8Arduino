# BCM Discovery Sniffer

**Purpose**: Discover unknown CAN bus messages for BCM (Body Control Module) functions in the Mazda RX8.

**Target Functions**:
- Door locks (lock/unlock)
- Power windows (up/down)
- Interior lights (on/off)
- Trunk release
- Other body control features

---

## Quick Start

### 1. Hardware Setup

**Required Hardware**:
- Arduino Leonardo (or compatible)
- MCP2515 CAN bus module
- OBD2 connector or direct CAN bus connection
- USB cable for Arduino programming

**Wiring**:
```
MCP2515 → Arduino Leonardo
-----------------------------
VCC     → 5V
GND     → GND
CS      → Digital Pin 17
INT     → Digital Pin 2
SI      → MOSI (ICSP header)
SO      → MISO (ICSP header)
SCK     → SCK (ICSP header)

CAN Bus → MCP2515
-----------------------------
CANH    → CANH (OBD2 pin 6)
CANL    → CANL (OBD2 pin 14)
```

**OBD2 Pinout** (looking at connector, key slot on top):
```
     [1]  [2]  [3]  [4]  [5]  [6]  [7]  [8]
  [9]  [10] [11] [12] [13] [14] [15] [16]
```
- Pin 6: CAN High (CANH) - Usually white/orange wire
- Pin 14: CAN Low (CANL) - Usually green/black wire
- Pin 16: +12V (battery power)
- Pin 4: Ground

---

### 2. Upload Sketch

**Using Arduino IDE**:
1. Open `BCM_Discovery_Sniffer.ino`
2. Select **Tools → Board → Arduino Leonardo**
3. Select **Tools → Port** (your Arduino's COM port)
4. Click **Upload** (→ button)

**Using arduino-cli**:
```bash
arduino-cli compile --fqbn arduino:avr:leonardo BCM_Discovery_Sniffer.ino
arduino-cli upload --fqbn arduino:avr:leonardo --port /dev/ttyACM0 BCM_Discovery_Sniffer.ino
```

---

### 3. Connect to Vehicle

1. **Locate OBD2 port** (usually under dashboard, driver's side)
2. **Plug in CAN sniffer** (engine can be off or on)
3. **Turn ignition to ON** (or start engine)
4. **Open Serial Monitor** (115200 baud)
5. **Start discovery process** (see below)

---

## Discovery Workflow

### Step 1: Baseline Capture (10 seconds)

**Goal**: Record normal CAN traffic when no buttons are pressed

**Procedure**:
1. Vehicle in normal state:
   - Doors closed (not locked)
   - Windows up
   - Lights off
   - Engine idling or ignition ON
2. Type **'b'** in Serial Monitor
3. Wait 10 seconds while system captures baseline

**Expected Output**:
```
========== BASELINE CAPTURE ==========
Recording normal CAN traffic for 10 seconds...
Keep vehicle in normal state (doors closed, windows up, lights off)

========== BASELINE COMPLETE ==========
Captured 15 unique CAN IDs, 1247 total messages

Now you can:
  1. Activate a function (lock doors, press trunk button, etc.)
  2. Type 'f' to filter and show only NEW or CHANGED messages
```

**Baseline will show known messages**:
- `0x201` (513) - PCM status (RPM, speed)
- `0x420` (1056) - MIL, temperature, odometer
- `0x4B0/0x4B1` (1200/1201) - Wheel speeds
- `0x620/0x630` (1568/1584) - ABS data
- Others...

---

### Step 2: Trigger Function & Filter

**Goal**: Identify CAN message that changes when you activate a function

**Example: Discovering Door Lock CAN ID**

1. **Complete baseline capture** (Step 1)
2. **Lock doors with key fob** (or door lock button)
3. **Immediately type 'f'** (filter mode)
4. **Observe new/changed messages**

**Expected Output** (example):
```
========== FILTER MODE ==========
Showing only NEW or CHANGED messages

ID (Hex) | ID (Dec) | Len | Data                   | Count  | Interval | Status
---------|----------|-----|------------------------|--------|----------|--------
0x3B5    |  949     | 8   | 01 00 00 00 00 00 00 00|      1 |     N/A  | [NEW]
```

**Analysis**:
- **0x3B5** (949 decimal) appeared after locking doors
- **Byte 0 = 0x01** (might mean "locked")
- This is likely the door lock control message!

---

### Step 3: Test Message Replay

**Goal**: Verify the discovered message by sending it back to the CAN bus

**Procedure**:
1. **Unlock doors** (return to baseline state)
2. Type **'r'** (replay mode)
3. Enter discovered message:
   ```
   3B5,8,01,00,00,00,00,00,00,00
   ```
   Format: `ID,Length,Byte0,Byte1,Byte2,...,Byte7`

**Expected Result**: Doors lock!

**If doors lock**: ✅ You discovered the door lock CAN message!
**If nothing happens**: ❌ Try different byte values or wait for more messages

---

### Step 4: Document Discovery

Record your findings:

**Example Discovery Log**:
```
Function: Door Lock
CAN ID: 0x3B5 (949 decimal)
Length: 8 bytes
Lock:   [01 00 00 00 00 00 00 00]
Unlock: [00 00 00 00 00 00 00 00]

Test Results:
- ✅ Replay test successful (10/10 times)
- ✅ No side effects observed
- ✅ Works with ignition ON/OFF
```

---

## Serial Commands Reference

| Command | Function | Description |
|---------|----------|-------------|
| **'b'** | Baseline | Capture 10 sec of normal CAN traffic |
| **'s'** | Sniff | Show ALL messages (continuous) |
| **'f'** | Filter | Show only NEW/CHANGED messages |
| **'d'** | Dump | Display current message buffer |
| **'c'** | Clear | Clear buffer and stop current mode |
| **'r'** | Replay | Send test message to CAN bus |
| **'h'** | Help | Show command help |

---

## Example Use Cases

### Use Case 1: Door Lock Discovery

**Goal**: Find CAN message for door locks

**Steps**:
1. Baseline: Doors unlocked (`'b'`)
2. Action: Lock doors with key fob
3. Filter: Look for new messages (`'f'`)
4. Expected: CAN ID 0x3XX or 0x4XX with byte change

**Sample Output**:
```
0x3B5 | 949 | 8 | 01 00 00 00 00 00 00 00 | 1 | N/A | [NEW]
```

**Replay Test**:
```
r
3B5,8,01,00,00,00,00,00,00,00
```

---

### Use Case 2: Trunk Release Discovery

**Goal**: Find CAN message for trunk release

**Steps**:
1. Baseline: Trunk closed (`'b'`)
2. Action: Press trunk release button (dash or key fob)
3. Filter: Look for **momentary** message (`'f'`)
4. Expected: Pulse message (appears once, then disappears)

**Sample Output**:
```
0x450 | 1104 | 4 | 01 00 00 00 | 1 | N/A | [NEW]
```

**Analysis**:
- Trunk message is likely **momentary** (not continuous)
- May need to press button multiple times to capture
- Byte 0 = 0x01 triggers trunk release

---

### Use Case 3: Interior Light Discovery

**Goal**: Find CAN message for dome light control

**Steps**:
1. Baseline: Lights off (`'b'`)
2. Action: Press dome light button
3. Filter: Look for changed message (`'f'`)
4. Expected: Existing message with byte change (not new ID)

**Sample Output**:
```
0x420 | 1056 | 7 | 91 00 00 00 01 40 02 | 523 | 109 ms | [CHANGED]
```

**Analysis**:
- Interior lights might be controlled by **existing** CAN ID
- Compare byte-by-byte to find which bit changed
- Might be part of 0x420 (warning lights message)

---

## Troubleshooting

### Problem: No CAN Messages Received

**Symptoms**:
- Serial monitor shows "0 unique messages"
- No activity even after 10 seconds

**Possible Causes**:
1. **CAN bus not connected**
   - Check OBD2 connection (pins 6 & 14)
   - Verify MCP2515 wiring

2. **Ignition OFF**
   - Turn ignition to ON position
   - Or start engine

3. **Wrong CAN bus speed**
   - RX8 uses 500 kbps (High Speed CAN)
   - Code default is 500 kbps (should work)

**Debug Steps**:
```cpp
// In setup(), after CAN0.begin():
Serial.println(F("Testing CAN receive..."));
delay(5000);  // Wait 5 seconds
if (totalMessages == 0) {
  Serial.println(F("ERROR: No messages received!"));
}
```

---

### Problem: Too Many Messages (Buffer Overflow)

**Symptoms**:
- Serial monitor floods with messages
- "Buffer full" warnings
- Sniffer stops responding

**Solution**:
- Increase `MAX_MESSAGES` in code (line 43)
- Or filter out known messages:

```cpp
// In processingCANMessage(), after readMsgBufID():
// Ignore known PCM/ABS messages to reduce clutter
if (id == 0x201 || id == 0x420 || id == 0x4B0 || id == 0x4B1 ||
    id == 0x620 || id == 0x630) {
  return;  // Skip known messages
}
```

---

### Problem: Replay Doesn't Work

**Symptoms**:
- Send message shows "SUCCESS" but nothing happens
- Doors don't lock, trunk doesn't open, etc.

**Possible Causes**:
1. **Wrong message format**
   - Check ID is correct (hex)
   - Verify all 8 bytes

2. **Checksum required**
   - Some messages need calculated checksum byte
   - Try incrementing last byte (0x00 → 0x01 → 0x02...)

3. **Sequence required**
   - BCM might need multiple messages
   - Or specific order (unlock before lock)

4. **Timing sensitive**
   - BCM might ignore repeated messages too fast
   - Try adding delay between sends

**Debug**:
```
// Try different byte 0 values:
3B5,8,00,00,00,00,00,00,00,00
3B5,8,01,00,00,00,00,00,00,00
3B5,8,02,00,00,00,00,00,00,00
...
```

---

## Safety & Legal Considerations

### ⚠️ Safety Warnings

1. **Do NOT test power windows while vehicle is moving**
   - Risk of injury if pinch detection fails
   - Test in parked, stationary vehicle only

2. **Do NOT disable safety features**
   - Keep ABS/DSC operational
   - Don't interfere with airbag systems (not on HS-CAN but still)

3. **Always have manual override**
   - Keep mechanical key accessible
   - Test unlock before testing lock
   - Verify manual door handles work

4. **Do NOT test while driving**
   - Potential distraction
   - CAN bus interference could affect engine control

---

### 🔒 Legal Considerations

1. **Only test on YOUR vehicle**
   - Unauthorized access to other vehicles is illegal
   - Vehicle tampering laws vary by jurisdiction

2. **Document all tests**
   - Keep log of messages sent
   - Note any failures or issues
   - Important for insurance/warranty claims

3. **Disclose modifications when selling**
   - Inform buyer of CAN bus experiments
   - Provide documentation of changes

---

## Expected Discovery Results

Based on research, here's what you might find:

| Function | Likelihood | Expected CAN ID Range | Notes |
|----------|------------|----------------------|-------|
| Door Locks | **90%** | 0x3XX - 0x4XX | High priority, likely discoverable |
| Interior Lights | **80%** | 0x420 (existing) | May be part of existing message |
| Trunk Release | **70%** | 0x3XX - 0x4XX | Momentary pulse message |
| Power Windows | **60%** | 0x3XX - 0x4XX | May have safety interlock |
| Horn | **20%** | Unknown | Security feature, may be blocked |

**Note**: These are estimates based on similar Mazda vehicles. Your results may vary!

---

## Sharing Your Discoveries

If you discover new BCM CAN messages, please share with the community:

### How to Share

1. **Document your findings**:
   ```
   Function: [Door Lock / Window / etc.]
   CAN ID: 0xXXX (decimal)
   Length: X bytes
   Message Format: [HEX bytes]
   Test Results: [Success rate, side effects]
   Vehicle: [Year, Model, Trim]
   ```

2. **Post to GitHub**:
   - Open issue: https://github.com/michaelprowacki/MazdaRX8Arduino/issues
   - Title: "[BCM Discovery] Door Lock CAN ID Found"
   - Include documentation above

3. **Post to RX8Club.com**:
   - Forum: "Series I Tech Garage"
   - Thread: "RX8 CAN Bus Research"
   - Credit: Mention this project

4. **Update documentation**:
   - Pull request to update `docs/RX8_BCM_CANBUS_RESEARCH.md`
   - Add to `Documentation/CAN_PID_Reference.md`

---

## Technical Details

### Message Database Structure

```cpp
struct CANMessage {
  unsigned long id;        // CAN message ID (0x000 - 0x7FF)
  byte len;                // Message length (0-8 bytes)
  byte data[8];            // Message data bytes
  unsigned long timestamp; // Last seen (milliseconds)
  unsigned long count;     // Times seen
  unsigned long interval;  // Avg time between messages (ms)
  boolean isNew;           // Not in baseline
  boolean hasChanged;      // Data changed since baseline
};
```

### Memory Usage

- **Message Buffer**: `MAX_MESSAGES × sizeof(CANMessage)` = 100 × 32 bytes = 3.2 KB
- **Baseline Buffer**: Same (3.2 KB)
- **Total RAM**: ~6.5 KB (Leonardo has 2.5 KB, **may need reduction!**)

**Optimization** (if running out of memory):
```cpp
// Reduce buffer size:
#define MAX_MESSAGES 50  // Instead of 100
```

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-11-17 | Initial release |

---

## Credits

- **Methodology**: docs/RX8_BCM_CANBUS_RESEARCH.md
- **CAN Protocol**: Documentation/CAN_PID_Reference.md
- **Inspiration**: Chamber of Understanding blog (David Blackhurst)
- **Community**: RX8Club.com, GitHub contributors

---

## License

MIT License (same as main repository)

---

**Happy Discovering! 🔍🚗**

If you find BCM CAN messages, you'll be contributing to the RX8 community's knowledge base!
