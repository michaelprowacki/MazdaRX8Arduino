# Body Control Module (BCM) Control via CAN Bus

**Goal**: Control RX8 body/convenience functions via CAN bus from Arduino ECU

**Status**: Research & implementation guide
**Difficulty**: Advanced (requires CAN reverse engineering)
**Safety**: Low risk (body functions, not safety-critical)

---

## Overview

The Mazda RX8 uses multiple control modules that communicate over the CAN bus:

```
┌─────────────────────────────────────────────────────┐
│              RX8 CAN Bus Architecture                │
└─────────────────────────────────────────────────────┘

High-Speed CAN (500 kbps):
├── PCM/ECU (Engine Control)          ← We're replacing this
├── TCM (Transmission Control)
├── ABS/DSC Module
├── Instrument Cluster (Gauges)       ← We already control this
├── BCM (Body Control Module)         ← NEW: Control this
├── Immobilizer                       ← We already handle this
├── Power Steering Module
└── HVAC Control Module

Medium-Speed CAN (125 kbps):
├── Audio System
├── Navigation (if equipped)
└── Multi-function Display
```

**What we currently control**:
- ✅ Instrument cluster (RPM, speed, temp, warning lights) via 0x201, 0x420, etc.
- ✅ Immobilizer (key authentication) via 0x041

**What we can add**:
- 🎯 Power windows (auto up/down)
- 🎯 Door locks (remote lock/unlock)
- 🎯 Interior/exterior lighting
- 🎯 Wiper control (already in ECU, can enhance)
- 🎯 HVAC controls (fan speed, temperature)
- 🎯 Horn/alarm
- 🎯 Fuel door release
- 🎯 Trunk release

---

## How BCM Control Works

### CAN Message Structure

Body control messages typically follow this pattern:

```
CAN ID: 0xXXX (specific to function)
Data: 8 bytes
  [0]: Command byte
  [1]: Parameter 1
  [2]: Parameter 2
  [3-7]: Additional data or padding (often 0x00)
```

**Example - Door Lock Control** (hypothetical):
```cpp
CAN ID: 0x410
Data: [0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
      ↑
      0x01 = Lock all doors
      0x02 = Unlock all doors
      0x03 = Lock driver door only
      0x04 = Unlock driver door only
```

### Discovery Process

To find BCM control messages, we need to **reverse engineer** the CAN bus:

1. **Capture baseline** - Record CAN traffic during normal operation
2. **Trigger function** - Press button (e.g., door lock)
3. **Compare traffic** - Find new/changed messages
4. **Isolate message** - Replay to confirm it controls the function
5. **Document** - Add to CAN database

---

## Known RX8 BCM CAN Messages

### Currently Documented (From Our Code)

| CAN ID | Function | Data Format | Status |
|--------|----------|-------------|--------|
| **0x201** | PCM Status (RPM, Speed) | 8 bytes | ✅ Implemented |
| **0x420** | Warning Lights (MIL, Temp) | 7 bytes | ✅ Implemented |
| **0x4B1** | Wheel Speeds | 8 bytes | ✅ Read only |
| **0x047** | Immobilizer Request | 8 bytes | ✅ Receive only |
| **0x041** | Immobilizer Response | 8 bytes | ✅ Implemented |

### To Be Discovered (BCM Functions)

| Function | Suspected CAN ID Range | Priority | Notes |
|----------|----------------------|----------|-------|
| **Door Locks** | 0x400-0x4FF | High | Common convenience feature |
| **Power Windows** | 0x400-0x4FF | Medium | Auto up/down control |
| **Wiper Control** | 0x200-0x2FF | Low | Already have basic control |
| **HVAC Fan** | 0x500-0x5FF | Medium | Fan speed override |
| **Interior Lights** | 0x400-0x4FF | Low | Dome light, map lights |
| **Exterior Lights** | 0x200-0x2FF | Medium | Headlights, turn signals |
| **Horn** | 0x400-0x4FF | Low | Security/alert functions |

**Note**: These are educated guesses based on typical automotive CAN bus layouts. Actual IDs must be discovered through sniffing.

---

## Reverse Engineering Tools

### Hardware Needed

1. **CAN Bus Sniffer**
   - **Budget** ($20): MCP2515 + Arduino (we already have this!)
   - **Mid-range** ($50-100): USB-CAN adapter (CANable, GVRET)
   - **Professional** ($300-1000): Vector CANalyzer, Kvaser

2. **OBD2 Splitter/Tap**
   - Allows simultaneous sniffing and normal vehicle operation
   - Cost: $10-20 on Amazon

### Software Tools

