# System Integration Guide

**Complete integration of all advanced ECU features with safety interlocks**

This guide shows how to integrate all advanced features using the SystemManager for coordinated, safe operation.

---

## Overview

The **SystemManager** coordinates all advanced features and implements critical safety interlocks:

**Coordinated Features:**
- Rotary engine management (OMP, dual ignition, apex seals)
- Flex fuel (E85)
- Idle control
- Decel fuel cut
- Boost control + anti-lag
- Launch control
- Knock detection
- Water/methanol injection
- Data logging

**Safety Interlocks:**
- Water/meth failure → reduce boost AND retard timing
- Knock detected → reduce boost + increase water/meth
- Coolant overtemp → cut boost + disable launch
- Low oil pressure → emergency mode (idle only)
- Flex fuel sensor fail → revert to gasoline tune

---

## Quick Start Example

### Complete Integrated Setup

```cpp
#include "system_manager.h"

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    // Configure which features to enable
    SystemManager::FeatureFlags features = {
        .rotary_engine = true,      // OMP + dual ignition (always enable for rotary!)
        .flex_fuel = true,          // E85 support (if sensor installed)
        .idle_control = true,       // Closed-loop idle
        .decel_fuel_cut = true,     // Fuel economy (free feature!)
        .boost_control = true,      // If turbocharged
        .launch_control = true,     // 2-step launch
        .knock_detection = true,    // Knock sensor (critical for turbo!)
        .water_meth = true,         // If water/meth system installed
        .data_logging = true        // Black box logging
    };

    // Initialize system manager
    SystemManager::init(features);

    // Run startup self-test
    SystemManager::SelfTestResults test = SystemManager::runStartupSelfTest();

    if (!test.passed) {
        Serial.println("WARNING: Self-test failed!");
        Serial.println("System will run in SAFE MODE");
        // Still continues, but with advanced features disabled
    }

    // Start data logging session
    DataLogger::startSession(DataLogger::MODE_CONTINUOUS);

    Serial.println("System ready!");
}

void loop() {
    // Read all sensors
    uint16_t rpm = readRPM();
    uint8_t throttle = readThrottle();
    int16_t coolant_temp = readCoolantTemp();
    uint8_t oil_pressure = readOilPressure();
    uint16_t battery_voltage = readBatteryVoltage();
    uint16_t boost = readBoostSensor();
    uint16_t speed = readVehicleSpeed();
    uint8_t gear = getCurrentGear();

    // Update system manager
    // This coordinates ALL features and applies safety interlocks
    SystemManager::update(rpm, throttle, coolant_temp, oil_pressure,
                         battery_voltage, boost, speed, gear);

    // Get combined adjustments from all features
    const SystemManager::SystemAdjustments* adj = SystemManager::getAdjustments();

    // Apply adjustments to engine control
    int8_t base_timing = getBaseTiming(rpm, throttle);
    int8_t final_timing = base_timing + adj->timing_total;  // ±10° from all features

    uint16_t base_fuel = getBaseFuel(rpm, throttle);
    uint16_t final_fuel = (base_fuel * adj->fuel_multiplier) / 1000;

    uint16_t boost_target = getBaseBoostTarget(gear);
    boost_target = min(boost_target, adj->boost_limit);  // Safety limit

    // Apply to hardware
    setIgnitionTiming(final_timing);
    setFuelInjection(final_fuel);
    setBoostTarget(boost_target);

    // Check system health
    const SystemManager::SystemHealth* health = SystemManager::getHealth();
    if (!health->all_ok) {
        // Warning light, reduced power, etc.
        Serial.println("WARNING: System fault detected");
    }

    // Check for emergency mode
    SystemManager::SystemMode mode = SystemManager::getMode();
    if (mode == SystemManager::MODE_EMERGENCY) {
        // IDLE ONLY - Cut power
        setRPMLimit(2000);
        Serial.println("EMERGENCY MODE ACTIVE - IDLE ONLY!");
    }
}
```

---

## Safety Interlock Examples

### Example 1: Water/Meth Failsafe

**Scenario**: Tank runs empty during high-boost pull

**What Happens:**
1. Water/meth system detects tank level < 5%
2. Water/meth enters failsafe mode
3. **SystemManager detects failsafe**
4. Boost limit reduced to 7 PSI (from 15 PSI)
5. Ignition timing retarded 3° for safety
6. Driver sees warning light
7. Safe mode until tank refilled

**Code:**
```cpp
// Inside SystemManager::applySafetyInterlocks()
if (features.water_meth) {
    const WaterMethInjection::WMIStatus* wmi = WaterMethInjection::getStatus();
    if (wmi->failsafe_active) {
        // Reduce boost
        adjustments.boost_limit = min(adjustments.boost_limit, (uint16_t)70);  // 7 PSI
        // Retard timing
        adjustments.timing_total -= 3;
        // Log fault
        logFault("WMI_FAILSAFE");
    }
}
```

