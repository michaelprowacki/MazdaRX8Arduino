# Odometer Increment Implementation Guide

**Last Updated**: 2025-11-17
**Priority**: **HIGH** ⭐
**Status**: Not Yet Implemented
**Estimated Effort**: 1-2 hours
**Risk Level**: Low (non-safety-critical)

---

## Overview

**Problem**: Our current implementation sets `send420[1]` (odometer byte) to a static value of `0`, meaning the odometer on the RX8 cluster does NOT increment as the vehicle drives.

**Solution**: Implement speed-based odometer increment logic (discovered from Chamber of Understanding blog Part 21).

**Legal Requirement**: Federal law (49 USC § 32703) prohibits odometer tampering. Our implementation will **only increment** (never decrement), which is legal and compliant.

---

## Current Implementation (Broken)

### RX8_CANBUS.ino:129, 182

```cpp
// setDefaults() - Line 129
send420[1] = 0;  // Odometer byte - STATIC (does not increment)

// updateMIL() - Line 182
send420[0] = engTemp;  // Byte 0: Engine temperature
send420[1] = 0;        // Byte 1: Odometer - STATIC (no change)
// ... rest of function
```

**Result**: Cluster odometer shows frozen mileage (or increments incorrectly).

---

## How RX8 Odometer Works

### CAN Message 0x420 Structure

| Byte | Function | Value Range | Notes |
|------|----------|-------------|-------|
| 0 | Engine Temperature | 0-255 | 145 = normal |
| **1** | **Odometer Increment** | **0-255** | **Increments by 1 per 0.1 mile** |
| 2 | Unknown | 0 | Static |
| 3 | Unknown | 0 | Static |
| 4 | Oil Pressure | 0-1 | 1 = OK |
| 5 | MIL Lights (bits) | 0-255 | Bit 6 = Check Engine |
| 6 | Warning Lights (bits) | 0-255 | Bit 7 = Oil Pressure |

### Key Insight: Byte 1 Behavior

**Discovery** (from Chamber blog & our testing):
- Cluster **counts byte 1 increments**, NOT absolute value
- Each increment = **0.1 mile** (160 meters)
- Byte wraps around (255 → 0) automatically
- Cluster maintains internal odometer total

**Example**:
```
Initial cluster odometer: 100,000.0 miles
Send: send420[1] = 5
Cluster increments by 5 * 0.1 = 0.5 miles
New cluster odometer: 100,000.5 miles

Next time:
Send: send420[1] = 6  (incremented by 1)
Cluster increments by 1 * 0.1 = 0.1 miles
New cluster odometer: 100,000.6 miles
```

**Important**: We only need to increment the byte, NOT calculate absolute mileage.

---

## Calculation Formula

### Speed to Odometer Interval

**Goal**: Increment `send420[1]` by 1 every time the vehicle travels 0.1 mile.

**Formula**:
```
Distance per increment = 0.1 mile
Time to travel 0.1 mile at V mph = (0.1 / V) hours = (0.1 / V) * 60 minutes = (6 / V) minutes
Convert to microseconds: (6 / V) * 60 * 1,000,000 = (360,000,000 / V) microseconds

interval_microseconds = 360,000,000 / vehicleSpeed
```

**Examples**:
| Speed (mph) | Time per 0.1 mile | Microseconds | Increment Frequency |
|-------------|-------------------|--------------|---------------------|
| 60 | 6 seconds | 6,000,000 μs | Every 6 seconds |
| 30 | 12 seconds | 12,000,000 μs | Every 12 seconds |
| 10 | 36 seconds | 36,000,000 μs | Every 36 seconds |
| 1 | 360 seconds (6 min) | 360,000,000 μs | Every 6 minutes |

---

## Implementation

### Step 1: Add Global Variables

**Location**: Top of RX8_CANBUS.ino (around line 70)

```cpp
// Add after existing global variables (line ~70)
// Odometer tracking
static unsigned long lastOdometerUpdate = 0;  // Last time odometer was incremented (microseconds)
static byte odometerByte = 0;                  // Current odometer increment byte (0-255, wraps around)
```

**Why `static`?**
- Preserves value between function calls
- `lastOdometerUpdate` tracks last increment time
- `odometerByte` tracks current increment count (wraps at 255)

---

### Step 2: Create `updateOdometer()` Function

**Location**: After `updatePCM()` function (around line 220)

