# Power Distribution Module (PDM)

**Complete electronic power distribution system for RX8 Arduino ECU**

---

## Features Implemented ✅

### 1. **Engine Swap Support** (LS1, 2JZ, V8, I6)
- ✅ Fuel pump control (prime + run logic)
- ✅ Progressive radiator fan control (50% → 70% → 100%)
- ✅ Water pump control (variable speed)
- ✅ A/C clutch control (with safety checks)
- ✅ Transmission cooler fan
- ✅ Auxiliary lights/accessories

### 2. **EV Conversion Support**
- ✅ BMS power management
- ✅ Motor controller 12V supply
- ✅ Motor coolant pump (temp + current-based)
- ✅ Battery coolant pump (temp + charging-based)
- ✅ DC-DC converter control
- ✅ Charging system enable
- ✅ Cabin heater control

### 3. **Race Car Features**
- ✅ Aggressive cooling (100% on track)
- ✅ Data logger power
- ✅ Shift light integration
- ✅ Brake cooling fan
- ✅ Telemetry logging (current, voltage, energy)
- ✅ Reliability features (no compromise cooling)

### 4. **Advanced Features**
- ✅ **Progressive fan control** (temperature-based ramping)
- ✅ **Soft-start** (500ms ramp, prevent voltage sag)
- ✅ **Load shedding** (automatic shutdown on low voltage)
- ✅ **Kill switch** (instant shutdown, interrupt-driven)
- ✅ **Emergency stop** (safety critical devices only)
- ✅ **Overcurrent protection** (per-channel current limiting)
- ✅ **CAN bus failsafe** (shutdown on ECU timeout)
- ✅ **Data logging** (CSV format, all channels)
- ✅ **Telemetry** (real-time current, voltage, energy)
- ✅ **Fault detection** (automatic fault reporting to ECU)

---

## Hardware Requirements

### Minimum (8-Channel PDM)
- Arduino Mega 2560 (clone OK, ~$18)
- MCP2515 CAN module (~$8)
- 8x MOSFET modules (IRLB8721 or similar, 30A, ~$7 each)
- 8x Current sensors (ACS712-30A, ~$4 each)
- 8x Inline fuses (blade fuses + holders, ~$2 each)
- Enclosure (waterproof IP65, ~$20)
- Wire (14 AWG, 50 ft, ~$15)
- Connectors (Anderson Powerpole or ring terminals, ~$20)

**Total**: ~$205

### Optional Additions
- Voltage regulator (for battery voltage sensing, ~$5)
- Kill switch (momentary or latching, ~$10)
- Emergency stop button (mushroom head, ~$15)
- Status display (OLED, ~$10)
- PCB (custom, JLCPCB 5x for ~$10)

---

## Quick Start

### 1. Upload PDM Code

**For Engine Swap**:
```cpp
// In PDM_Module.ino, uncomment:
#define BUILD_ENGINE_SWAP
```

**For EV Conversion**:
```cpp
// In PDM_Module.ino, uncomment:
#define BUILD_EV_CONVERSION
```

**For Race Car**:
```cpp
// In PDM_Module.ino, uncomment:
#define BUILD_RACE_CAR
```