### Example 2: Knock Detection + Multi-Feature Response

**Scenario**: Knock detected at 12 PSI boost

**What Happens:**
1. Knock sensor detects detonation
2. Knock detection module retards timing 2°
3. **SystemManager coordinates response:**
   - Reduces boost limit to 10 PSI
   - Increases water/meth injection (if available)
   - Logs knock event to black box
4. Driver gets knock warning
5. Gradually recovers if knock stops

**Code:**
```cpp
// Inside SystemManager::applySafetyInterlocks()
if (features.knock_detection) {
    const KnockDetection::KnockStatus* knock = KnockDetection::getStatus();
    if (knock->knock_detected) {
        // Reduce boost
        adjustments.boost_limit = min(adjustments.boost_limit, (uint16_t)100);  // 10 PSI

        // Water/meth automatically increases with boost
        // (no explicit action needed)

        // Log fault
        logFault("KNOCK_DETECTED");
    }
}
```

### Example 3: Coolant Overtemp Protection

**Scenario**: Coolant reaches 115°C

**What Happens:**
1. Coolant temp sensor reads >115°C
2. **SystemManager enters safe mode**
3. All boost cut (0 PSI)
4. Launch control disabled
5. Power reduction active
6. Driver sees temperature warning
7. Engine can still run (no shutdown), but no power

**Code:**
```cpp
// Inside SystemManager::checkCriticalFailures()
if (coolant_temp > 1150) {  // >115°C
    enterSafeMode("Coolant overtemperature");
    logFault("COOLANT_OVERTEMP");

    // In applySafetyInterlocks()
    if (!health.coolant_temp_ok) {
        adjustments.boost_limit = 0;  // No boost
        if (features.launch_control) {
            LaunchControl::setEnabled(false);
        }
    }
}
```

### Example 4: Low Oil Pressure Emergency

**Scenario**: Oil pressure drops to 8 PSI at 5000 RPM

**What Happens:**
1. Oil pressure sensor reads <10 PSI
2. **SystemManager enters EMERGENCY MODE**
3. RPM limit set to 2000 (idle only)
4. 90% power reduction
5. All advanced features disabled
6. Driver sees critical warning
7. Must pull over immediately

**Code:**
```cpp
// Inside SystemManager::checkCriticalFailures()
if (oil_pressure < 10) {  // <10 PSI
    enterEmergencyMode("Low oil pressure");
    return true;  // Critical failure
}

// In enterEmergencyMode()
adjustments.rpm_limit = 2000;  // Idle only
adjustments.power_reduction_active = true;
adjustments.power_reduction_percent = 90;  // 90% power cut
```

---

## Combined Adjustments

### Timing Adjustment Example

**Scenario**: E85 fuel + water/meth injection + no knock

**Calculation:**
```
Base timing:        20° BTDC
Flex fuel (E85):    +5° (higher octane)
Water/meth:         +3° (charge cooling)
Knock retard:       -0° (no knock)
-------------------------
Final timing:       28° BTDC
```

**Code:**
```cpp
int8_t calculateTotalTimingAdjustment() {
    int8_t total = 0;

    // Flex fuel: +0-5° for E85
    if (features.flex_fuel) {
        total += FlexFuel::getStatus()->timing_adjustment;  // +5°
    }

    // Knock: -0-10° if knock detected
    if (features.knock_detection) {
        total -= KnockDetection::getStatus()->current_retard;  // -0°
    }

    // Water/meth: +0-5° with full injection
    if (features.water_meth && features.boost_control) {
        int8_t wmi_advance = WaterMethInjection::calculateTimingAdvance(
            boost, wmi_duty
        );
        total += wmi_advance;  // +3°
    }

    // Clamp to ±10°
    return constrain(total, -10, 10);  // +8° total
}
```

### Boost Limit Example

**Scenario**: 15 PSI base target + water/meth active + no knock

**Calculation:**
```
Base boost target:       15.0 PSI
Water/meth bonus:        +2.5 PSI (active, full duty)
Knock reduction:         -0.0 PSI (no knock)
Temp limit:             No reduction (temp OK)
-----------------------------------
Effective boost limit:   17.5 PSI
```

