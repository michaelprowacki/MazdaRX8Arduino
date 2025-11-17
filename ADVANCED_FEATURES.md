# Advanced Features - Rotary Engine (13B-MSP)

**Advanced engine management features for Mazda RX8 13B-MSP (RENESIS) rotary engine**

⚠️ **WARNING**: These are ADVANCED features requiring extensive testing and tuning. Use at your own risk!

---

## Table of Contents

1. [Rotary Engine Management](#rotary-engine-management)
2. [Oil Metering Pump (OMP) Control](#oil-metering-pump-control)
3. [Dual Ignition System](#dual-ignition-system)
4. [Turbo/Boost Control](#turboboost-control)
5. [Launch Control](#launch-control)
6. [Knock Detection & Protection](#knock-detection--protection)
7. [Apex Seal Health Monitoring](#apex-seal-health-monitoring)
8. [Data Logging (Black Box)](#data-logging-black-box)
9. [Integration Guide](#integration-guide)
10. [Tuning Guide](#tuning-guide)

---

## Rotary Engine Management

### Overview

The rotary engine module (`rotary_engine.h/cpp`) provides comprehensive management specifically tailored for the Mazda 13B-MSP (RENESIS) rotary engine.

### Key Features

- **Oil Metering Pump (OMP)** control for apex seal lubrication
- **Dual ignition** system (leading/trailing spark)
- **Apex seal** health monitoring
- **Compression** monitoring per rotor
- **Oil consumption** tracking
- **Coolant seal** integrity checking
- **Operating hours** tracking

### Usage

```cpp
#include "advanced/rotary_engine.h"

void setup() {
    RotaryEngine::init();

    // Configure OMP
    RotaryEngine::OMPConfig omp = {
        .idle_rate = 30,      // 30% at idle
        .cruise_rate = 50,    // 50% at cruise
        .wot_rate = 100,      // 100% at WOT
        .rpm_threshold = 5000,
        .premix_mode = false  // OMP enabled
    };
    RotaryEngine::configureOMP(omp);
}

void loop() {
    uint16_t rpm = 3000;
    uint8_t throttle = 50;
    int16_t coolant_temp = 900;  // 90.0°C

    RotaryEngine::update(rpm, throttle, coolant_temp);
}
```

### Health Monitoring

```cpp
const RotaryEngine::HealthStatus* health = RotaryEngine::getHealthStatus();

Serial.printf("Apex seal condition: %d%%\n", health->apex_seal_condition);
Serial.printf("Rotor 1 compression: %d%%\n", health->compression_rotor1);
Serial.printf("Rotor 2 compression: %d%%\n", health->compression_rotor2);
Serial.printf("Oil consumption: %d ml/100km\n", health->oil_consumption_ml_100km);
Serial.printf("Operating hours: %lu\n", health->hours_since_rebuild / 100);

if (health->apex_seal_condition < 70) {
    Serial.println("WARNING: Apex seals may need replacement!");
}
```

---

## Oil Metering Pump Control

### Why OMP is Critical for Rotaries

Rotary engines use apex seals (similar to piston rings) that **require constant lubrication**. The factory OMP injects 2-stroke oil directly into the combustion chamber to lubricate these seals.

**Without proper OMP control**: Apex seals will fail within hours!

### OMP Control Algorithm

The OMP duty cycle is calculated based on:
1. **Throttle position** (idle/cruise/WOT)
2. **Engine RPM** (higher RPM = more oil)
3. **Engine temperature** (cold engine needs more oil)

#### Formula

```
Base Rate = interpolate(throttle, idle_rate, cruise_rate, wot_rate)
RPM Bonus = (RPM - threshold) * 2% per 100 RPM
Cold Bonus = +15% if temp < 60°C
Final Duty = Base + RPM Bonus + Cold Bonus (clamped to 0-100%)
```

### Configuration

```cpp
RotaryEngine::OMPConfig omp_config = {
    .idle_rate = 30,        // 30% injection at idle
    .cruise_rate = 50,      // 50% at cruise (most efficient)
    .wot_rate = 100,        // 100% at wide-open throttle
    .rpm_threshold = 5000,  // Increase injection above 5000 RPM
    .premix_mode = false    // Set true if using premix fuel
};
RotaryEngine::configureOMP(omp_config);
```

### Premix Mode

Some users premix 2-stroke oil in their fuel (typically 1:100 ratio). If using premix:

```cpp
omp_config.premix_mode = true;  // Disables OMP
```

⚠️ **WARNING**: Never run a rotary without either OMP or premix! Apex seals WILL fail!

### Hardware Connection

```
OMP Solenoid PWM Output:
- Connect PWM pin to OMP solenoid driver
- Typical frequency: 100 Hz
- Duty cycle: 0-100%
```

---

## Dual Ignition System

### Rotary Ignition Architecture

The 13B-MSP uses **two spark plugs per rotor**:
- **Leading spark** (primary): Fires first, initiates combustion
- **Trailing spark** (secondary): Fires later, completes combustion

This dual ignition ensures complete fuel burn across the large combustion chamber.

### Timing Strategy

#### Leading Spark Timing
- **Idle**: 10° BTDC
- **Cruise**: 15° BTDC
- **WOT**: 20° BTDC
- **High RPM bonus**: +1° per 100 RPM above 3000 RPM (max 35° BTDC)

#### Trailing Spark Timing
- **Typically 10° retarded** from leading spark
- **Split mode**: Increases retard at high RPM for better flame propagation

### Knock Protection

When knock is detected:
- **Immediate retard**: -2° per knock event
- **Maximum retard**: 10° (configurable)
- **Recovery**: Gradual return at 1°/second

### Configuration

```cpp
RotaryEngine::IgnitionConfig ignition = {
    .leading_advance_idle = 10,     // 10° BTDC at idle
    .leading_advance_cruise = 15,   // 15° BTDC at cruise
    .leading_advance_wot = 20,      // 20° BTDC at WOT
    .trailing_offset = 10,          // 10° retarded from leading
    .split_mode = true              // Enable split timing
};
RotaryEngine::configureIgnition(ignition);
```

### Usage

```cpp
uint16_t rpm = 6000;
uint8_t throttle = 75;
bool knock_detected = false;

uint8_t leading = RotaryEngine::calculateLeadingTiming(rpm, throttle, knock_detected);
uint8_t trailing = RotaryEngine::calculateTrailingTiming(leading, rpm);

Serial.printf("Leading: %d° BTDC, Trailing: %d° BTDC\n", leading, trailing);
// Output leading to ignition driver 1
// Output trailing to ignition driver 2
```

---

## Turbo/Boost Control

### Overview

Electronic wastegate control for turbocharged 13B-MSP engines. Includes:
- **Boost-by-gear** functionality
- **Closed-loop PID control**
- **Anti-lag system** (ALS)
- **Overboost protection**
- **Boost ramping**

### Safety Limits

**Stock 13B-MSP (naturally aspirated)**:
- **NOT designed for boost!**
- Maximum safe boost: **~7-8 PSI** (with proper tuning)
- Compression ratio: 10:1 (high for turbo!)

**Recommended**:
- Lower compression to 9:1 or less for boost >8 PSI
- Upgrade fuel system
- Tune AFR conservatively (rich)

### Configuration

```cpp
#include "advanced/boost_control.h"

BoostControl::BoostConfig boost = {
    .target_boost_psi = {50, 70, 90, 100, 100, 90},  // Per gear (PSI * 10)
    .max_boost_psi = 120,           // 12 PSI absolute max
    .boost_ramp_rate = 50,          // Medium ramp rate
    .antilag_enabled = false,       // VERY hard on engine!
    .wastegate_preload = 30,        // 30% wastegate preload
    .spool_rpm_threshold = 2500     // Start control at 2500 RPM
};
BoostControl::configure(boost);
BoostControl::init();
```

### Usage

```cpp
void loop() {
    uint16_t rpm = 5000;
    uint8_t throttle = 100;
    uint16_t current_boost = 85;  // 8.5 PSI
    uint8_t gear = 3;

    uint8_t wastegate_duty = BoostControl::update(rpm, throttle, current_boost, gear);

    // Output wastegate_duty to wastegate solenoid PWM pin
    analogWrite(WASTEGATE_PIN, wastegate_duty * 255 / 100);
}
```

### Boost-by-Gear

Different boost targets for each gear prevent wheelspin and drivetrain damage:

| Gear | Target Boost | Rationale |
|------|--------------|-----------|
| 1st  | 5 PSI        | Prevent wheelspin |
| 2nd  | 7 PSI        | Limited traction |
| 3rd  | 9 PSI        | Good traction |
| 4th  | 10 PSI       | Maximum boost |
| 5th  | 10 PSI       | Highway pulls |
| 6th  | 9 PSI        | Overdrive gear |

```cpp
BoostControl::setGearBoost(3, 90);  // Set gear 3 to 9.0 PSI
```

### Anti-Lag System (ALS)

⚠️ **EXTREME WARNING**: Anti-lag is VERY hard on the engine and exhaust!

**What it does**:
- Retards ignition 20-30° on throttle lift-off
- Keeps injectors firing
- Ignites fuel in exhaust manifold/turbo
- Keeps turbo spooled during shifts

**Damage risks**:
- Exhaust manifold cracking from extreme heat
- Catalytic converter destruction
- Turbo bearing damage
- Excessive carbon buildup

**Only use on**:
- Track/race applications
- Dedicated race engines
- When you accept the maintenance cost

```cpp
BoostControl::setAntiLagEnabled(true);  // Enable at your own risk!
```

---

## Launch Control

### Overview

2-step launch control for optimal acceleration from standstill.

### Features

- RPM limiting at launch RPM (e.g., 4000 RPM)
- Ignition cut or retard
- Boost building at standstill
- Clutch switch integration
- Automatic deactivation above speed threshold

### How It Works

1. **Arm**: Hold throttle wide open at standstill with clutch in
2. **Build boost**: Turbo spools while RPM limited
3. **Launch**: Release clutch → full power at optimal RPM
4. **Deactivate**: Above 10 km/h, switches to main rev limiter

### Configuration

```cpp
#include "advanced/launch_control.h"

LaunchControl::LaunchConfig launch = {
    .launch_rpm = 4000,         // Launch at 4000 RPM
    .main_rpm_limit = 9000,     // Main redline
    .ignition_cut_mode = 1,     // 0=soft, 1=hard, 2=retard
    .launch_retard = 15,        // 15° retard during launch
    .boost_build = true,        // Build boost at standstill
    .speed_threshold = 100,     // Deactivate above 10 km/h
    .require_clutch = true      // Require clutch for safety
};
LaunchControl::configure(launch);
LaunchControl::setEnabled(true);
```

### Usage

```cpp
void loop() {
    uint16_t rpm = 4200;
    uint8_t throttle = 100;
    uint16_t speed = 0;  // km/h * 10
    bool clutch_pressed = digitalRead(CLUTCH_SWITCH_PIN);

    bool limiting = LaunchControl::update(rpm, throttle, speed, clutch_pressed);

    if (limiting) {
        // Cut ignition or retard timing
        uint8_t action = LaunchControl::applyRPMLimiting(rpm);
        if (action == 1) {
            cutIgnition();  // Hard cut
        } else if (action == 2) {
            retardIgnition(launch.launch_retard);
        }
    }
}
```

### Safety Features

- **Speed threshold**: Auto-disables if moving >10 km/h
- **Clutch requirement**: Won't activate without clutch (if configured)
- **Manual disarm**: Can be disarmed anytime

---

## Knock Detection & Protection

### Overview

Protects engine from detonation (knock) by detecting and retarding timing.

### How Knock Damages Rotaries

Knock (detonation) in rotaries can:
- **Crack apex seals** (catastrophic!)
- **Damage rotor housings**
- **Burn holes in rotors**
- Rotaries are **more knock-sensitive** than piston engines!

### Detection Methods

1. **Analog knock sensor** (0-5V piezo sensor)
2. **Digital knock controller** (pre-processed signal)
3. **Accelerometer** (vibration-based)

### Frequency Characteristics

Rotary knock typically occurs at:
- **6-8 kHz** (primary knock frequency)
- ±2 kHz window for detection

### Configuration

```cpp
#include "advanced/knock_detection.h"

KnockDetection::KnockConfig knock_cfg = {
    .sensor_type = KnockDetection::SENSOR_ANALOG,
    .sensitivity = 50,          // Medium sensitivity
    .knock_frequency = 7000,    // 7 kHz target
    .frequency_window = 2000,   // ±2 kHz
    .max_retard = 10,           // Max 10° retard
    .retard_step = 2,           // 2° per knock event
    .recovery_rate = 1,         // 1°/second recovery
    .enable_protection = true
};
KnockDetection::configure(knock_cfg);
KnockDetection::init();
```

### Usage

```cpp
void loop() {
    uint16_t rpm = 7000;
    uint8_t throttle = 100;
    uint16_t boost = 100;  // 10 PSI

    bool knock = KnockDetection::update(rpm, throttle, boost);

    if (knock) {
        const auto* status = KnockDetection::getStatus();
        Serial.printf("KNOCK! Retarding %d degrees\n", status->current_retard);

        // Apply retard to ignition timing
        applyTimingRetard(status->current_retard);
    }
}
```

### Knock Event Logging

```cpp
const auto* status = KnockDetection::getStatus();
Serial.printf("Total knock events: %lu\n", status->knock_count_total);
Serial.printf("Session knock events: %lu\n", status->knock_count_session);

// View knock log
const KnockDetection::KnockEvent* log = KnockDetection::getEventLog(20);
for (int i = 0; i < 20; i++) {
    Serial.printf("Knock at T=%lu: RPM=%u, Throttle=%u%%, Boost=%.1f PSI\n",
                 log[i].timestamp, log[i].rpm, log[i].throttle,
                 log[i].boost / 10.0f);
}
```

### Tuning Tips

- **Start conservative**: High sensitivity, low max retard
- **Calibrate in controlled conditions**: Dyno or steady-state
- **Monitor false positives**: Mechanical noise can trigger detection
- **Use frequency filtering**: Reduces false triggers

---

## Apex Seal Health Monitoring

### Overview

Monitors apex seal condition to predict maintenance needs.

### What Are Apex Seals?

Apex seals are the **most critical** wear component in rotary engines:
- Equivalent to piston rings
- Seal combustion chamber as rotor spins
- Wear over time (typical life: 80,000-100,000 miles)

### Health Indicators

1. **Compression** (per rotor)
   - 100% = new engine
   - <90% = noticeable power loss
   - <70% = rebuild recommended

2. **Oil consumption**
   - Normal: 300-700 ml/100km
   - Warning: >1000 ml/100km
   - Critical: >1500 ml/100km

3. **Operating hours**
   - Typical rebuild interval: 2000-2500 hours
   - Adjusted based on health score

### Usage

```cpp
// Update oil consumption after filling
uint32_t distance_since_fill = 500;  // km
uint16_t oil_added = 600;  // ml

uint16_t consumption = RotaryEngine::estimateOilConsumption(
    distance_since_fill, oil_added
);
Serial.printf("Oil consumption: %d ml/100km\n", consumption);

// Get health status
const auto* health = RotaryEngine::getHealthStatus();

if (health->apex_seal_condition < 70) {
    Serial.println("WARNING: Apex seals degraded!");
    Serial.printf("Recommended rebuild: %lu hours\n",
                 RotaryEngine::getRecommendedRebuildHours());
}

if (!health->coolant_seal_ok) {
    Serial.println("CRITICAL: Coolant seal failure!");
    Serial.println("Coolant may be entering combustion chamber");
}
```

### Coolant Seal Monitoring

Coolant seal failure symptoms:
- Coolant loss with no external leak
- White smoke from exhaust (steam)
- Coolant in oil (milky appearance)
- Rapid temperature fluctuations

---

## Data Logging (Black Box)

### Overview

High-speed data logging for performance analysis and diagnostics.

### Features

- **Sample rates**: 1-100 Hz
- **Storage**: SD card (CSV format)
- **Circular buffer**: Last 10 seconds in RAM (crash data)
- **Trigger modes**: Continuous, triggered, or circular buffer only
- **Data export**: CSV format compatible with MegaLogViewer, RaceCapture

### Logged Parameters

| Parameter | Units | Description |
|-----------|-------|-------------|
| timestamp | ms | Milliseconds since session start |
| rpm | RPM | Engine RPM |
| speed | km/h * 10 | Vehicle speed |
| throttle | % | Throttle position |
| boost | PSI * 10 | Boost pressure |
| coolant_temp | °C * 10 | Coolant temperature |
| battery_voltage | V * 100 | Battery voltage |
| oil_pressure | PSI | Oil pressure |
| omp_duty | % | OMP duty cycle |
| leading_timing | degrees | Leading spark timing |
| trailing_timing | degrees | Trailing spark timing |
| wastegate_duty | % | Wastegate duty |
| knock_retard | degrees | Knock retard amount |
| lambda | lambda * 1000 | AFR (air/fuel ratio) |
| gear | 0-6 | Current gear |
| flags | bitfield | Status flags |

### Configuration

```cpp
#include "advanced/data_logger.h"

DataLogger::LoggerConfig log_cfg = {
    .mode = DataLogger::MODE_CONTINUOUS,
    .sample_rate_hz = 10,       // 10 Hz (10 samples/second)
    .max_log_size_mb = 100,     // 100 MB max file size
    .enable_sd_card = true,     // Log to SD card
    .enable_serial = false,     // Don't spam serial
    .triggers = {
        .trigger_on_knock = true,
        .trigger_on_overboost = true,
        .trigger_on_high_rpm = true,
        .trigger_on_launch = true,
        .rpm_threshold = 7000,
        .boost_threshold = 100  // 10 PSI
    }
};
DataLogger::configure(log_cfg);
DataLogger::init();
```

### Usage

```cpp
// Start logging session
DataLogger::startSession(DataLogger::MODE_CONTINUOUS);

// In main loop, log data at configured rate
void loop() {
    static uint32_t last_log = 0;
    if (millis() - last_log > 100) {  // 10 Hz
        DataLogger::LogEntry entry = {
            .timestamp = millis(),
            .rpm = current_rpm,
            .speed_kmh = current_speed,
            .throttle = current_throttle,
            // ... fill all fields
        };

        DataLogger::logSample(entry);
        last_log = millis();
    }

    DataLogger::update();  // Handle buffering/writing
}

// Stop session when done
DataLogger::stopSession();
```

### Trigger Mode

Only log when interesting events occur:

```cpp
DataLogger::LoggerConfig log_cfg = {
    .mode = DataLogger::MODE_TRIGGERED,
    .triggers = {
        .trigger_on_knock = true,      // Log when knock detected
        .trigger_on_overboost = true,  // Log when overboost
        .trigger_on_high_rpm = true,   // Log above 7000 RPM
        .trigger_on_launch = true      // Log during launch
    }
};
```

### Data Analysis

Log files are CSV format, compatible with:
- **MegaLogViewer** (recommended)
- **RaceCapture**
- **Excel / Google Sheets**
- **Python pandas**

Example log file:
```csv
timestamp,rpm,speed,throttle,boost,coolant,voltage,oil_psi,...
0,800,0,5,0,850,1380,45,...
100,850,0,5,0,852,1379,45,...
200,900,12,15,5,855,1378,46,...
```

---

## Integration Guide

### Pin Assignments

Define in `firmware/automotive_ecu/src/hal/<platform>/pin_mapping.h`:

```cpp
// OMP control
#define OMP_PWM_PIN          9   // OMP solenoid PWM

// Ignition outputs
#define IGNITION_LEADING_1   10  // Rotor 1 leading spark
#define IGNITION_TRAILING_1  11  // Rotor 1 trailing spark
#define IGNITION_LEADING_2   12  // Rotor 2 leading spark
#define IGNITION_TRAILING_2  13  // Rotor 2 trailing spark

// Boost control
#define WASTEGATE_PWM_PIN    14  // Electronic wastegate solenoid

// Knock sensor
#define KNOCK_SENSOR_PIN     A0  // Analog knock sensor input

// Launch control
#define CLUTCH_SWITCH_PIN    22  // Clutch pedal switch (digital input)

// Data logging
#define SD_CARD_CS_PIN       53  // SD card chip select
```

### Main Loop Integration

```cpp
#include "advanced/rotary_engine.h"
#include "advanced/boost_control.h"
#include "advanced/launch_control.h"
#include "advanced/knock_detection.h"
#include "advanced/data_logger.h"

void setup() {
    // Initialize all systems
    RotaryEngine::init();
    BoostControl::init();
    LaunchControl::init();
    KnockDetection::init();
    DataLogger::init();

    // Start logging
    DataLogger::startSession(DataLogger::MODE_CONTINUOUS);
}

void loop() {
    // Read sensors
    uint16_t rpm = readRPM();
    uint8_t throttle = readThrottle();
    uint16_t boost = readBoostSensor();
    int16_t coolant_temp = readCoolantTemp();
    uint16_t speed = readVehicleSpeed();
    bool clutch = digitalRead(CLUTCH_SWITCH_PIN);

    // Update rotary engine control
    RotaryEngine::update(rpm, throttle, coolant_temp);
    uint8_t omp_duty = RotaryEngine::controlOMP(rpm, throttle);
    analogWrite(OMP_PWM_PIN, omp_duty * 255 / 100);

    // Update boost control
    uint8_t gear = getCurrentGear();
    uint8_t wastegate_duty = BoostControl::update(rpm, throttle, boost, gear);
    analogWrite(WASTEGATE_PWM_PIN, wastegate_duty * 255 / 100);

    // Update knock detection
    bool knock = KnockDetection::update(rpm, throttle, boost);

    // Update ignition timing
    uint8_t leading = RotaryEngine::calculateLeadingTiming(rpm, throttle, knock);
    uint8_t trailing = RotaryEngine::calculateTrailingTiming(leading, rpm);
    setIgnitionTiming(leading, trailing);

    // Update launch control
    bool launch_active = LaunchControl::update(rpm, throttle, speed, clutch);
    if (launch_active) {
        // Apply launch RPM limiting
    }

    // Log data
    DataLogger::LogEntry entry = {
        .timestamp = millis(),
        .rpm = rpm,
        .speed_kmh = speed,
        .throttle = throttle,
        .boost_psi = boost,
        .coolant_temp = coolant_temp,
        // ... fill remaining fields
    };
    DataLogger::logSample(entry);
    DataLogger::update();
}
```

---

## Tuning Guide

### General Principles

1. **Start conservative**: Safe values, then optimize
2. **Change one thing at a time**: Isolate effects
3. **Log everything**: Data-driven tuning
4. **Test incrementally**: Small steps
5. **Monitor closely**: Watch for knock, overheating

### OMP Tuning

**Symptoms of too little oil**:
- Loss of compression
- Hard starting when warm
- Apex seal chatter (rattling noise)
- Blue smoke from exhaust

**Symptoms of too much oil**:
- Excessive oil consumption
- Carbon buildup on plugs
- Black smoke from exhaust

**Tuning procedure**:
1. Start with factory rates (30/50/100%)
2. Monitor oil consumption over 500 km
3. Adjust based on results:
   - High consumption (>1000 ml/100km) → reduce rates
   - Low consumption (<300 ml/100km) → increase rates
4. Compression test every 10,000 km to verify

### Ignition Timing Tuning

**Start values** (naturally aspirated):
- Idle: 10° BTDC
- Cruise: 15° BTDC
- WOT: 20° BTDC

**Turbo (boost >5 PSI)**:
- Reduce by 3-5° across the board
- Monitor knock carefully
- Rich AFR (11.5-12:1) for safety

**Tuning on dyno**:
1. Set target RPM (e.g., 5000)
2. Hold throttle position
3. Advance timing 1° at a time
4. Monitor power and knock
5. Stop at first sign of knock or power plateau
6. Back off 2° for safety margin

### Boost Control Tuning

**PID tuning** (advanced):
- **P (Proportional)**: Increase for faster response, decrease for stability
- **I (Integral)**: Increase to eliminate steady-state error
- **D (Derivative)**: Usually keep low for boost control

**Start values**:
- Kp = 0.5
- Ki = 0.1
- Kd = 0.05

**Symptoms**:
- Oscillating boost: Reduce P, increase D
- Slow to target: Increase P
- Overshoots target: Reduce P, increase D
- Never reaches target: Increase I

### Knock Detection Tuning

**False positive detection**:
1. Log background noise level
2. Increase threshold if too many false triggers
3. Verify knock frequency (6-8 kHz for rotary)

**False negative (missing real knock)**:
1. Decrease threshold
2. Increase sensitivity
3. Verify sensor placement (near exhaust port)

**Sensor placement**:
- **Best**: Near exhaust port on rotor housing
- **Avoid**: Near mechanical components (alternator, water pump)

---

## Safety Checklist

Before using advanced features:

### Hardware
- [ ] Proper knock sensor installed and calibrated
- [ ] OMP solenoid wired and tested
- [ ] Wastegate solenoid wired (if turbo)
- [ ] All sensors reading correctly
- [ ] Ignition drivers rated for duty cycle
- [ ] Wiring protected from heat and vibration

### Software
- [ ] All features tested on bench (not in vehicle)
- [ ] Conservative initial settings
- [ ] Data logging enabled
- [ ] Safety limits configured (max boost, max RPM)
- [ ] Knock protection enabled
- [ ] Failsafe behavior defined

### Testing
- [ ] Idle test (10 minutes)
- [ ] Low-speed driving (< 50 km/h, 30 minutes)
- [ ] Highway speed (100 km/h, 1 hour)
- [ ] Full throttle test (controlled environment)
- [ ] Data logs reviewed for anomalies
- [ ] No knock events detected
- [ ] All sensors stable

### Maintenance
- [ ] Compression test before and after
- [ ] Oil consumption monitored
- [ ] Spark plugs inspected (every 500 km initially)
- [ ] Boost leaks checked (if turbo)
- [ ] Data logs archived

---

## Troubleshooting

### OMP Issues

**Symptom**: Excessive oil consumption (>1500 ml/100km)
- **Cause**: OMP duty too high
- **Fix**: Reduce idle/cruise/wot rates by 10-20%

**Symptom**: Low compression, hard starting
- **Cause**: Insufficient oil injection
- **Fix**: Increase OMP rates, check solenoid operation

**Symptom**: Carbon buildup on spark plugs
- **Cause**: Too much oil at idle
- **Fix**: Reduce idle_rate

### Ignition Issues

**Symptom**: Misfires at high RPM
- **Cause**: Ignition timing too advanced
- **Fix**: Reduce WOT advance

**Symptom**: Poor idle quality
- **Cause**: Ignition timing too retarded at idle
- **Fix**: Increase idle advance to 10-12°

**Symptom**: Excessive knock events
- **Cause**: Timing too aggressive
- **Fix**: Reduce advance, check fuel octane

### Boost Control Issues

**Symptom**: Boost spikes and drops
- **Cause**: PID tuning too aggressive
- **Fix**: Reduce Kp, increase Kd

**Symptom**: Slow to reach target boost
- **Cause**: Wastegate preload too high or PID too conservative
- **Fix**: Reduce preload, increase Kp

**Symptom**: Overboost despite wastegate open
- **Cause**: Wastegate stuck closed or undersized
- **Fix**: Check wastegate operation, may need larger wastegate

### Data Logging Issues

**Symptom**: SD card write errors
- **Cause**: Card too slow or corrupted
- **Fix**: Use Class 10 or better SD card, format FAT32

**Symptom**: Missing samples in log
- **Cause**: Sample rate too high for SD card speed
- **Fix**: Reduce sample rate or use faster card

---

## Performance Gains

Expected improvements with advanced features:

| Feature | Power Gain | Reliability Impact | Complexity |
|---------|------------|-------------------|------------|
| OMP Control | 0% | +50% (prevents failure) | Low |
| Dual Ignition | +2-5% | +10% (complete burn) | Medium |
| Boost Control (7 PSI) | +30-40% | -20% (if tuned badly) | High |
| Launch Control | 0% | 0% | Medium |
| Knock Protection | 0% | +30% (prevents damage) | Medium |
| Data Logging | 0% | +20% (enables tuning) | Low |

---

**Last Updated**: 2025-11-16
**Part of Phase 5+ advanced rotary engine features**
**For 13B-MSP (RENESIS) Mazda RX8**