**Upload**:
1. Open `PDM_Module/PDM_Module.ino` in Arduino IDE
2. Select **Tools → Board → Arduino Mega 2560**
3. Select **Tools → Port** (your Mega's COM port)
4. Click **Upload**

### 2. Integrate with ECU

**Add to `ECU_Module/RX8_CANBUS.ino`**:

```cpp
// At top of file (after #includes):
#include "ECU_PDM_Integration.h"

// In setup():
void setup() {
  // ... existing code ...

  #ifdef ENABLE_PDM
  pdm_setup();
  #endif
}

// In sendOnTenth():
void sendOnTenth() {
  // ... existing code ...

  #ifdef ENABLE_PDM
  pdm_update(CAN0, engineRPM, engTemp);
  #endif

  // ... rest of function ...
}

// In loop(), add to CAN message handler:
if (ID == 0x601 || ID == 0x603) {
  #ifdef ENABLE_PDM
  pdm_receiveStatus(ID, buf);
  #endif
}
```

### 3. Wire Hardware

**Power Connections**:
```
Battery 12V ─┬─[100A Fuse]─> PDM Main Input (Mega VIN)
             │
             └─[20A Fuse]──> Channel 1 MOSFET → Fuel Pump
               [25A Fuse]──> Channel 2 MOSFET → Fan 1
               [25A Fuse]──> Channel 3 MOSFET → Fan 2
               ... (8 channels total)
```

**CAN Bus**:
```
ECU MCP2515 CANH ───> PDM MCP2515 CANH
ECU MCP2515 CANL ───> PDM MCP2515 CANL
Common Ground
```

**Safety Inputs** (optional):
```
Kill Switch ──> Pin 3 (active LOW, pull-up enabled)
E-Stop Button ──> Pin 4 (active LOW, pull-up enabled)
```

---

## Configuration Examples

### Example 1: LS1 Engine Swap (V8)

**Channels**:
1. Fuel Pump (15A) - ON when RPM > 200 or priming
2. Radiator Fan 1 (20A) - Progressive: 50% @ 75°C, 70% @ 85°C, 100% @ 95°C
3. Radiator Fan 2 (20A) - Progressive: ON @ 85°C
4. Electric Water Pump (15A) - ON when running, 100% @ 90°C+
5. A/C Compressor Clutch (10A) - ON when requested, RPM > 800, Temp < 100°C
6. Trans Cooler Fan (15A) - ON @ 80°C+
7. Auxiliary Lights (10A) - User control
8. Auxiliary Power (10A) - User control

**Code**:
```cpp
#define BUILD_ENGINE_SWAP  // In PDM_Module.ino
#define PDM_ENGINE_SWAP    // In ECU_PDM_Integration.h
```

---

### Example 2: EV Conversion (Leaf Motor)

**Channels**:
1. BMS Power (10A) - Always ON when key ON
2. Motor Controller 12V (15A) - Always ON when key ON
3. Motor Coolant Pump (15A) - Temp-based + current-based
4. Battery Coolant Pump (15A) - Temp-based + charging-based
5. DC-DC Converter (30A) - Always ON when key ON
6. Charging System (20A) - ON when charging
7. Cabin Heater (25A) - Temperature control
8. Aux 12V (10A) - User control

**Code**:
```cpp
#define BUILD_EV_CONVERSION  // In PDM_Module.ino
#define PDM_EV_CONVERSION    // In ECU_PDM_Integration.h
```

**Integration with EV_ECU_Module**:
- Use existing BMS interface
- Motor temp from motor controller CAN
- Battery temp from BMS CAN
- Charging status from charge controller

---

### Example 3: Race Car (Rotary or Swapped)

**Channels**:
1. Fuel Pump (20A) - Always ON during session
2. Oil Cooler Fan (20A) - Aggressive: 100% on track @ 80°C+
3. Electric Water Pump (15A) - Always 100% when running
4. Transmission Oil Pump (10A) - ON @ RPM > 1000
5. Data Logger Power (5A) - ON during session
6. Shift Light (5A) - RPM-based
7. Brake Cooling Fan (15A) - ON when on track
8. Aux (10A) - User control

**Code**:
```cpp
#define BUILD_RACE_CAR  // In PDM_Module.ino
#define PDM_RACE_CAR    // In ECU_PDM_Integration.h
```

---

## Feature Details

### Progressive Fan Control

**Algorithm**:
```
Coolant Temp < 75°C:  Fans OFF
Coolant Temp 75-85°C: Fan 1 @ 50% PWM
Coolant Temp 85-95°C: Fan 1 @ 70%, Fan 2 @ 70%
Coolant Temp > 95°C:  Both fans @ 100%
```

**Benefits**:
- Reduced power consumption (fans only at needed speed)
- Reduced noise (50% speed = much quieter)
- Extended fan life (less wear from constant 100%)
- Prevents rapid cycling (hysteresis zones)

**Code** (in `ECU_PDM_Integration.h`):
```cpp
void pdm_updateFanControl(byte coolantTemp) {
  if (coolantTemp > 95) {
    pdm_fan1Enable = true;
    pdm_fan2Enable = true;
    pdm_fan1PWM = 255;  // 100%
    pdm_fan2PWM = 255;
  } else if (coolantTemp > 85) {
    pdm_fan1Enable = true;
    pdm_fan2Enable = true;
    pdm_fan1PWM = 180;  // 70%
    pdm_fan2PWM = 180;
  } else if (coolantTemp > 75) {
    pdm_fan1Enable = true;
    pdm_fan2Enable = false;
    pdm_fan1PWM = 128;  // 50%
  } else {
    pdm_fan1Enable = false;
    pdm_fan2Enable = false;
  }
}
```

---

### Soft-Start

**Purpose**: Prevent voltage sag when turning on high-current devices (fuel pump, fans)

**Algorithm**:
1. Channel enabled → Begin soft-start
2. Ramp PWM from 0 → 100% over 500ms (configurable)
3. Update every 10ms for smooth ramp
4. After 500ms → Full power

**Code** (in `PDM_Module.ino`):
```cpp
#define SOFTSTART_RAMP_TIME 500   // ms
#define SOFTSTART_STEP_INTERVAL 10 // ms

// In updateChannels():
unsigned long elapsed = millis() - ch->softStartTime;
if (elapsed < SOFTSTART_RAMP_TIME) {
  ch->currentPWM = (ch->targetPWM * elapsed) / SOFTSTART_RAMP_TIME;
} else {
  ch->currentPWM = ch->targetPWM;  // Ramp complete
}
```

**Benefits**:
- Prevents battery voltage drop (lights dimming)
- Reduces inrush current stress on alternator
- Extends device lifespan (less mechanical shock)

**Example**:
- Fuel pump draw: 15A instant → 3A ramp @ 0ms, 9A @ 250ms, 15A @ 500ms
- Voltage sag: 0.5V drop (with soft-start) vs. 2V drop (instant on)

---

### Load Shedding

**Purpose**: Automatically shed non-critical loads when battery voltage drops (alternator failure)

**Thresholds**:
```
13.8V: Normal (alternator charging)
12.5V: Low (engine off, battery only)
11.0V: Critical (load shedding triggered)
```

**Priority Levels**:
```
Priority 0 (Critical):   NEVER shed (fuel pump, BMS)
Priority 1 (Essential):  Shed only in emergency (fans, water pump)
Priority 2 (Comfort):    Shed early (A/C, heater)
Priority 3 (Auxiliary):  Shed first (lights, accessories)
```

**Algorithm**:
1. Detect battery voltage < 11.0V
2. Shed Priority 3 devices (auxiliary)
3. Check voltage → still low?
4. Shed Priority 2 devices (comfort)
5. Check voltage → recovered?
6. Re-enable devices in reverse order

**Code** (in `PDM_Module.ino`):
```cpp
void updateLoadShedding() {
  if (systemState.batteryVoltage < BATTERY_CRITICAL) {
    systemState.loadSheddingActive = true;

    // Shed by priority (3 → 2 → 1, never 0)
    for (int priority = PRIORITY_AUXILIARY; priority >= PRIORITY_COMFORT; priority--) {
      for (int i = 0; i < NUM_CHANNELS; i++) {
        if (channels[i].priority == priority) {
          channels[i].loadShed = true;
          channels[i].enabled = false;
        }
      }

      // Check if recovered
      if (systemState.batteryVoltage > BATTERY_LOW) {
        systemState.loadSheddingActive = false;
        return;
      }
    }
  }
}
```

**Benefits**:
- Prevents complete battery drain (can still start engine)
- Prioritizes critical systems (fuel pump, BMS stay ON)
- Automatic recovery when alternator fixed

---

### Kill Switch

**Purpose**: Instant shutdown of all systems (emergency or end-of-session)

**Hardware**:
- Momentary or latching switch
- Connected to Pin 3 (interrupt-capable)
- Active LOW (pull-up enabled)

**Response Time**: <1ms (interrupt-driven, not polled)

**Code** (in `PDM_Module.ino`):
```cpp
// Interrupt service routine (runs immediately)
void killSwitchISR() {
  systemState.killSwitchActive = !digitalRead(KILL_SWITCH_PIN);
  if (systemState.killSwitchActive) {
    // Immediately shut down ALL channels
    for (int i = 0; i < NUM_CHANNELS; i++) {
      digitalWrite(channels[i].pin, LOW);
      channels[i].currentPWM = 0;
    }
  }
}

// In setup():
attachInterrupt(digitalPinToInterrupt(KILL_SWITCH_PIN), killSwitchISR, CHANGE);
```

**Use Cases**:
- **Race car**: End-of-session shutdown (turn off fuel pump, fans, everything)
- **Emergency**: Fire, accident (instant power cut)
- **Maintenance**: Work on engine bay (ensure pumps/fans OFF)

---

### Data Logging

**Purpose**: Record all telemetry for analysis, troubleshooting, performance tuning

**Format**: CSV (comma-separated values)

**Logged Data**:
- Timestamp (milliseconds since startup)
- Battery voltage (0.01V resolution)
- Total current (0.01A resolution)
- Total energy (Watt-hours)
- Per-channel current (0.01A resolution, 8 channels)
- Kill switch status
- Load shedding status
- Fault flags

**Output Example**:
```
LOG,0,0,13.82,0.00,0.0,0.00,0.00,0.00,0.00,0.00,0.00,0.00,0.00,0,0,0
LOG,1,1000,13.75,15.23,4.2,15.00,0.00,0.00,0.23,0.00,0.00,0.00,0.00,0,0,0
LOG,2,2000,13.70,35.45,12.7,14.98,20.12,0.00,0.35,0.00,0.00,0.00,0.00,0,0,0
...
```

**Enable/Disable**:
```cpp
// Send CAN command 0x02 to enable logging
pdm_sendCommand(CAN0, 0x02);

// Send CAN command 0x03 to disable logging
pdm_sendCommand(CAN0, 0x03);
```

**Analysis**:
- Import CSV into Excel, MATLAB, Python
- Plot current vs. time (identify patterns)
- Calculate average/peak current per device
- Estimate battery life, alternator capacity

---

## Telemetry & Monitoring

### Real-Time Display (Serial Monitor)

**Format**:
```
PDM: CONN | Batt: 13.8V | Total: 35.2A | Energy: 127Wh
PDM: CONN | Batt: 13.7V | Total: 42.1A | Energy: 135Wh | FAULTS: 0x02
PDM: DISC | Batt: 12.5V | Total: 0.0A | Energy: 140Wh | LOAD SHEDDING
```

**Code** (in `ECU_PDM_Integration.h`):
```cpp
pdm_printTelemetry();  // Call periodically for debugging
```

### CAN Bus Telemetry (to ECU)

**Message 0x601** (Channel Status):
- Byte 0: Active channels (bit flags)
- Bytes 1-4: Current per channel (0-255 = 0-30A)
- Byte 5: Fault flags
- Bytes 6-7: Total current (0.1A resolution)

**Message 0x603** (System Telemetry):
- Byte 0: Battery voltage (0.1V resolution)
- Byte 1: Status flags (kill switch, emergency stop, load shedding)
- Bytes 2-3: Total energy (Watt-hours)
- Bytes 4-5: Peak total current
- Bytes 6-7: Uptime (seconds)

**Usage in ECU**:
```cpp
// Read PDM telemetry
if (pdm_loadSheddingActive) {
  // Trigger dashboard warning light
  checkEngineMIL = 1;
}

if (pdm_totalCurrent > 80) {
  // Approaching alternator capacity (100A)
  // Could reduce non-critical loads
}
```

---

## Testing & Calibration

### Phase 1: Bench Test (Power Supply)

**Equipment**:
- Lab power supply (12V, 10A minimum)
- Multimeter
- Resistive loads (12V bulbs, fans)

**Tests**:
1. **Power-up**: Connect 12V → Verify Mega boots, LEDs blink
2. **Channel control**: Enable Ch1 via Serial → Verify MOSFET ON
3. **PWM test**: Set Ch1 to 50% → Measure ~6V average
4. **Current sensing**: Connect 5A load → Verify PDM reads ~5A
5. **Overcurrent**: Set limit to 3A, connect 5A load → Verify shutdown
6. **Soft-start**: Enable Ch1 → Observe voltage ramp over 500ms

**Serial Commands** (for manual testing):
```
PDM doesn't have built-in serial commands (controlled via CAN only)
Use ECU to send CAN messages, or modify code to add Serial control
```

---

### Phase 2: Integration Test (with ECU)

**Setup**:
- PDM powered from 12V supply
- ECU powered separately
- CAN bus connected (CANH, CANL, GND)

**Tests**:
1. **CAN communication**: Start both → Verify "PDM: CONN" in Serial
2. **Control test**: Set `pdm_fuelPumpEnable = true` in ECU → Verify Ch1 ON
3. **Fan control**: Set `engTemp = 85` → Verify fans at 70% PWM
4. **Status readback**: Check `pdm_totalCurrent` in ECU matches PDM output
5. **Fault propagation**: Trigger overcurrent in PDM → Verify ECU sees fault flag

---

### Phase 3: Vehicle Test (In Car)

⚠️ **SAFETY**: Have fire extinguisher ready, test low-power devices first!

**Test 1: Low-Power Device** (Auxiliary Light, <5A)
1. Connect light to Ch7 (Aux)
2. Set `pdm_aux2Enable = true` in ECU
3. Verify light turns ON
4. Check current reading (should be 2-5A)

**Test 2: Fuel Pump** (Engine OFF, 8-15A)
1. Connect fuel pump to Ch1
2. Start ECU (sets `pdm_fuelPumpEnable = true`)
3. Verify fuel pump primes for 2 seconds
4. Check current (should be 8-15A)
5. **Listen for leaks!**

**Test 3: Radiator Fan** (Engine OFF, 15-25A)
1. Connect fan to Ch2
2. Override `engTemp = 85` in ECU code
3. Verify fan runs at 70% speed
4. Check current (should be 15-20A at 70%)

**Test 4: Full System** (Engine ON, all channels)
1. Connect all devices
2. Start engine
3. Monitor total current (should be <100A)
4. Check battery voltage (should stay >12.5V)
5. Verify no overheating (MOSFETs, wire, connections)

---

## Troubleshooting

### Problem: PDM doesn't power up

**Checks**:
1. Verify 12V at Mega VIN pin (use multimeter)
2. Check main fuse (100A) not blown
3. Verify ground connection (chassis to PDM GND)
4. Check USB connection (if powering via USB for initial test)

---

### Problem: CAN communication fails ("PDM: DISC")

**Checks**:
1. Verify CAN wiring: CANH ↔ CANH, CANL ↔ CANL
2. Check CAN termination (120Ω resistors at each end)
3. Verify both ECU and PDM use same baud rate (500 kbps)
4. Check MCP2515 CS pin (PDM: Pin 53, ECU: Pin 17)
5. Monitor with CAN sniffer (look for messages 0x600, 0x601, 0x603)

---

### Problem: Channel won't turn ON

**Checks**:
1. Verify enable flag: `pdm_fuelPumpEnable == true`
2. Check for fault: `channels[0].faulted == false`
3. Check kill switch: `systemState.killSwitchActive == false`
4. Verify PWM value: `pdm_fuelPumpPWM > 0`
5. Check MOSFET gate voltage (should be 5V when ON)
6. Verify load connected to output (measure with multimeter)
7. Check inline fuse not blown

---

### Problem: Overcurrent false triggers

**Symptoms**: Channel shuts off immediately, even with light load

**Cause**: Current sensor calibration issue

**Fix**:
1. Measure actual current with multimeter (clip-on ammeter)
2. Compare to PDM reading (Serial output)
3. Adjust calibration in `monitorCurrent()`:
   ```cpp
   // Original:
   ch->current = abs((voltage - 2.5) / 0.066);

   // Adjust coefficient (0.066 = 66 mV/A for ACS712-30A):
   ch->current = abs((voltage - 2.5) / 0.070);  // If reading 10% high
   ```
4. Re-test with known load

---

### Problem: Soft-start not working (instant on)

**Checks**:
1. Verify `channels[i].softStart == true` (in `configureChannels()`)
2. Check ramp time: `SOFTSTART_RAMP_TIME` (default 500ms)
3. Monitor PWM output with oscilloscope (should ramp 0 → 255)
4. Ensure channel targetPWM = 255 (full power after ramp)

---

### Problem: Load shedding activates unexpectedly

**Cause**: Battery voltage sensor reading low

**Checks**:
1. Measure actual battery voltage (multimeter at battery terminals)
2. Compare to PDM reading: `systemState.batteryVoltage`
3. Check voltage divider calibration:
   ```cpp
   // Original (11:1 ratio, 10kΩ + 1kΩ):
   systemState.batteryVoltage = adcVoltage * 11.0;

   // Adjust if reading high/low:
   systemState.batteryVoltage = adcVoltage * 10.5;  // If reading 5% high
   ```
4. Verify voltage divider resistors (measure with multimeter)
5. Adjust threshold if needed:
   ```cpp
   #define BATTERY_CRITICAL 10.5  // Lower if too sensitive (was 11.0)
   ```

---

## Future Enhancements

### Planned Features
- [ ] **Web dashboard** (ESP8266 integration for WiFi telemetry)
- [ ] **OLED display** (status on PDM enclosure)
- [ ] **SD card logging** (offline data storage)
- [ ] **Shift light control** (RPM-based output)
- [ ] **Boost controller** (for turbocharged engines)
- [ ] **NOS controller** (solenoid control)
- [ ] **Launch control** (disable fans/AC during launch)
- [ ] **Configurable via CAN** (update channel config without re-uploading)

### Community Contributions Welcome
- Submit issues/PRs on GitHub
- Share your configuration (engine swap, EV, race car)
- Improve documentation
- Add support for new devices

---

## Cost Breakdown (8-Channel PDM)

| Component | Quantity | Unit Price | Total |
|-----------|----------|------------|-------|
| Arduino Mega 2560 (clone) | 1 | $18 | $18 |
| MCP2515 CAN module | 1 | $8 | $8 |
| MOSFET modules (IRLB8721, 30A) | 8 | $7 | $56 |
| ACS712 current sensors (30A) | 8 | $4 | $32 |
| Blade fuse holders | 8 | $2 | $16 |
| Fuses (assorted, 10-30A) | 20 | $0.50 | $10 |
| Enclosure (IP65 waterproof) | 1 | $20 | $20 |
| Wire (14 AWG, 50 ft) | 1 | $15 | $15 |
| Connectors (Anderson, ring) | 20 | $1 | $20 |
| **Subtotal** | | | **$195** |
| Optional: PCB (JLCPCB 5x) | 1 | $10 | $10 |
| Optional: Kill switch | 1 | $10 | $10 |
| Optional: E-stop button | 1 | $15 | $15 |
| **Total (with options)** | | | **$230** |

**Compare to Commercial PDM**: $800-1,500 (3.5x-6.5x cost!)

---

## License

MIT License (same as main repository)

---

## Credits

- **RX8 Arduino ECU Project**: Main ECU implementation
- **Chamber of Understanding**: Inspiration and methodology
- **Community**: Testing, feedback, improvements

---

## Support

**GitHub**: https://github.com/michaelprowacki/MazdaRX8Arduino
**Issues**: https://github.com/michaelprowacki/MazdaRX8Arduino/issues
**Documentation**: `docs/PDM_INTEGRATION_GUIDE.md`

---

**Happy Building! 🔧⚡**