**Code:**
```cpp
uint16_t calculateEffectiveBoostLimit(uint16_t base_boost) {
    uint16_t limit = base_boost;  // 150 (15.0 PSI)

    // Water/meth: +0-3 PSI if active
    if (features.water_meth) {
        const WMIStatus* wmi = WaterMethInjection::getStatus();
        if (wmi->active && !wmi->failsafe_active) {
            uint16_t bonus = WaterMethInjection::calculateSafeBoostIncrease(
                base_boost, wmi->pump_duty
            );
            limit += bonus;  // +25 (2.5 PSI)
        }
    }

    // Knock: reduce to 10 PSI if knock
    if (features.knock_detection) {
        if (KnockDetection::getStatus()->knock_detected) {
            limit = min(limit, (uint16_t)100);  // No reduction
        }
    }

    // Temperature: cut boost if overheating
    if (!health.coolant_temp_ok) {
        limit = 0;  // No reduction
    }

    return limit;  // 175 (17.5 PSI)
}
```

### Fuel Multiplier Example

**Scenario**: E85 fuel

**Calculation:**
```
Base fuel:            100%
Flex fuel (E85):      ×1.30 (30% more fuel needed)
---------------------------------
Total fuel:           130%
```

**Code:**
```cpp
uint16_t calculateTotalFuelMultiplier() {
    uint16_t multiplier = 1000;  // 100%

    // Flex fuel: 1.0-1.3x for E85
    if (features.flex_fuel) {
        multiplier = (multiplier * FlexFuel::getStatus()->fuel_multiplier) / 1000;
        // multiplier = (1000 * 1300) / 1000 = 1300 (130%)
    }

    return multiplier;  // 1300 (130%)
}
```

---

## Startup Self-Test

The system manager runs a comprehensive self-test on startup:

```cpp
SystemManager::SelfTestResults runStartupSelfTest() {
    // Test 1: Sensors
    - Throttle position sensor
    - Coolant temp sensor
    - Oil pressure sensor
    - Battery voltage
    - Knock sensor (if enabled)
    - Flex fuel sensor (if enabled)
    - Boost sensor (if enabled)

    // Test 2: Actuators
    - OMP solenoid
    - IAC valve
    - Wastegate solenoid (if turbo)
    - Water/meth pump (if installed)
    - Ignition drivers

    // Test 3: Memory
    - EEPROM checksum
    - RAM test

    // Test 4: CAN Bus
    - CAN controller init
    - CAN message transmission
    - CAN message reception

    // Test 5: Feature-specific
    - Flex fuel sensor frequency
    - Water/meth tank level
    - Knock sensor baseline

    return results;  // PASS or FAIL
}
```

**If self-test fails**: System enters **SAFE MODE**
- Advanced features disabled
- Conservative base tune
- Can still drive (limp mode)
- Faults logged for diagnostics

---

## Operating Modes

### MODE_STARTUP
- Initial mode during boot
- Self-test running
- Features not yet active

### MODE_NORMAL
- Normal operation
- All enabled features active
- Safety interlocks active
- Full power available

### MODE_SAFE
- Triggered by non-critical faults
- Advanced features disabled
- Conservative tune
- Reduced power
- Can still drive

**Enters when:**
- Coolant overtemp (>115°C)
- Self-test fails
- Sensor faults
- Manual activation

### MODE_DIAGNOSTIC
- Verbose logging enabled
- Status printed every 5 seconds
- All features active
- For troubleshooting

**Example output:**
```
======================================
  SYSTEM STATUS
======================================
Mode: DIAGNOSTIC
Health: OK
Faults: 0

Adjustments:
  Timing: +8°
  Boost Limit: 17.5 PSI
  Fuel Mult: 130.0%

Features:
  Apex Seals: 95%
  Knock Events: 0
  WMI Tank: 85%
======================================
```

### MODE_EMERGENCY
- Triggered by critical faults
- IDLE ONLY (2000 RPM limit)
- 90% power reduction
- Must pull over immediately

**Enters when:**
- Oil pressure <10 PSI
- Critical sensor failure
- Manual emergency stop

---

## Feature Integration Examples

### Turbocharged RX8 with Full Features

```cpp
SystemManager::FeatureFlags features = {
    .rotary_engine = true,      // OMP + dual ignition
    .flex_fuel = true,          // E85 support
    .idle_control = true,       // Stable idle
    .decel_fuel_cut = true,     // Fuel economy
    .boost_control = true,      // 15 PSI target
    .launch_control = true,     // 4000 RPM launch
    .knock_detection = true,    // Critical for turbo!
    .water_meth = true,         // 17+ PSI safe
    .data_logging = true        // Log everything
};

// Expected performance:
// - 15 PSI base boost
// - 17.5 PSI with water/meth
// - 28° timing advance (E85 + water/meth)
// - +40% power over stock
// - Safe for apex seals (cooler temps)
```

### Naturally Aspirated RX8 with E85