1. **SavvyCAN** (Free, Open Source) ⭐ **RECOMMENDED**
   - Windows/Mac/Linux
   - Real-time CAN sniffing
   - Graphing, filtering, replay
   - DBC file support
   - Download: https://github.com/collin80/SavvyCAN

2. **Kayak** (Free, Java-based)
   - Cross-platform
   - Good for DBC editing
   - Download: https://github.com/dschanoeh/Kayak

3. **Wireshark** (Free) + socketCAN (Linux)
   - Standard network analysis tool
   - Supports CAN bus with socketCAN driver
   - Good for deep packet inspection

4. **CANalyzer** (Commercial, $$$)
   - Industry standard
   - Overkill for hobbyist use

---

## Step-by-Step: Discovering Door Lock Control

Let's walk through discovering the door lock CAN message as an example.

### Step 1: Setup CAN Sniffer (30 min)

**Hardware Setup**:
```
RX8 OBD2 Port
     │
  Splitter
  ┌──┴──┐
  │     │
Vehicle  MCP2515 Arduino
Normal   (Sniffer)
 Use        │
         Laptop
       (SavvyCAN)
```

**Arduino Sketch** (CAN sniffer):
```cpp
// File: can_sniffer.ino
#include <mcp_can.h>

MCP_CAN CAN0(10);  // CS pin 10

void setup() {
  Serial.begin(2000000);  // High baud rate for fast logging

  if(CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("CAN Init OK");
  }

  CAN0.setMode(MCP_NORMAL);
}

void loop() {
  long unsigned int rxId;
  unsigned char len = 0;
  unsigned char rxBuf[8];

  if(CAN0.checkReceive() == CAN_MSGAVAIL) {
    CAN0.readMsgBuf(&rxId, &len, rxBuf);

    // Output in GVRET format (compatible with SavvyCAN)
    Serial.print(millis());
    Serial.print(",");
    Serial.print(rxId, HEX);
    Serial.print(",");

    for(int i = 0; i < len; i++) {
      if(rxBuf[i] < 16) Serial.print("0");
      Serial.print(rxBuf[i], HEX);
      if(i < len-1) Serial.print(" ");
    }

    Serial.println();
  }
}
```

**Upload and test**: Should see constant stream of CAN messages

### Step 2: Capture Baseline (5 min)

1. Start SavvyCAN, connect to Arduino serial port
2. **Record baseline** - 30 seconds of normal operation (no actions)
3. Save as `baseline.csv`

### Step 3: Capture Door Lock Event (5 min)

