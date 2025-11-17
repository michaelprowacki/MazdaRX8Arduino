# Vehicle Testing & Validation - RX8 Unified ECU

**CRITICAL: Read this entire document before vehicle installation**

⚠️ **SAFETY WARNING**: This ECU replacement controls critical vehicle systems. Improper installation or testing can result in loss of vehicle control, collision, injury, or death. Only proceed if you have automotive electrical experience and accept all risks.

---

## Legal & Safety Disclaimer

```
THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND.
AUTHOR ASSUMES NO LIABILITY FOR VEHICLE DAMAGE, INJURY, OR DEATH.

By proceeding with vehicle installation, you acknowledge:
- You have automotive electrical experience
- You accept all risks and liability
- You will test thoroughly in safe environment
- You understand this voids vehicle warranty
- You comply with all local laws/regulations
```

---

## Table of Contents

1. [Pre-Installation Requirements](#pre-installation-requirements)
2. [Installation Procedure](#installation-procedure)
3. [Initial Power-On (Engine Off)](#initial-power-on-engine-off)
4. [Dashboard & Warning Light Testing](#dashboard--warning-light-testing)
5. [Immobilizer Testing](#immobilizer-testing)
6. [Throttle Response Testing](#throttle-response-testing)
7. [Dynamic Testing (On-Road)](#dynamic-testing-on-road)
8. [Long-Term Validation](#long-term-validation)
9. [Troubleshooting](#troubleshooting)
10. [Rollback Procedure](#rollback-procedure)

---

## Pre-Installation Requirements

### Mandatory Prerequisites

- ✅ **ALL bench tests passed** (see `TEST_PROCEDURES.md`)
- ✅ **ALL CAN bus tests passed**
- ✅ **Integration testing complete**
- ✅ **Original ECU backed up** (ROM dump via J2534 tool)
- ✅ **Wiring diagram created** for your specific installation
- ✅ **Second person present** for safety (recommended)
- ✅ **Fire extinguisher nearby** (ABC or BC rated)

### Recommended Tools

- Laptop with CAN analyzer software
- Multimeter
- OBD-II scanner (for error code checking)
- Socket set (for ECU removal/installation)
- Wire crimpers/soldering iron
- Electrical tape / heat shrink
- Zip ties for cable management

### Test Environment

**CRITICAL**: Initial testing must be in controlled environment:
- ✅ Private property or closed course (NOT public roads)
- ✅ Low traffic area
- ✅ Good weather (dry, daylight)
- ✅ Emergency stop area available
- ✅ Second vehicle nearby (for emergency transportation)

---

## Installation Procedure

### Step 1: Disconnect Battery

```
1. Turn off ignition, remove key
2. Open hood
3. Disconnect negative (-) terminal first
4. Disconnect positive (+) terminal
5. Wait 15 minutes (capacitor discharge)
```

⚠️ **WARNING**: This will reset radio presets, clock, etc.

### Step 2: Remove Factory ECU

```
RX8 ECU Location:
- Behind passenger-side dashboard (right-hand drive: driver side)
- Remove lower trim panel
- Disconnect main harness connector (large multi-pin)
- Remove mounting bolts
- Carefully extract ECU
```

**IMPORTANT**: Label all connectors before disconnecting!

### Step 3: Wiring Connections

#### Option A: Direct PCM Replacement

```
Replace factory PCM with automotive ECU board:
- Use same connector pinout
- Requires custom PCB or adapter harness
```

#### Option B: Parallel Installation (Recommended for Testing)

```
Leave factory PCM connected (initially):
- Tap into CAN bus wires (CAN-H, CAN-L, GND)
- Connect power (12V switched, GND)
- Install in separate enclosure
```

**CAN Bus Tap Points**:
```
RX8 OBD-II Port:
- Pin 6: CAN-H (Yellow wire typically)
- Pin 14: CAN-L (Green wire typically)
- Pin 4/5: GND
```

#### Power Connections

```
12V Switched (IGN):
- Tap into ignition-switched 12V source
- Use 5A fuse minimum
- Connect to automotive ECU VIN

GND:
- Connect to chassis ground
- Use heavy gauge wire (14 AWG minimum)
- Ensure clean, tight connection
```

#### UART to ESP32

```
Connect automotive ECU ↔ ESP32:
- STM32 UART_TX → ESP32 RX (GPIO16)
- STM32 UART_RX ← ESP32 TX (GPIO17)
- Common GND
```

### Step 4: Initial Wiring Check

**Before reconnecting battery**:

- [ ] Verify CAN-H connected to correct pin
- [ ] Verify CAN-L connected to correct pin
- [ ] Check for shorts (multimeter continuity test)
- [ ] Verify 120Ω termination resistors present
- [ ] Check all GND connections solid
- [ ] Verify no loose wires touching metal

---

## Initial Power-On (Engine Off)

### Step 5: Reconnect Battery

```
1. Reconnect positive (+) terminal
2. Reconnect negative (-) terminal
3. DO NOT start engine yet!
```

### Step 6: Verify Power

**Check LED indicators**:
- ✅ STM32/C2000/S32K power LED should be on
- ✅ ESP32 power LED should be on

**Check serial output**:
```bash
# Connect laptop to automotive ECU via USB
# Open serial monitor @ 115200 baud

Expected:
[HAL] STM32F407 HAL Initialization
[SAFETY] Watchdog initialized
[CAN] Controller initialized @ 500 kbps
...
=== AUTOMOTIVE ECU READY ===
```

**If no serial output**:
- Check USB cable connection
- Verify COM port selection
- Check power supply (measure 12V at VIN pin)

### Step 7: Turn Ignition to ON (Do Not Start)

```
Turn key to ON position (or press START button without pressing brake)
Engine should NOT crank yet!
```

**Observe**:
- Dashboard lights up normally
- Fuel pump primes (listen for buzz from rear)
- Warning lights illuminate (normal for key-on/engine-off)

**Check CAN bus traffic**:
```bash
# Using CAN analyzer
candump can0

Expected messages from vehicle:
- 0x4B1: Wheel speeds (all zero when stationary)
- Other modules broadcasting
```

**Check ECU transmission**:
```
Expected messages FROM automotive ECU:
- 0x201: PCM status (every 100ms)
- 0x420: MIL/warnings (every 100ms)
- 0x620, 0x630, 0x650: ABS messages
```

---

## Dashboard & Warning Light Testing

### Test 8: Dashboard Gauges

**With ignition ON, engine OFF**:

#### Speedometer Test

```
Expected: 0 km/h (or mph)

Note: Speedometer reads from wheel speed sensors.
At standstill, should show 0.
```

#### Tachometer Test

```
Expected: 0 RPM

If showing non-zero RPM with engine off:
- Check CAN message 0x201 encoding
- Verify RPM multiplier (3.85)
```

#### Fuel Gauge Test

```
Expected: Shows current fuel level

If gauge not working:
- RX8 fuel gauge may be controlled by gauge cluster (not ECU)
- Or requires specific CAN message
```

#### Temperature Gauge Test

```
Expected: Shows coolant temperature (should be ambient temp when cold)

Set test temperature:
vehicle_state.coolant_temp = 145 * 10;  // 145°C (normal operating)

Expected on dashboard: Gauge moves to middle position
```

### Test 9: Warning Lights

#### MIL (Malfunction Indicator Lamp / Check Engine)

```cpp
// Turn MIL on
vehicle_state.warnings.check_engine = 1;

// Expected: Check engine light illuminates on dashboard
```

#### ABS Light

```cpp
// Turn ABS light on
ABSDSC::setABSWarning(true);

// Expected: ABS warning light illuminates
```

**Common Issue**: ABS light won't turn off
```
Cause: Wrong ABS variant byte
Fix: Try different values in vehicle_config.h:
    #define ABS_VARIANT  2  // Try 2, 3, or 4
```

#### Oil Pressure Light

```cpp
vehicle_state.warnings.oil_pressure = 1;

// Expected: Oil pressure warning light on
```

#### Battery/Charging Light

```cpp
vehicle_state.warnings.battery_charge = 1;

// Expected: Battery warning light on
```

### Pass Criteria

- ✅ All dashboard gauges respond to ECU commands
- ✅ Warning lights can be controlled via CAN messages
- ✅ No unexpected lights (e.g., ABS should be off when configured correctly)

---

## Immobilizer Testing

### Test 10: Immobilizer Bypass

**CRITICAL**: Engine will not start if immobilizer not unlocked!

#### Procedure

```
1. Turn ignition OFF
2. Wait 5 seconds
3. Monitor serial output on automotive ECU
4. Turn ignition ON

Expected serial output:
[IMMOB] Received request (0x47): 7F 02
[IMMOB] Sending response A (0x41)
[IMMOB] Received request (0x47): 5C F4
[IMMOB] Sending response B (0x41)
[IMMOB] Immobilizer UNLOCKED - vehicle can start
```

#### Verification

```cpp
if (Immobilizer::isUnlocked()) {
    Serial.println("✅ Immobilizer unlocked - safe to start");
} else {
    Serial.println("❌ Immobilizer LOCKED - DO NOT attempt to start!");
}
```

**If immobilizer not unlocking**:
1. Check CAN message 0x047 is being received
2. Verify response on 0x041 matches expected bytes
3. Check timing (responses must be immediate)

### Pass Criteria

- ✅ Immobilizer handshake completes successfully
- ✅ "UNLOCKED" message appears in serial output
- ✅ Security light on dashboard goes off (not blinking)

---

## Throttle Response Testing

⚠️ **EXTREME CAUTION**: Uncontrolled throttle can cause accident!

### Test 11: Throttle Pedal Calibration (Engine Off)

```
1. Turn ignition ON, engine OFF
2. Monitor serial output

Expected at startup:
[THROTTLE] Calibrating... pedal at rest
[THROTTLE] Zero position: 341 (ADC counts)
[THROTTLE] Calibration complete
[THROTTLE] Range: 341 - 803 (1.7V - 4.0V)
```

**Press throttle pedal slowly**:
```
Expected serial output:
Throttle: 0%   (pedal released)
Throttle: 25%  (1/4 pressed)
Throttle: 50%  (1/2 pressed)
Throttle: 75%  (3/4 pressed)
Throttle: 100% (full throttle)
```

**Check for safety limits**:
```cpp
// Should clamp to 0-100% range
if (throttle_percent > 100) {
    Serial.println("ERROR: Throttle exceeded 100%!");
}
```

### Test 12: Throttle Output (Engine Off)

**WARNING**: This test should be done with spark plugs DISCONNECTED to prevent accidental start!

```
Measure PWM output to motor controller:
- Connect multimeter (DC voltage mode) to THROTTLE_PWM_PIN
- Press throttle pedal slowly

Expected:
- 0% throttle = 0V
- 50% throttle = ~1.65V (50% of 3.3V)
- 100% throttle = ~3.3V
```

### Pass Criteria

- ✅ Throttle calibrates correctly on startup
- ✅ Pedal position reads 0-100% smoothly
- ✅ PWM output voltage linear with pedal position
- ✅ No sudden jumps or dead zones

---

## First Engine Start

⚠️ **CRITICAL SAFETY STEPS**:

1. **Ensure vehicle in NEUTRAL or PARK**
2. **Apply parking brake firmly**
3. **Chock wheels** (front and rear)
4. **Second person ready to disconnect battery** in emergency
5. **Fire extinguisher within reach**
6. **Clear area around vehicle**

### Test 13: Engine Cranking (Do Not Start)

```
1. Turn key to START position (or press START button)
2. Engine should crank but DO NOT let it start (release immediately)
3. Verify:
   - Starter motor engages
   - Engine cranks normally
   - No unusual sounds
   - CAN traffic continues during cranking
```

### Test 14: Engine Idle (First Start)

```
1. Ensure all safety precautions in place
2. Start engine
3. Immediately monitor:
   - RPM (should stabilize at ~800-1000 RPM)
   - Warning lights (should go off after 2-3 seconds)
   - Unusual sounds or vibrations
   - Serial output for errors
```

**Expected serial output**:
```
[ENGINE] RPM: 850
[IMMOB] Unlocked
[SAFETY] Watchdog OK
[CAN] Bus traffic normal
```

**If engine doesn't start**:
1. Check immobilizer unlocked
2. Verify fuel pump running
3. Check for error codes (OBD-II scanner)

**If engine starts but runs rough**:
1. Check throttle calibration
2. Verify RPM reading accurate
3. May need tuning (beyond scope of this ECU)

### Test 15: Throttle Response (Engine Running)

⚠️ **CAUTION**: Keep RPM < 3000 for initial testing!

```
1. Gently press throttle pedal
2. Expected:
   - RPM increases smoothly
   - No hesitation or surging
   - RPM displayed on dashboard matches actual
   - Serial output shows throttle %
```

**Monitor for issues**:
- ❌ Throttle sticks (ECU not releasing)
- ❌ RPM hangs (incorrect CAN encoding)
- ❌ Engine stalls (lean condition, not ECU fault)

### Pass Criteria

- ✅ Engine starts normally
- ✅ Idle stable (±50 RPM)
- ✅ Throttle response smooth and linear
- ✅ No warning lights (except those expected for testing)
- ✅ Can rev to 3000 RPM without issues

---

## Dynamic Testing (On-Road)

⚠️ **ONLY proceed if ALL stationary tests pass!**

### Test Environment

- Private property or closed course
- Low speed initially (< 50 km/h)
- Good visibility, dry conditions
- Emergency stop area identified
- Second person in vehicle (optional but recommended)

### Test 16: Low-Speed Driving

```
Speed: 20-50 km/h
Duration: 5-10 minutes

Monitor:
- Speedometer accuracy (compare to GPS)
- Throttle response while moving
- Dashboard gauges update correctly
- No warning lights appear
- CAN messages continue normally
```

**Check wheel speed sensors**:
```
Serial output should show:
Wheel FL: 20 km/h
Wheel FR: 20 km/h
Wheel RL: 20 km/h
Wheel RR: 20 km/h
```

**If wheel speeds differ significantly**:
- May indicate sensor issue (not ECU fault)
- Check for tire pressure differences
- Verify wheel speed mismatch detection working

### Test 17: Highway Speed Test

```
Speed: 50-100 km/h
Duration: 10-15 minutes

Monitor:
- Speed accuracy at higher speeds
- RPM tracking (should increase with speed)
- Coolant temperature (normal range)
- Fuel consumption (if monitored)
- No warning lights
```

**Check for wipers (if enabled)**:
```
At different speeds:
- < 30 km/h: Slower intermittent
- 30-80 km/h: Normal intermittent
- > 80 km/h: Faster intermittent

Verify wipers adjust timing based on speed
```

### Test 18: ABS/DSC Integration

**CAUTION**: Test on safe, controlled surface only!

```
Test ABS:
1. Drive at 30-40 km/h
2. Apply brakes firmly (not hard enough to trigger ABS)
3. Verify:
   - No ABS warning light
   - Brakes work normally
   - No CAN bus errors
```

**Do NOT test actual ABS activation** unless:
- On closed course
- Professional driver
- Proper safety equipment

### Test 19: Power Steering

```
While driving slowly (< 20 km/h):
1. Turn steering wheel left and right
2. Verify:
   - Power steering assistance working
   - No unusual sounds
   - Steering effort normal
```

**If power steering fails**:
- May require specific CAN message
- Check CAN traffic from steering module
- Consult RX8 service manual for steering CAN protocol

### Pass Criteria

- ✅ Speedometer accurate (±5%)
- ✅ RPM tracks vehicle speed correctly
- ✅ All dashboard gauges functional
- ✅ No warning lights (except intentionally set)
- ✅ Throttle response smooth at all speeds
- ✅ Brakes, steering, ABS functional
- ✅ No CAN bus errors or timeouts

---

## Long-Term Validation

### Test 20: Extended Drive Test

```
Duration: 1-2 hours continuous driving
Conditions: Mixed (city, highway, hills)

Monitor:
- Coolant temperature stability
- Battery voltage (charging system)
- Fuel consumption
- Any intermittent issues
- Watchdog resets (should be zero)
```

**Data Logging** (if WiFi enabled):
```
Enable telemetry:
- ThingSpeak or InfluxDB
- Log: RPM, speed, temp, voltage
- Review after test for anomalies
```

### Test 21: Cold Start Test

```
After vehicle sits overnight (cold engine):
1. Attempt to start
2. Verify:
   - Immobilizer unlocks
   - Engine starts normally
   - Warmup behavior correct
   - No issues when cold
```

### Test 22: Hot Start Test

```
After driving (engine at operating temperature):
1. Turn off engine
2. Wait 5 minutes (heat soak)
3. Restart
4. Verify:
   - No vapor lock issues
   - Starts normally when hot
   - Temperature readings stable
```

### Pass Criteria

- ✅ No watchdog resets during extended operation
- ✅ No warning lights during normal operation
- ✅ Fuel consumption reasonable (compare to baseline)
- ✅ Starts reliably (cold and hot)
- ✅ Temperature and voltage within normal range
- ✅ No degradation over time

---

## Troubleshooting

### Issue: Engine Won't Start

**Check**:
- [ ] Immobilizer unlocked (serial output)
- [ ] Fuel pump priming (listen for buzz)
- [ ] Spark present (check with spark tester)
- [ ] CAN messages transmitting (all required IDs)

### Issue: Dashboard Gauges Not Working

**Check**:
- [ ] CAN messages transmitting (use analyzer)
- [ ] Correct message encoding (RPM * 3.85, etc.)
- [ ] 100ms timing correct
- [ ] No CAN bus errors

### Issue: ABS Light Won't Turn Off

**Fix**:
```cpp
// Try different ABS variant:
#define ABS_VARIANT  2  // Try 2, 3, or 4

// Or try different 0x620 configuration
```

### Issue: Speedometer Incorrect

**Check**:
- [ ] Wheel speed sensors functioning
- [ ] CAN message 0x4B1 received correctly
- [ ] Speed encoding formula correct (mph conversion)

### Issue: Warning Lights Flashing/Erratic

**Cause**: CAN bus timing issue
**Fix**:
- Verify 100ms update rate
- Check for CAN bus overload
- Reduce debug serial output

### Issue: Watchdog Reset During Operation

**Cause**: Loop blocking too long
**Fix**:
- Check for blocking delays
- Verify SafetyMonitor::kick() called every loop
- Reduce processing time in loop

---

## Rollback Procedure

If testing reveals issues or you need to restore original functionality:

### Emergency: Immediate Rollback

```
1. Turn off ignition immediately
2. Disconnect battery
3. Disconnect automotive ECU from CAN bus
4. (If parallel installation) Factory PCM still connected
5. Reconnect battery
6. Start vehicle normally with factory PCM
```

### Permanent Rollback

```
1. Remove automotive ECU and ESP32
2. Restore all original wiring
3. Reinstall factory PCM (if removed)
4. Clear error codes with OBD-II scanner
5. Verify all systems functional
```

---

## Success Criteria

Vehicle is ready for normal use when:

- ✅ **ALL bench tests passed**
- ✅ **ALL CAN bus tests passed**
- ✅ **Immobilizer unlocks reliably**
- ✅ **Engine starts normally (cold and hot)**
- ✅ **Throttle response smooth and safe**
- ✅ **Dashboard gauges accurate**
- ✅ **No unexpected warning lights**
- ✅ **ABS/braking functional**
- ✅ **Power steering functional**
- ✅ **Extended drive test passed (2+ hours)**
- ✅ **No watchdog resets**
- ✅ **No CAN bus errors**

---

## Maintenance & Monitoring

### Regular Checks

**Weekly**:
- Check for new warning lights
- Verify WiFi telemetry working (if enabled)
- Review logged data for anomalies

**Monthly**:
- Check all electrical connections (corrosion, loose wires)
- Verify firmware still current (no watchdog resets)
- Update firmware if bugs fixed

**After Any Issues**:
- Download logs via serial/WiFi
- Review CAN bus traffic
- Consider rollback if persistent problems

---

## Final Notes

This ECU replacement is experimental and provided AS-IS. While extensive testing has been performed:

- ⚠️ No guarantee of reliability
- ⚠️ Not certified for safety-critical use
- ⚠️ Use at your own risk
- ⚠️ Author assumes no liability

**For production use**: Consider ISO 26262 certified platforms (S32K148, AURIX, Hercules).

**Support**: Community support available via GitHub issues, but no warranty or guarantee of response.

---

**Last Updated**: 2025-11-16
**Part of Phase 5 unified architecture**