```cpp
/**
 * Update odometer increment based on vehicle speed
 * Called every loop cycle (~100ms)
 * Increments send420[1] every 0.1 mile traveled
 *
 * Formula: 0.1 mile at V mph = (360,000,000 / V) microseconds
 */
void updateOdometer() {
    // Only increment if vehicle is moving
    if (vehicleSpeed > 0) {
        // Calculate microseconds per 0.1 mile at current speed
        // UL = unsigned long literal (prevents integer overflow)
        unsigned long interval = 360000000UL / vehicleSpeed;

        // Check if enough time has passed since last increment
        if (micros() - lastOdometerUpdate >= interval) {
            // Increment odometer byte (wraps automatically at 255 → 0)
            odometerByte++;

            // Update CAN message byte 1
            send420[1] = odometerByte;

            // Record this update time
            lastOdometerUpdate = micros();

            // Optional: Debug output
            #ifdef DEBUG_ODOMETER
                Serial.print("Odometer incremented: ");
                Serial.print(odometerByte);
                Serial.print(" (Speed: ");
                Serial.print(vehicleSpeed);
                Serial.println(" mph)");
            #endif
        }
    } else {
        // Vehicle stopped - no increment, but update timer to prevent overflow issues
        lastOdometerUpdate = micros();
    }
}
```

**Key Features**:
- ✅ Only increments when moving (`vehicleSpeed > 0`)
- ✅ Uses `micros()` for high precision (1 μs resolution)
- ✅ Automatic byte wraparound (255 → 0)
- ✅ Prevents overflow with `unsigned long` (UL suffix)
- ✅ Updates timer even when stopped (prevents wraparound issues)

---

### Step 3: Call from Main Loop

**Location**: `loop()` function (around line 276)

```cpp
void loop() {
    // Existing code...

    if (millis() - sendTime >= 100) {
        sendTime = millis();

        updateMIL();    // Update warning lights
        updatePCM();    // Update RPM, speed, throttle
        updateOdometer(); // ← ADD THIS LINE

        sendOnTenth();  // Transmit all CAN messages
    }

    // ... rest of loop
}
```

**Why here?**
- Called every 100ms (same as other updates)
- Called AFTER `updatePCM()` (which sets `vehicleSpeed`)
- Called BEFORE `sendOnTenth()` (which transmits `send420`)

---

### Step 4: Initialize in `setDefaults()`

**Location**: `setDefaults()` function (around line 129)

```cpp
void setDefaults() {
    // ... existing code ...

    // Odometer initialization
    send420[1] = 0;  // Start at 0 (cluster will increment from current value)
    odometerByte = 0;
    lastOdometerUpdate = micros();  // Initialize timer

    // ... rest of function ...
}
```