1. Clear capture buffer in SavvyCAN
2. **Start recording**
3. **Press door lock button** (driver's door switch)
4. **Wait 2 seconds**
5. **Stop recording**
6. Save as `door_lock.csv`

### Step 4: Compare & Find Difference (10 min)

**In SavvyCAN**:
1. Load both files
2. Use "Flow View" to see message timeline
3. Look for messages that appear ONLY in `door_lock.csv`
4. Common patterns:
   - New message appears right when button pressed
   - Existing message changes one byte
   - Burst of messages (BCM acknowledging command)

**Example findings**:
```
Baseline:     No 0x410 messages

Door lock:    Timestamp    ID    Data
              1234.567    0x410  01 00 00 00 00 00 00 00
              1234.587    0x410  00 00 00 00 00 00 00 00
                                 ↑
                          Byte 0 toggled!
```

### Step 5: Replay Message (10 min)

**Test if we found it**:

```cpp
// File: test_door_lock.ino
#include <mcp_can.h>

MCP_CAN CAN0(10);

byte lockDoors[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
byte unlockDoors[8] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void setup() {
  Serial.begin(115200);
  CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ);
  CAN0.setMode(MCP_NORMAL);

  delay(1000);
}

void loop() {
  Serial.println("Sending LOCK command...");
  CAN0.sendMsgBuf(0x410, 0, 8, lockDoors);
  delay(3000);

  Serial.println("Sending UNLOCK command...");
  CAN0.sendMsgBuf(0x410, 0, 8, unlockDoors);
  delay(3000);
}
```

**Upload and test**:
- Doors should lock/unlock every 3 seconds
- If it works → **SUCCESS!** You found the message
- If nothing happens → Try different IDs or byte patterns

### Step 6: Document (5 min)

Add to `docs/rx8_can_database.dbc`:

```
BO_ 1040 DoorLockControl: 8 Vector__XXX
 SG_ LockCommand : 0|8@1+ (1,0) [0|255] "" Vector__XXX
   0 = No action
   1 = Lock all doors
   2 = Unlock all doors
   3 = Lock driver only
   4 = Unlock driver only
```

---

## Discovered BCM Messages (Community Contributions)

**⚠️ IMPORTANT**: These are UNVERIFIED examples for illustration. Always test on YOUR vehicle before relying on them.

### Example 1: Door Locks (Hypothetical)

```cpp
// Hypothetical - needs verification on actual RX8

namespace DoorLocks {
  const uint16_t CAN_ID = 0x410;

  enum Command {
    NO_ACTION = 0x00,
    LOCK_ALL = 0x01,
    UNLOCK_ALL = 0x02,
    LOCK_DRIVER = 0x03,
    UNLOCK_DRIVER = 0x04
  };

  void sendCommand(Command cmd) {
    byte msg[8] = {cmd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    CAN0.sendMsgBuf(CAN_ID, 0, 8, msg);
  }
}
```

### Example 2: Power Windows (Hypothetical)

```cpp
// Hypothetical - needs verification

namespace PowerWindows {
  const uint16_t CAN_ID = 0x420;  // Different from warning lights!

  enum Window {
    DRIVER_FRONT = 0,
    PASSENGER_FRONT = 1,
    DRIVER_REAR = 2,
    PASSENGER_REAR = 3
  };

  enum Action {
    STOP = 0x00,
    UP = 0x01,
    DOWN = 0x02,
    AUTO_UP = 0x03,
    AUTO_DOWN = 0x04
  };

  void controlWindow(Window win, Action act) {
    byte msg[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    msg[0] = (win << 4) | act;  // Window in upper nibble, action in lower
    CAN0.sendMsgBuf(CAN_ID, 0, 8, msg);
  }
}
```

### Example 3: Wiper Control (Enhancement)

We already have basic wiper control in `ECU_Module`, but BCM might accept advanced commands:

```cpp
// Current implementation: Speed-based auto wipers (ECU_Module)
// Possible BCM enhancement: Direct wiper control

namespace WiperControl {
  const uint16_t CAN_ID = 0x???;  // To be discovered

  enum Mode {
    OFF = 0x00,
    INTERMITTENT = 0x01,
    LOW = 0x02,
    HIGH = 0x03,
    MIST = 0x04
  };

  enum Interval {
    FAST = 0x01,    // 1 second
    MEDIUM = 0x02,  // 3 seconds
    SLOW = 0x03     // 5 seconds
  };

  void setMode(Mode mode, Interval interval = MEDIUM) {
    byte msg[8] = {mode, interval, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    CAN0.sendMsgBuf(CAN_ID, 0, 8, msg);
  }
}
```

---

## Integration with Existing ECU Code

### Option 1: Separate BCM Control Module

Create a new module: `firmware/automotive_ecu/src/bcm_control/`

```cpp
// File: bcm_control.h

#ifndef BCM_CONTROL_H
#define BCM_CONTROL_H

#include <stdint.h>

namespace BCMControl {

/**
 * @brief Initialize BCM control
 */
void init();

/**
 * @brief Update BCM functions
 *
 * Call periodically (10 Hz recommended)
 */
void update();

// Door locks
void lockAllDoors();
void unlockAllDoors();
void lockDriverDoor();
void unlockDriverDoor();

// Power windows
void driverWindowUp();
void driverWindowDown();
void passengerWindowUp();
void passengerWindowDown();

// Lighting
void flashHeadlights(uint8_t count);
void honkHorn(uint16_t duration_ms);

// HVAC
void setFanSpeed(uint8_t speed);  // 0-7
void setTemperature(uint8_t temp_c);  // 16-30°C

// Status
bool areDoorsLocked();
bool isDriverWindowUp();

} // namespace BCMControl

#endif // BCM_CONTROL_H
```

### Option 2: Add to SystemManager

Integrate BCM control into existing `system_manager.cpp`:

```cpp
// In system_manager.cpp

namespace SystemManager {

// Add BCM control to operating modes
enum SystemMode {
  MODE_STARTUP,
  MODE_NORMAL,
  MODE_SAFE,
  MODE_DIAGNOSTIC,
  MODE_EMERGENCY,
  MODE_VALET        // NEW: Valet mode with BCM lockout
};

// Valet mode features
void enterValetMode() {
  // Lock advanced features
  MapSwitching::switchToMap(2);  // Valet map (power limited)

  // Lock BCM functions
  BCMControl::lockAllDoors();
  BCMControl::driverWindowUp();
  BCMControl::passengerWindowUp();

  // Disable features
  LaunchControl::setEnabled(false);
  NitrousControl::setEnabled(false);

  Serial.println("[SystemMgr] VALET MODE ACTIVE");
}

} // namespace
```

### Option 3: Remote Control via WiFi

Add remote BCM control to WiFi dashboard:

```cpp
// In wifi_dashboard (ESP32)

void handleRemoteCommand(String cmd) {
  if(cmd == "lock_doors") {
    // Send CAN message via ESP32 -> Arduino Serial/I2C
    sendCANCommand(0x410, {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
  }
  else if(cmd == "unlock_doors") {
    sendCANCommand(0x410, {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
  }
  else if(cmd == "flash_lights") {
    flashHeadlights(3);  // Flash 3 times
  }
}

// Web API endpoint
server.on("/api/bcm/lock", HTTP_POST, []() {
  handleRemoteCommand("lock_doors");
  server.send(200, "application/json", "{\"status\":\"locked\"}");
});
```

---

## Safety Considerations

### What's Safe to Control

✅ **Low Risk**:
- Door locks (can't lock out occupants - manual override exists)
- Power windows (auto-reverse on obstruction)
- Interior lights (cosmetic)
- Wiper control (driver still has manual switch)
- HVAC fan speed (comfort only)

⚠️ **Medium Risk**:
- Horn (could annoy neighbors if stuck on)
- Headlights (don't blind other drivers)
- Trunk release (security concern)

❌ **High Risk - DO NOT CONTROL**:
- Immobilizer (beyond auth handshake) - could brick vehicle
- ABS/DSC active functions - safety critical
- Airbag deployment - illegal & dangerous
- Fuel pump cutoff (beyond normal ECU control) - safety

### Anti-Theft Protection

**Problem**: If we can unlock doors via CAN, so can thieves with CAN injection attack.

**Solution**: Add security layer

```cpp
namespace BCMControl {

bool security_armed = true;
uint32_t last_auth_time = 0;
const uint32_t AUTH_TIMEOUT = 30000;  // 30 seconds

bool authenticate(const char* pin) {
  if(strcmp(pin, "1234") == 0) {  // Replace with secure storage
    last_auth_time = millis();
    security_armed = false;
    return true;
  }
  return false;
}

void lockAllDoors() {
  // No authentication needed to LOCK (safer default)
  sendDoorLockCommand(LOCK_ALL);
}

void unlockAllDoors() {
  // Require recent authentication to UNLOCK
  if(millis() - last_auth_time > AUTH_TIMEOUT) {
    Serial.println("[BCM] Authentication expired!");
    return;
  }

  sendDoorLockCommand(UNLOCK_ALL);

  // Re-arm security after use
  security_armed = true;
}

} // namespace
```

---

## Use Cases

### 1. Remote Start Integration

**Scenario**: Add aftermarket remote start, need to unlock doors automatically

```cpp
void remoteStartSequence() {
  // 1. Authenticate with immobilizer (we already do this)
  // 2. Unlock driver door
  BCMControl::unlockDriverDoor();
  delay(500);
  // 3. Start engine (via starter relay)
  digitalWrite(STARTER_PIN, HIGH);
  delay(2000);
  digitalWrite(STARTER_PIN, LOW);
  // 4. Monitor engine RPM via CAN
  // 5. Lock doors after 30 seconds if no entry detected
}
```

### 2. Automatic Rain-Sensing Windows

**Scenario**: Close windows automatically when it starts raining

```cpp
// Integrate with rain sensor (if equipped) or external sensor

void update() {
  if(rainSensorDetectsRain() && !BCMControl::isDriverWindowUp()) {
    Serial.println("[BCM] Rain detected, closing windows");
    BCMControl::driverWindowUp();
    BCMControl::passengerWindowUp();
  }
}
```

### 3. Valet Mode Security

**Scenario**: Hand keys to valet, limit access to trunk/glovebox

```cpp
void enterValetMode() {
  // Lock all features
  BCMControl::lockAllDoors();

  // Prevent trunk release
  valet_mode_active = true;

  // Limit engine power (already have this via map switching)
  MapSwitching::switchToMap(2);  // Valet map

  Serial.println("[BCM] Valet mode: Trunk locked, power limited");
}

void trunkReleaseRequest() {
  if(valet_mode_active) {
    Serial.println("[BCM] Trunk release blocked - valet mode");
    return;  // Deny
  }

  // Normal trunk release
  sendTrunkReleaseCommand();
}
```

### 4. Security Alarm Integration

**Scenario**: Flash lights and honk horn on alarm trigger

```cpp
void alarmTriggered() {
  for(int i = 0; i < 10; i++) {
    BCMControl::flashHeadlights(1);
    BCMControl::honkHorn(200);  // 200ms beep
    delay(500);
  }

  // Log event
  DataLogger::logEvent("ALARM_TRIGGERED");

  // Send notification via WiFi
  WiFiDashboard::sendAlert("Vehicle alarm triggered!");
}
```

---

## Testing Procedure

### Bench Testing (No Vehicle)

**CAN Bus Simulator**:
1. Use two Arduinos
2. Arduino 1: Sends test BCM messages
3. Arduino 2: Our ECU, tries to control BCM
4. Verify message format before vehicle testing

### Vehicle Testing (Incremental)

**Phase 1**: Passive monitoring only
- Read BCM messages
- Log to SD card
- No transmission
- **Risk**: None

**Phase 2**: Send known-safe messages
- Start with door locks (easy to verify)
- Test in controlled environment
- Monitor for unexpected behavior
- **Risk**: Low

**Phase 3**: Advanced features
- Power windows, lighting
- Test one feature at a time
- Have manual override ready
- **Risk**: Medium

---

## Resources

### Community Databases

1. **RX8Club Forums**
   - https://www.rx8club.com/
   - Search: "CAN bus reverse engineering"
   - Many members have documented CAN messages

2. **GitHub Repositories**
   - Search: "mazda rx8 can bus"
   - Look for DBC files or message logs

3. **Our Project**
   - `docs/rx8_can_database.dbc` - Add discoveries here
   - `docs/CAN_PID_Reference.md` - Document new messages

### Recommended Reading

- **"Car Hacker's Handbook"** by Craig Smith (O'Reilly)
- **"CAN Bus Hacking"** (various online tutorials)
- **ISO 11898** (CAN bus standard - technical spec)

---

## Contributing Your Discoveries

**If you discover BCM CAN messages, please contribute!**

1. Document the message:
   - CAN ID (hex)
   - Data bytes (what each byte does)
   - Testing notes (what vehicle/year)
   - Safety considerations

2. Test thoroughly:
   - Verify on multiple RX8s if possible
   - Check for side effects
   - Document failure modes

3. Submit:
   - GitHub pull request to this repo
   - Or post on RX8Club with "CAN Discovery" tag
   - Include DBC file update

**Format**:
```
CAN ID: 0xXXX
Function: Door locks
Data Format:
  [0]: Command (0x01=lock, 0x02=unlock)
  [1-7]: 0x00 (unused)
Tested On: 2004 RX8 MT
Safety: Low risk, doors still manually operable
Discovered By: [Your name/handle]
Date: 2025-11-16
```

---

## Implementation Roadmap

### Phase 1: Discovery (Community Effort)
- [ ] Door locks
- [ ] Power windows
- [ ] Trunk release
- [ ] HVAC fan control
- [ ] Lighting control

### Phase 2: Library Creation
- [ ] Create `BCMControl` library
- [ ] Add to `lib/` directory
- [ ] Document API

### Phase 3: Integration
- [ ] Add to SystemManager
- [ ] WiFi remote control
- [ ] Valet mode implementation

### Phase 4: Testing
- [ ] Bench testing with CAN simulator
- [ ] Vehicle testing (controlled environment)
- [ ] Long-term reliability testing

---

## FAQ

**Q: Can this brick my car?**
A: Very unlikely. BCM functions are non-critical. Worst case: disconnect battery to reset. Don't mess with immobilizer beyond auth handshake.

**Q: Will this void my warranty?**
A: RX8s are 14-20 years old - no warranty left! But yes, modifications can affect resale value.

**Q: Can I control my car from my phone?**
A: Yes! Via WiFi dashboard → Arduino → CAN bus. Add authentication for security.

**Q: What if I send the wrong message?**
A: Most modules ignore invalid messages. Test incrementally. Have kill switch ready (disconnect Arduino).

**Q: Is this legal?**
A: Modifying your own car: Legal (most jurisdictions)
Interfering with safety systems: Illegal
Hacking someone else's car: Very illegal

---

## Conclusion

BCM control via CAN bus opens up many possibilities:
- Remote start integration
- Custom alarm systems
- Convenience features (auto-closing windows, etc.)
- Valet mode security
- Integration with home automation

**Current Status**: Research phase - messages need to be discovered through community effort.

**How You Can Help**:
1. Set up CAN sniffer
2. Discover BCM messages for your vehicle
3. Test and document
4. Submit findings to this repo

Together we can build a complete BCM control library for the RX8!

---

**Last Updated**: 2025-11-16
**Status**: Research guide / Community contribution needed
**Safety Level**: Low-Medium (body functions only)

**See Also**:
- `docs/CAN_PID_Reference.md` - Known CAN messages
- `docs/rx8_can_database.dbc` - DBC file for CAN messages
- `core/ECU_Module/RX8_CANBUS.ino` - Current CAN implementation