```cpp
SystemManager::FeatureFlags features = {
    .rotary_engine = true,      // OMP + dual ignition
    .flex_fuel = true,          // E85 support
    .idle_control = true,       // Stable idle
    .decel_fuel_cut = true,     // Fuel economy
    .boost_control = false,     // NA engine
    .launch_control = false,    // Not needed
    .knock_detection = true,    // Always good to have
    .water_meth = false,        // Not needed for NA
    .data_logging = true        // Track everything
};

// Expected performance:
// - 25° timing advance (E85)
// - +5-8% power from E85
// - Better throttle response
// - Cleaner combustion
```

### Stock RX8 (Reliability Focus)

```cpp
SystemManager::FeatureFlags features = {
    .rotary_engine = true,      // OMP CRITICAL!
    .flex_fuel = false,         // Pump gas only
    .idle_control = true,       // Better drivability
    .decel_fuel_cut = true,     // Fuel economy
    .boost_control = false,     // NA engine
    .launch_control = false,    // Not needed
    .knock_detection = true,    // Safety monitoring
    .water_meth = false,        // Not needed
    .data_logging = true        // Monitor apex seal health
};

// Focus:
// - OMP ensures apex seal lubrication
// - Apex seal health monitoring
// - Operating hours tracking
// - Conservative tune for reliability
```

---

## Diagnostics

### View System Status

```cpp
// In main loop (diagnostic mode)
SystemManager::printStatus();
```

**Output:**
```
======================================
  SYSTEM STATUS
======================================
Mode: NORMAL
Health: OK
Faults: 0

Adjustments:
  Timing: +8°
  Boost Limit: 17.5 PSI
  Fuel Mult: 130.0%

Features:
  Apex Seals: 95%
  Knock Events: 2
  WMI Tank: 85%
======================================
```

### Check for Faults

```cpp
const SystemManager::SystemHealth* health = SystemManager::getHealth();

if (!health->all_ok) {
    Serial.printf("Faults: %d\n", health->fault_count);
    for (uint8_t i = 0; i < health->fault_count; i++) {
        Serial.printf("  %d: %s\n", i + 1, health->fault_codes[i]);
    }
}
```

### Clear Faults

```cpp
// Clear specific fault
SystemManager::clearFault("KNOCK_DETECTED");

// Clear all faults
SystemManager::clearAllFaults();
```

---

## Safety Checklist

Before driving with integrated system:

**Hardware:**
- [ ] All sensors installed and wired
- [ ] All actuators tested (OMP, IAC, wastegate, water/meth)
- [ ] CAN bus communicating
- [ ] Wideband O2 sensor installed
- [ ] All grounds secure

**Software:**
- [ ] Startup self-test passes
- [ ] Features configured correctly
- [ ] Data logging enabled
- [ ] Failsafes tested (simulate faults)
- [ ] Conservative initial tune

**Testing:**
- [ ] Bench test all features
- [ ] Idle test (10 minutes)
- [ ] Low-speed test (<50 km/h)
- [ ] Highway test (100 km/h)
- [ ] Gradual boost increase (if turbo)
- [ ] Monitor knock events
- [ ] Verify failsafes activate

**Monitoring:**
- [ ] Watch coolant temp
- [ ] Watch oil pressure
- [ ] Monitor knock count
- [ ] Check water/meth tank level
- [ ] Review data logs after each drive

---

## Troubleshooting

### System enters SAFE MODE unexpectedly

**Check:**
1. Coolant temperature (should be 80-100°C)
2. Sensor readings (all in valid range?)
3. Fault codes (`SystemManager::getHealth()`)
4. Self-test results

**Common causes:**
- Coolant overtemp
- Sensor disconnected
- Low battery voltage
- CAN bus error

### EMERGENCY MODE activated

**Causes:**
- Low oil pressure (<10 PSI)
- Critical sensor failure

**Action:**
1. Pull over immediately
2. Check oil level
3. Check oil pressure sensor
4. Do not drive until fixed

### Features not working

**Check:**
1. Feature enabled in FeatureFlags?
2. Hardware installed?
3. Self-test passed for that feature?
4. Mode = NORMAL (not SAFE)?

**Example:**
```cpp
const SystemManager::FeatureFlags* flags = SystemManager::getFeatureFlags();
if (!flags->water_meth) {
    Serial.println("Water/meth feature disabled");
}
```

---

## Best Practices

1. **Always run startup self-test**
2. **Enable data logging** for all test drives
3. **Monitor faults** after each drive
4. **Test failsafes** before relying on them
5. **Start conservative**, add features gradually
6. **Use diagnostic mode** when tuning
7. **Review logs** to understand system behavior

---

**Last Updated**: 2025-11-16
**Requires**: Phase 5++ Advanced ECU Firmware
**Safety Critical**: YES - Follow all procedures