**Why initialize?**
- Sets starting values for odometer tracking
- `send420[1] = 0` is safe (cluster won't jump to wrong mileage)
- Timer initialized to current time (prevents immediate increment)

---

## Testing

### Bench Testing (Without Vehicle)

**Goal**: Verify calculation logic without driving

**Setup**:
1. Upload modified code to Arduino
2. Connect to Serial Monitor (115200 baud)
3. Enable debug output: `#define DEBUG_ODOMETER` (add at top of file)

**Test Cases**:

#### Test 1: Simulate 60 mph
```cpp
// In loop(), temporarily override vehicleSpeed
vehicleSpeed = 60;  // Force 60 mph
```

**Expected Result**:
- Odometer increments every **6 seconds**
- Serial output: `Odometer incremented: 1 (Speed: 60 mph)`
- After 60 seconds: `odometerByte` = 10 (10 * 0.1 mile = 1 mile traveled)

#### Test 2: Simulate 30 mph
```cpp
vehicleSpeed = 30;  // Force 30 mph
```

**Expected Result**:
- Odometer increments every **12 seconds**
- After 60 seconds: `odometerByte` = 5 (5 * 0.1 mile = 0.5 miles)

#### Test 3: Simulate Stop/Start
```cpp
// Alternate between stopped and moving
if (millis() % 20000 < 10000) {
    vehicleSpeed = 0;   // Stopped for 10 seconds
} else {
    vehicleSpeed = 60;  // Moving for 10 seconds
}
```

**Expected Result**:
- No increments during stopped periods
- Increments resume immediately when moving
- No timer overflow issues

---

### Vehicle Testing (In Car)

**⚠️ IMPORTANT**: Document current odometer reading BEFORE installation!

**Pre-Test Checklist**:
- [ ] Record current cluster odometer: __________ miles
- [ ] Install Arduino ECU with odometer code
- [ ] Drive **exactly 1.0 mile** (measure with GPS or known route)
- [ ] Check cluster odometer increment: __________ miles
- [ ] Verify increment matches actual distance (±0.1 mile tolerance)

**Test Procedure**:
1. **Baseline**: Note cluster odometer before starting
2. **Drive**: Travel exactly 1.0 mile (use GPS app for accuracy)
3. **Check**: Cluster should show +1.0 mile (±0.1 mile tolerance)
4. **Repeat**: Drive 5 miles, verify +5.0 miles on cluster

**Acceptable Tolerance**:
- ±5% error is acceptable (0.95 - 1.05 miles per actual mile)
- ±10% error: Needs calibration
- >±10% error: Check formula or speed sensor accuracy

**Troubleshooting**:

| Symptom | Cause | Fix |
|---------|-------|-----|
| Odometer increments too fast | Speed sensor reads high | Calibrate `vehicleSpeed` calculation |
| Odometer increments too slow | Speed sensor reads low | Calibrate `vehicleSpeed` calculation |
| Odometer doesn't increment | `vehicleSpeed` = 0 always | Check wheel speed CAN message 0x4B0 |
| Odometer jumps erratically | Byte wrapping issue | Check for overflow in interval calculation |

---

## Calibration (If Needed)

**Problem**: Speed sensor accuracy varies between RX8s (tire size, differential ratio).

**Solution**: Add calibration factor to `updateOdometer()` function.

### Calibration Formula

```cpp
// Measure actual miles driven vs. cluster miles shown
float calibrationFactor = actual_miles / cluster_miles;

// Example: Drive 10.0 miles, cluster shows 9.5 miles
calibrationFactor = 10.0 / 9.5 = 1.053

// Apply in updateOdometer():
unsigned long interval = (360000000UL / vehicleSpeed) * calibrationFactor;
```

**Steps**:
1. Drive known distance (e.g., 10 miles via GPS)
2. Compare cluster odometer increment
3. Calculate calibration factor
4. Add to code (see below)

### Calibrated Code

```cpp
// Add at top of file (after #includes)
#define ODOMETER_CALIBRATION 1.00  // Adjust based on testing (default: 1.00 = no adjustment)

void updateOdometer() {
    if (vehicleSpeed > 0) {
        unsigned long interval = (360000000UL / vehicleSpeed) * ODOMETER_CALIBRATION;

        if (micros() - lastOdometerUpdate >= interval) {
            odometerByte++;
            send420[1] = odometerByte;
            lastOdometerUpdate = micros();
        }
    } else {
        lastOdometerUpdate = micros();
    }
}
```

**Example Calibrations**:
- Odometer reads 5% high: `#define ODOMETER_CALIBRATION 1.05`
- Odometer reads 5% low: `#define ODOMETER_CALIBRATION 0.95`
- Perfect accuracy: `#define ODOMETER_CALIBRATION 1.00`

---

## Legal & Safety Considerations

### Federal Odometer Law (49 USC § 32703)

**Legal**:
- ✅ **Incrementing** odometer based on actual distance traveled
- ✅ Documenting mileage before/after installation
- ✅ Disclosing ECU replacement when selling vehicle

**Illegal**:
- ❌ **Decrementing** odometer (rollback)
- ❌ Disconnecting odometer entirely
- ❌ Failing to disclose ECU replacement to buyer

**Our Implementation**:
- ✅ **Only increments** (no decrement code)
- ✅ Calculates increment based on actual wheel speed
- ✅ Matches factory odometer behavior

**Recommendation**:
- Document cluster odometer reading BEFORE installation
- Document cluster odometer reading AFTER installation
- Keep records in case of future sale
- Disclose ECU replacement in vehicle history (e.g., CarFax report)

---

### Accuracy Requirements

**State Laws Vary**:
- Most states: No specific accuracy requirement for aftermarket ECUs
- Federal: Odometer must "accurately record" mileage (no specific tolerance)
- Good practice: ±5% accuracy or better

**Our Target**: ±1% accuracy (achievable with calibration)

---

## Known Issues & Limitations

### Issue 1: `micros()` Overflow

**Problem**: `micros()` returns `unsigned long` (32-bit) which overflows every ~71 minutes.

**Symptom**: Odometer might increment incorrectly once every 71 minutes.

**Solution**: Already handled in our code:
```cpp
if (micros() - lastOdometerUpdate >= interval) {
    // This subtraction handles overflow correctly!
}
```

**Why it works**: Unsigned integer overflow is well-defined in C++. The subtraction `micros() - lastOdometerUpdate` works correctly even if `micros()` wraps around from 4,294,967,295 to 0.

**No action needed** - overflow is automatically handled.

---

### Issue 2: Wheel Speed Sensor Failures

**Problem**: If wheel speed sensors fail, `vehicleSpeed` = 0 → odometer stops incrementing.

**Current Handling** (RX8_CANBUS.ino:314-319):
```cpp
// Safety check: detect wheel speed mismatch
if (abs(frontLeft - frontRight) > 500) {  // 5 kph difference
    checkEngineMIL = 1;  // Light up warning
    vehicleSpeed = 0;    // Safe state → odometer stops
}
```

**Symptom**: Check engine light ON, odometer frozen.

**Solution**: Fix wheel speed sensor issue, odometer will resume.

**Recommendation**: Add Serial debug output to alert user of sensor failures.

---

### Issue 3: Byte Wraparound at 255

**Problem**: `odometerByte` is `byte` (0-255). What happens at 255 + 1?

**Answer**: Automatic wraparound to 0 (C++ unsigned behavior).

**Example**:
```
odometerByte = 254
odometerByte++ → 255
odometerByte++ → 0  (wraps around)
```

**Does cluster handle this?**
- ✅ **Yes** - Cluster tracks increments, NOT absolute value
- Cluster internal odometer: 100,255.0 miles
- Receive `send420[1] = 255`
- Receive `send420[1] = 0` (next increment)
- Cluster interprets as +1 increment → 100,255.1 miles

**No action needed** - wraparound is expected and handled.

---

## Future Enhancements

### Enhancement 1: EEPROM Mileage Storage

**Goal**: Store total Arduino ECU mileage in EEPROM (survives power loss).

**Use Case**: Track mileage driven with Arduino ECU vs. factory PCM.

**Implementation**:
```cpp
#include <EEPROM.h>

// Store total 0.1-mile increments as unsigned long (4 bytes)
#define EEPROM_ADDR_MILEAGE 0  // Addresses 0-3

void updateOdometer() {
    // ... existing code ...

    if (/* increment occurred */) {
        // Read current total from EEPROM
        unsigned long totalIncrements;
        EEPROM.get(EEPROM_ADDR_MILEAGE, totalIncrements);

        // Increment and write back
        totalIncrements++;
        EEPROM.put(EEPROM_ADDR_MILEAGE, totalIncrements);

        // Calculate total miles
        float totalMiles = totalIncrements * 0.1;
    }
}
```

**Note**: EEPROM has limited write cycles (~100,000). At 1 write per 0.1 mile, this = 10,000 miles before wear-out. Use wear-leveling or write less frequently (e.g., every 1 mile).

---

### Enhancement 2: Serial Odometer Display

**Goal**: Display Arduino ECU mileage via Serial Monitor.

**Implementation**:
```cpp
// Add to updateOdometer()
if (odometerByte % 10 == 0) {  // Print every 1 mile
    Serial.print("Arduino ECU Miles: ");
    Serial.println(totalMiles);
}
```

---

### Enhancement 3: WiFi Odometer Dashboard

**Goal**: Display real-time odometer on web dashboard (AC Display ESP8266).

**Implementation**:
- Send odometer data via Serial to ESP8266
- ESP8266 serves web page with live mileage
- See `AC_Display_Module/ESP8266_Companion/` for integration

---

## Summary

### What We're Adding
- ✅ `updateOdometer()` function (calculates speed-based increments)
- ✅ Global variables (`lastOdometerUpdate`, `odometerByte`)
- ✅ Call from `loop()` every 100ms
- ✅ Initialize in `setDefaults()`

### Expected Results
- ✅ Cluster odometer increments accurately as vehicle drives
- ✅ Legal compliance (increments only, based on actual distance)
- ✅ ±1% accuracy (with calibration if needed)
- ✅ No impact on other ECU functions

### Testing Checklist
- [ ] Bench test: Verify 60 mph = 6 second increment
- [ ] Bench test: Verify 30 mph = 12 second increment
- [ ] Vehicle test: Drive 1 mile, verify cluster +1.0 mile
- [ ] Vehicle test: Drive 10 miles, verify cluster +10.0 miles (±0.5 mile)
- [ ] Document before/after odometer readings
- [ ] Calibrate if needed (adjust `ODOMETER_CALIBRATION`)

### Estimated Effort
- **Coding**: 30 minutes (copy/paste from this guide)
- **Bench Testing**: 15 minutes
- **Vehicle Testing**: 30 minutes (1-mile + 10-mile test drives)
- **Calibration** (if needed): 15 minutes
- **Total**: 1-2 hours

---

**This is a critical missing feature that needs implementation ASAP for legal compliance!**

---

## Next Steps

1. **Implement code changes** (see Step 1-4 above)
2. **Bench test** with simulated speeds
3. **Vehicle test** with known distances
4. **Calibrate** if accuracy <95%
5. **Document** before/after odometer readings
6. **Update CLAUDE.md** with odometer feature
7. **Commit and push** changes

---

*End of Odometer Implementation Guide*
