# Odometer Implementation - Test Procedure

**Created**: 2025-11-17
**Status**: Ready for Testing
**Implementation**: ECU_Module/RX8_CANBUS.ino

---

## Implementation Summary

✅ **Odometer function implemented** with accurate formula from Chamber of Understanding Part 6:
- **4,140 increments per mile** (not 0.1 mile per increment as originally thought)
- Formula: `interval = 869,565 / vehicleSpeed` microseconds per increment
- Updates `send420[1]` automatically based on wheel speed

### Code Changes

| File | Lines | Change |
|------|-------|--------|
| `ECU_Module/RX8_CANBUS.ino` | 99-101 | Added odometer timing variables |
| `ECU_Module/RX8_CANBUS.ino` | 246-269 | Added `updateOdometer()` function |
| `ECU_Module/RX8_CANBUS.ino` | 229 | Modified `updateMIL()` to use `odo` byte |
| `ECU_Module/RX8_CANBUS.ino` | 289 | Call `updateOdometer()` in `sendOnTenth()` |
| `ECU_Module/RX8_CANBUS.ino` | 181-183 | Initialize odometer variables in `setDefaults()` |

---

## Quick Test (Bench - No Vehicle Required)

### Test 1: Code Compilation

```bash
cd /home/user/MazdaRX8Arduino/ECU_Module
# If using Arduino IDE:
# 1. Open RX8_CANBUS.ino
# 2. Select Tools → Board → Arduino Leonardo
# 3. Click Verify (checkmark icon)
#
# If using arduino-cli:
arduino-cli compile --fqbn arduino:avr:leonardo RX8_CANBUS.ino
```

**Expected Result**: ✅ Compiles without errors

---

### Test 2: Simulated Speed (Serial Monitor)

**Goal**: Verify increment timing at different speeds

**Setup**:
1. Upload code to Arduino
2. Open Serial Monitor (115200 baud)
3. Temporarily add debug output to `updateOdometer()`:

```cpp
// In updateOdometer() after line 263:
if (micros() - lastOdometerUpdate >= interval) {
    odometerByte++;
    odo = odometerByte;
    lastOdometerUpdate = micros();

    // ADD THIS DEBUG OUTPUT:
    Serial.print("ODO: ");
    Serial.print(odometerByte);
    Serial.print(" @ ");
    Serial.print(vehicleSpeed);
    Serial.println(" mph");
}
```

4. Temporarily override `vehicleSpeed` in `setDefaults()` or `loop()`:

```cpp
// Add to loop() before sendOnTenth():
vehicleSpeed = 60;  // Simulate 60 mph
```

**Expected Results**:
| Speed | Increment Interval | Increments/Second |
|-------|-------------------|-------------------|
| 60 mph | ~14.5 ms | ~69/sec |
| 30 mph | ~29 ms | ~35/sec |
| 10 mph | ~87 ms | ~11/sec |

**Verification**:
- Count increments in 1 minute
- At 60 mph: Should see ~4,140 increments (1 mile)
- `odometerByte` wraps at 255 → 0 (normal behavior)

---

### Test 3: Stop/Start Behavior

**Goal**: Verify odometer stops when vehicle is stopped

**Procedure**:
1. Set `vehicleSpeed = 0;` in loop
2. Observe: No odometer increments (no Serial output)
3. Set `vehicleSpeed = 30;`
4. Observe: Increments resume immediately

**Expected Result**: ✅ No increments when stopped, resumes when moving

---

## Vehicle Test (In-Car)

⚠️ **IMPORTANT**: Document cluster odometer BEFORE installing Arduino ECU!

### Pre-Test Checklist

- [ ] Record current cluster odometer: __________ miles
- [ ] GPS app ready (for measuring exact distance)
- [ ] Known route (e.g., 1.0 mile loop)
- [ ] Camera/photo of cluster before test
- [ ] Backup key (in case of issues)

---

### Test Procedure

#### Test 1: 1-Mile Accuracy Test

1. **Baseline**: Note cluster odometer before starting
2. **Drive**: Travel exactly 1.0 mile (verify with GPS)
3. **Check**: Cluster should increment by 1.0 mile (±0.05 mile tolerance)

**Acceptable Tolerance**:
- ✅ **Excellent**: ±1% (0.99 - 1.01 miles)
- ✅ **Good**: ±5% (0.95 - 1.05 miles)
- ⚠️ **Needs Calibration**: ±10% (0.90 - 1.10 miles)
- ❌ **Problem**: >±10% error

#### Test 2: 10-Mile Accuracy Test

1. Drive exactly 10.0 miles (GPS verified)
2. Cluster should increment by 10.0 miles (±0.5 mile tolerance)

**Calculation**:
```
Accuracy % = (Cluster Miles / Actual Miles) × 100%
Example: (10.2 / 10.0) × 100% = 102% (2% high)
```

---

### Calibration (If Needed)

**Problem**: Odometer reads high or low by >5%

**Solution**: Add calibration factor to `updateOdometer()` function

#### Step 1: Calculate Calibration Factor

```
Calibration Factor = Actual Miles / Cluster Miles

Example:
- Drive 10.0 miles (GPS)
- Cluster shows 9.5 miles
- Factor = 10.0 / 9.5 = 1.053
```

#### Step 2: Apply Calibration

Edit `updateOdometer()` in `ECU_Module/RX8_CANBUS.ino`:

```cpp
void updateOdometer() {
  if (vehicleSpeed > 0) {
    // Add calibration factor here:
    unsigned long interval = (869565UL / vehicleSpeed) * 1.053;  // ← Add your factor
    // ... rest of function
  }
}
```

Or add a #define at the top of the file:

```cpp
// Add near line 45 (after #define LED3 7):
#define ODOMETER_CALIBRATION 1.053  // Adjust based on testing

// Then in updateOdometer():
unsigned long interval = (869565UL * ODOMETER_CALIBRATION) / vehicleSpeed;
```

#### Step 3: Re-Test

- Drive another 10 miles
- Verify accuracy improves to ±1%

---

### Common Calibration Values

| Symptom | Likely Cause | Calibration Factor |
|---------|--------------|-------------------|
| Reads 5% high | Smaller tires | 0.95 |
| Reads 5% low | Larger tires | 1.05 |
| Reads 10% high | Wheel speed sensor issue | 0.90 |
| Reads 10% low | Different differential ratio | 1.10 |

**Note**: Factors outside 0.90-1.10 suggest a problem beyond tire size. Check wheel speed sensors (CAN 0x4B0/0x4B1).

---

## Troubleshooting

### Problem: Odometer Doesn't Increment

**Symptoms**:
- Cluster shows frozen mileage
- No increments in Serial debug output

**Possible Causes**:
1. **Vehicle speed = 0**
   - Check wheel speed CAN messages (ID 0x4B0/0x4B1)
   - Serial monitor should show speed when driving

2. **Odometer function not called**
   - Verify `updateOdometer()` is called in `sendOnTenth()` (line 289)
   - Check 100ms timing loop is running

3. **send420[1] not updated**
   - Verify `updateMIL()` sets `send420[1] = odo;` (line 229)

**Debug Steps**:
```cpp
// Add to updateOdometer() to verify it's being called:
Serial.print("updateOdometer called, speed=");
Serial.println(vehicleSpeed);
```

---

### Problem: Odometer Increments Too Fast

**Symptoms**:
- Cluster shows 2 miles after driving 1 mile
- Increments twice as fast as expected

**Possible Causes**:
1. **Speed sensor reads double**
   - Check `vehicleSpeed` value in Serial monitor
   - May need calibration factor of 0.5

2. **Interval calculation error**
   - Verify formula: `869565UL / vehicleSpeed`
   - NOT `869565 * vehicleSpeed` (would be WAY too fast)

**Fix**: Apply calibration factor (see above)

---

### Problem: Odometer Increments Too Slow

**Symptoms**:
- Cluster shows 0.5 miles after driving 1 mile
- Half speed increments

**Possible Causes**:
1. **Speed sensor reads half**
   - Check `vehicleSpeed` value
   - May be reading KPH instead of MPH
   - KPH × 0.621 = MPH

2. **Wrong formula**
   - OLD formula: `360000000UL / vehicleSpeed` (0.1 mile per increment)
   - CORRECT formula: `869565UL / vehicleSpeed` (4,140 increments/mile)

**Fix**: Verify line 252 uses `869565UL` constant

---

### Problem: Odometer Jumps Erratically

**Symptoms**:
- Increments sporadically
- Sometimes increments 10+ times at once

**Possible Causes**:
1. **micros() overflow** (should be handled, but check)
2. **vehicleSpeed fluctuations**
   - Wheel speed mismatch detection (line 365-367)
   - Check for "5 kph difference" warning

**Debug Steps**:
```cpp
// Add to loop() to monitor speed stability:
Serial.print("FL: "); Serial.print(frontLeft);
Serial.print(" FR: "); Serial.println(frontRight);
```

---

## Legal Compliance

### Federal Odometer Law (49 USC § 32703)

✅ **Our Implementation**:
- Only increments (never decrements)
- Based on actual wheel speed
- Matches factory odometer behavior

⚠️ **Your Responsibilities**:
1. **Document** cluster odometer before/after installation
2. **Disclose** ECU replacement when selling vehicle
3. **Keep records** of mileage during Arduino ECU operation
4. **Do NOT** disconnect or disable odometer

**Penalties for Tampering**:
- Federal: Up to $10,000 fine + 3 years imprisonment
- State: Varies (check local laws)

---

## Success Criteria

### ✅ Implementation Complete

- [ ] Code compiles without errors
- [ ] Bench test: Increments at correct rate (60 mph = ~69/sec)
- [ ] Bench test: Stops when speed = 0
- [ ] Vehicle test: 1 mile accuracy ±5%
- [ ] Vehicle test: 10 mile accuracy ±5%
- [ ] No check engine light
- [ ] No cluster errors
- [ ] Odometer displays correctly on dashboard

### ⚠️ Calibration Needed

- [ ] Accuracy 5-10% off
- [ ] Calibration factor applied and tested
- [ ] Final accuracy ±2% or better

### ❌ Problem - Needs Investigation

- [ ] >10% error after calibration
- [ ] Erratic increments
- [ ] Odometer not incrementing at all
- [ ] Check engine light triggered

---

## Next Steps After Testing

### If Successful ✅
1. Remove debug Serial.print statements
2. Document final calibration factor (if used)
3. Update CLAUDE.md with odometer feature
4. Commit changes to git
5. Share results with community

### If Problems ❌
1. Document exact symptoms
2. Post Serial monitor output
3. Check wheel speed CAN messages (0x4B0/0x4B1)
4. Ask for help on GitHub Issues or RX8Club

---

## References

- **Implementation Guide**: `docs/ODOMETER_IMPLEMENTATION_GUIDE.md`
- **Research Document**: `docs/RX8_BCM_CANBUS_RESEARCH.md`
- **Source**: Chamber of Understanding Part 6 (chamberofunderstanding.co.uk)
- **Formula**: 4,140 increments per mile = 869,565 microseconds/increment @ V mph

---

**Test Status**: ⏳ Awaiting User Testing

**Last Updated**: 2025-11-17
