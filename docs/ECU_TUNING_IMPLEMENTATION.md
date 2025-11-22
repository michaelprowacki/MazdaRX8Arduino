# Full ECU Tuning Implementation Plan

Comprehensive engine control with closed-loop tuning, adaptive learning, and advanced features.

## Current Project Capabilities

### What We Have Now

The RX8 Arduino project currently:
- Emulates CAN bus messages for body systems (dashboard, ABS, power steering)
- Passes throttle pedal signal through to motor controller
- Handles immobilizer handshake
- Controls warning lights, odometer, gauges

### What We Don't Have

- Fuel injection control (timing, pulse width)
- Ignition timing control
- Closed-loop fuel trim (O2 feedback)
- Knock detection and retard
- Adaptive learning
- Boost control

**The throttle passthrough goes to an aftermarket ECU or EV motor controller** - we don't currently control the engine directly.

## Platform Choice: rusEFI

**Recommendation**: Use rusEFI as the engine control platform.

### Why rusEFI Over Speeduino

| Feature | Speeduino | rusEFI | Winner |
|---------|-----------|--------|--------|
| **Processor** | ATmega2560 (16MHz) | STM32 (168MHz+) | rusEFI |
| **Sequential Injection** | 4 cyl max | 8-12 cyl | rusEFI |
| **Knock Control** | External only | Native DSP | rusEFI |
| **CAN Bus** | Add-on | Built-in | rusEFI |
| **Wideband O2** | External | Built-in controller | rusEFI |
| **Map Size** | Limited (AVR RAM) | Large (STM32 RAM) | rusEFI |
| **Drive-by-Wire** | No | Dual DBW | rusEFI |
| **SD Logging** | No | Yes | rusEFI |
| **Cost** | $100-200 | $200-300 | Speeduino |
| **Documentation** | Good | Fair | Speeduino |

**rusEFI is more advanced** and has most features we need built-in.

### rusEFI Hardware Options

**uaEFI121** ($199 assembled):
- 12 injector outputs (sequential 12-cyl)
- 12 ignition outputs
- 6 analog inputs
- 2 VR inputs (crank/cam)
- Built-in wideband controller
- Knock input with DSP
- Dual H-bridge (DBW throttle)
- SD card logging
- CAN bus

**microRusEFI** ($150):
- 4 injector outputs
- 4 ignition outputs
- Smaller footprint
- Good for 4-cyl

## Implementation Architecture

### Two-Board Solution

```
┌─────────────────┐     CAN Bus      ┌─────────────────┐
│   rusEFI        │◄────────────────►│  RX8 Arduino    │
│ (Engine Control)│                  │ (Body Systems)  │
└────────┬────────┘                  └────────┬────────┘
         │                                    │
    ┌────┴────┐                          ┌────┴────┐
    │ Sensors │                          │Dashboard│
    │Injectors│                          │   ABS   │
    │  Coils  │                          │   P/S   │
    └─────────┘                          └─────────┘
```

**rusEFI handles**:
- Fuel injection
- Ignition timing
- Closed-loop AFR
- Knock control
- Boost control
- Idle control
- All engine sensors

**RX8 Arduino handles**:
- Dashboard CAN translation
- ABS/DSC communication
- Power steering
- Immobilizer
- PDM control
- Body electronics

### CAN Gateway Protocol

rusEFI sends engine data via CAN. We translate to RX8 format.

**rusEFI CAN Output** (configurable):
```
0x100: RPM (16-bit), Coolant Temp, TPS
0x101: MAP, IAT, Battery Voltage
0x102: AFR, Fuel Pressure, Oil Pressure
0x103: Knock Retard, Timing Advance
```

**RX8 Arduino Translation**:
```cpp
void translateRusEFI() {
  // Read rusEFI CAN
  if (canID == 0x100) {
    int rusRPM = (buf[0] << 8) | buf[1];
    int rusCLT = buf[2];
    int rusTPS = buf[3];

    // Translate to RX8 format
    engineRPM = rusRPM;
    engTemp = rusCLT;
    throttlePedal = rusTPS;

    // Send RX8 CAN messages
    updatePCM();  // 0x201
    updateMIL();  // 0x420
  }
}
```

---

## Feature Implementation Details

### 1. Closed-Loop Fuel Control

**Hardware (rusEFI built-in)**:
- Wideband O2 controller (LSU 4.9)
- 0-5V analog output or CAN

**Software (rusEFI)**:

```cpp
// Target AFR Table (16x16)
float targetAFR[16][16];  // RPM x Load

// Short-Term Fuel Trim
float calculateSTFT(float currentAFR, float targetAFR) {
  float error = targetAFR - currentAFR;
  float correction = error * STFT_GAIN;  // P-controller
  return constrain(correction, -25.0, 25.0);  // ±25% max
}

// Long-Term Fuel Trim (Learning)
void updateLTFT(int rpmCell, int loadCell, float stft) {
  // Only learn in steady state
  if (abs(stftRate) < 2.0 && engineRunTime > 30) {
    // Exponential moving average
    ltftTable[rpmCell][loadCell] =
      ltftTable[rpmCell][loadCell] * 0.99 + stft * 0.01;

    // Save to EEPROM periodically
    if (millis() - lastLTFTSave > 60000) {
      saveLTFTToEEPROM();
      lastLTFTSave = millis();
    }
  }
}

// Rich Bias (Safety)
float richBias = 3.0;  // Learn 3% rich
// If O2 sensor fails, engine runs slightly rich (safe)
```

**AFR Target Examples**:
- Idle: 14.7 (stoich)
- Cruise: 15.5-16.0 (lean for economy)
- WOT: 11.5-12.5 (rich for power/safety)
- Cold start: 12.0 (rich for stability)

---

### 2. Knock Control

**Hardware (rusEFI built-in)**:
- Knock sensor input with hardware bandpass filter
- STM32 DSP for software processing

**Software (rusEFI)**:

```cpp
// Knock detection parameters
#define KNOCK_FREQ 6800      // Hz (engine-specific)
#define KNOCK_WINDOW_START 10  // degrees ATDC
#define KNOCK_WINDOW_END 70    // degrees ATDC
#define KNOCK_THRESHOLD 2.5    // Volts (calibrate per engine)

// Per-cylinder knock retard
float knockRetard[8] = {0};  // degrees

void processKnock(int cylinder) {
  // Sample knock signal during knock window
  float knockLevel = readKnockSensor();

  // Apply bandpass filter (6.8kHz for typical gasoline)
  float filtered = bandpassFilter(knockLevel, KNOCK_FREQ);

  // Compare to threshold
  if (filtered > KNOCK_THRESHOLD) {
    // Immediate retard
    knockRetard[cylinder] += 3.0;  // 3 degree step
    knockRetard[cylinder] = min(knockRetard[cylinder], 15.0);  // Max 15°

    // Log event
    logKnockEvent(cylinder, filtered);
  } else {
    // Gradual recovery
    knockRetard[cylinder] -= 0.1;  // 0.1° per cycle
    knockRetard[cylinder] = max(knockRetard[cylinder], 0.0);
  }
}

// Long-term knock learning
void updateKnockLearning(int rpmCell, int loadCell, float avgRetard) {
  // If consistently knocking, reduce base timing
  if (avgRetard > 3.0) {
    knockLearnTable[rpmCell][loadCell] -= 0.5;  // Reduce 0.5°
  }
}
```

**Knock Frequency by Engine**:
- 4-cyl gasoline: 6-7 kHz
- V8 gasoline: 5-6 kHz
- Rotary (13B): 8-10 kHz (different combustion)

---

### 3. Adaptive Learning

**Fuel Trim Learning**:

```cpp
struct FuelLearnCell {
  float correction;     // % correction
  int sampleCount;      // Learning confidence
  float variance;       // Data quality
};

FuelLearnCell fuelLearn[16][16];  // RPM x Load

void adaptiveFuelLearn() {
  int rpmCell = mapRPMToCell(engineRPM);
  int loadCell = mapLoadToCell(engineLoad);

  // Calculate required correction
  float currentAFR = readWidebandO2();
  float targetAFR = getTargetAFR(rpmCell, loadCell);
  float error = (targetAFR - currentAFR) / targetAFR * 100.0;

  // Update learning cell
  FuelLearnCell *cell = &fuelLearn[rpmCell][loadCell];

  // Weighted average (more samples = more confidence)
  float weight = 1.0 / (cell->sampleCount + 1);
  cell->correction = cell->correction * (1 - weight) + error * weight;
  cell->sampleCount++;

  // Calculate variance for data quality
  float diff = error - cell->correction;
  cell->variance = cell->variance * 0.9 + (diff * diff) * 0.1;

  // Apply correction to fuel map
  if (cell->sampleCount > 10 && cell->variance < 25.0) {
    // Good data quality, apply correction
    fuelMap[rpmCell][loadCell] *= (1.0 + cell->correction / 100.0);
  }
}
```

**Idle Learning**:

```cpp
struct IdleLearn {
  int targetRPM;
  float basePosition;      // Learned idle valve position
  float loadCorrection;    // A/C, P/S, electrical load
  float tempCorrection;    // Coolant temp compensation
};

void adaptiveIdleLearn() {
  if (!isIdleCondition()) return;

  float rpmError = targetIdleRPM - engineRPM;

  // PID control
  float correction = idlePID.calculate(rpmError);

  // Learn base position when warm and stable
  if (coolantTemp > 80 && abs(rpmError) < 25 && engineRunTime > 120) {
    idleLearn.basePosition =
      idleLearn.basePosition * 0.99 + currentIdlePosition * 0.01;
  }

  // Learn load corrections
  if (acCompressorOn) {
    idleLearn.loadCorrection =
      idleLearn.loadCorrection * 0.95 + correction * 0.05;
  }
}
```

---

### 4. Boost Control

**Hardware**:
- 3-port MAC solenoid ($30)
- MAP sensor (up to 4 bar for turbo)
- Wastegate actuator

**Software (rusEFI)**:

```cpp
// 4D Boost Target Table
// Axes: RPM, TPS, Gear, Coolant Temp
float boostTarget[16][16][6][4];  // Complex but powerful

// Closed-loop boost control
void boostControl() {
  float targetBoost = getBoostTarget(rpm, tps, gear, clt);
  float currentBoost = readMAP() - baroPressure;
  float error = targetBoost - currentBoost;

  // PID controller
  float output = boostPID.calculate(error);

  // PWM to wastegate solenoid (0-100%)
  setWastegateeDuty(output);

  // Boost cut safety
  if (currentBoost > maxBoost) {
    cutFuel();
    logOverboost();
  }
}

// Boost by gear (launch control, traction)
float getBoostByGear(int gear) {
  switch (gear) {
    case 1: return 10.0;   // 10 psi (traction limited)
    case 2: return 15.0;   // 15 psi
    case 3: return 18.0;   // 18 psi
    default: return 20.0;  // Full boost
  }
}

// Boost learning
void boostLearn(int rpmCell, int tpsCell, float error) {
  // Learn wastegate duty cycle for target boost
  if (abs(error) > 1.0 && steadyState) {
    boostLearnTable[rpmCell][tpsCell] += error * 0.1;
  }
}
```

---

### 5. J2534 Interface

**What J2534 Is**:
- SAE standard for vehicle diagnostic communication
- PassThru API for OBD-II, CAN, K-Line, etc.
- Used by factory and aftermarket scan tools

**Implementation**:

J2534 is typically used for diagnostic tools, not ECU firmware. However, we can implement a J2534-compatible interface for:
- ECU flashing and calibration
- Datastream logging
- Diagnostic trouble codes

```cpp
// J2534 Message Structure
struct J2534_MSG {
  uint32_t ProtocolID;    // CAN, ISO15765, etc.
  uint32_t RxStatus;
  uint32_t TxFlags;
  uint32_t Timestamp;
  uint32_t DataSize;
  uint32_t ExtraDataIndex;
  uint8_t Data[4128];
};

// J2534 API Functions (implement in PC software)
long PassThruOpen(void *pName, unsigned long *pDeviceID);
long PassThruClose(unsigned long DeviceID);
long PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID,
                     unsigned long Flags, unsigned long Baudrate,
                     unsigned long *pChannelID);
long PassThruReadMsgs(unsigned long ChannelID, J2534_MSG *pMsg,
                      unsigned long *pNumMsgs, unsigned long Timeout);
long PassThruWriteMsgs(unsigned long ChannelID, J2534_MSG *pMsg,
                       unsigned long *pNumMsgs, unsigned long Timeout);
```

**For rusEFI**: TunerStudio is the configuration software (not J2534-based, uses MegaSquirt protocol).

---

## Hardware Requirements

### For Full ECU Capability

| Component | Specification | Cost | Source |
|-----------|---------------|------|--------|
| **rusEFI uaEFI121** | Main ECU board | $199 | rusEFI shop |
| **Wideband O2** | LSU 4.9 sensor | $80 | Amazon |
| **Knock Sensor** | Bosch 0261231006 | $25 | RockAuto |
| **MAP Sensor** | GM 3-bar (turbo) | $40 | Amazon |
| **IAT Sensor** | GM open element | $15 | Amazon |
| **VR Conditioner** | For crank/cam (built into uaEFI) | - | - |
| **Injector Drivers** | Built into uaEFI (12 outputs) | - | - |
| **Ignition Drivers** | Built into uaEFI (12 outputs) | - | - |
| **DBW Throttle** | If using drive-by-wire | $100 | Junkyard |

**Total**: ~$500 for complete rusEFI-based ECU

### For RX8 Integration

Keep our Arduino project for:
- CAN gateway (translate rusEFI → RX8 body)
- PDM control
- Dashboard, ABS, P/S compatibility
- Immobilizer

**Total System Cost**: ~$700 (rusEFI + Arduino + PDM)

---

## Implementation Phases

### Phase 1: CAN Gateway (1 week)

Add CAN translation between rusEFI and RX8 body systems.

**Tasks**:
1. Define rusEFI CAN protocol (configurable in TunerStudio)
2. Add receive handler in RX8_CANBUS.ino
3. Translate RPM, CLT, TPS to 0x201 format
4. Translate warnings to 0x420 format
5. Forward oil pressure, fuel pressure status

**Files**: `ECU_Module/RX8_CANBUS.ino`

### Phase 2: rusEFI Base Tune (2 weeks)

Configure rusEFI for the swap engine.

**Tasks**:
1. Set trigger pattern (crank/cam)
2. Configure injector characteristics
3. Build base fuel map (VE table)
4. Build base ignition map
5. Configure idle control
6. Test start and run

**Tools**: TunerStudio, wideband O2, timing light

### Phase 3: Closed-Loop Tuning (1 week)

Enable adaptive features.

**Tasks**:
1. Configure wideband O2 controller
2. Set target AFR table
3. Enable short-term fuel trim
4. Enable long-term fuel trim learning
5. Tune PID parameters
6. Road test and log data

### Phase 4: Knock Control (1 week)

Add knock detection and retard.

**Tasks**:
1. Wire knock sensor
2. Configure knock frequency (engine-specific)
3. Set knock window (degrees)
4. Calibrate threshold
5. Configure retard and recovery rates
6. Test under load (dyno recommended)

### Phase 5: Boost Control (if turbo) (1 week)

Add closed-loop boost control.

**Tasks**:
1. Wire wastegate solenoid
2. Configure boost target table
3. Tune PID parameters
4. Set boost cut safety
5. Configure boost by gear (optional)
6. Dyno tune

---

## Software Architecture

### rusEFI Firmware Structure

```
rusEFI/
├── firmware/
│   ├── controllers/
│   │   ├── engine_cycle/
│   │   │   ├── fuel_schedule.cpp
│   │   │   └── spark_schedule.cpp
│   │   ├── actuators/
│   │   │   ├── idle_thread.cpp
│   │   │   └── boost_control.cpp
│   │   ├── sensors/
│   │   │   ├── sensor_reader.cpp
│   │   │   └── knock_logic.cpp
│   │   └── algo/
│   │       ├── fuel_math.cpp
│   │       ├── advance_map.cpp
│   │       └── closed_loop_fuel.cpp
│   ├── hw_layer/
│   │   ├── adc/
│   │   ├── can/
│   │   └── pwm/
│   └── tunerstudio/
│       └── tunerstudio.cpp
```

### RX8 Arduino Integration

```cpp
// Add to ECU_Module/RX8_CANBUS.ino

// rusEFI CAN IDs (configure in TunerStudio)
#define RUSEFI_ENGINE_DATA 0x100
#define RUSEFI_SENSORS 0x101
#define RUSEFI_AFR 0x102
#define RUSEFI_TIMING 0x103

void processRusEFICAN() {
  if (CAN_MSGAVAIL == CAN0.checkReceive()) {
    CAN0.readMsgBuf(&rxId, &len, rxBuf);

    switch (rxId) {
      case RUSEFI_ENGINE_DATA:
        // RPM: bytes 0-1 (16-bit, 0.25 RPM resolution)
        engineRPM = ((rxBuf[0] << 8) | rxBuf[1]) / 4;

        // Coolant Temp: byte 2 (offset -40°C)
        engTemp = rxBuf[2];  // Already in RX8 format

        // TPS: byte 3 (0-100%)
        throttlePedal = rxBuf[3];
        break;

      case RUSEFI_AFR:
        // AFR: bytes 0-1 (0.001 resolution)
        float afr = ((rxBuf[0] << 8) | rxBuf[1]) / 1000.0;

        // Check for lean condition warning
        if (afr > 16.0 && throttlePedal > 50) {
          checkEngineMIL = 1;  // Warn driver
        }
        break;

      case RUSEFI_TIMING:
        // Knock retard: byte 0 (degrees)
        if (rxBuf[0] > 5) {
          // Significant knock - warn driver
          checkEngineMIL = 1;
        }
        break;
    }
  }
}
```

---

## Comparison: DIY vs Commercial

| Feature | Our Solution | Haltech Elite | MoTeC M150 |
|---------|-------------|---------------|------------|
| **Cost** | $700 | $2,500 | $5,000 |
| **Sequential Inj** | 12 cyl | 8 cyl | 12 cyl |
| **Knock Control** | Native DSP | Native DSP | Native DSP |
| **Adaptive Learning** | Yes | Yes | Yes |
| **Boost Control** | 4D closed-loop | 4D closed-loop | 4D closed-loop |
| **Wideband O2** | Built-in | External | External |
| **CAN Bus** | Yes | Yes | Yes |
| **Data Logging** | SD card | Internal | Internal |
| **Tuning Software** | TunerStudio (free) | NSP ($0) | MoTeC ($$$) |
| **Support** | Community | Commercial | Commercial |
| **Open Source** | Yes | No | No |

**Our solution is 72% cheaper than Haltech** with comparable features.

---

## UnlockECU Assessment

**Not directly useful** for our project because:
- It unlocks factory ECUs for diagnostic access
- We're building/using aftermarket ECU (rusEFI)
- Seed-key algorithms not needed for open-source ECU

**Could be useful if**:
- You need to read factory ECU calibration before swap
- You want to extract factory sensor calibration data
- Diagnostic tool development

---

## Recommendations

### For Engine Swap Projects

1. **Use rusEFI** for engine control ($200)
2. **Keep RX8 Arduino** for body systems
3. **Add CAN gateway** to connect them
4. **Total cost**: $700 vs $2,500+ for commercial

### For EV Conversions

1. **Keep current project** (no fuel/ignition needed)
2. **Motor controller** handles propulsion
3. **RX8 Arduino** handles body systems
4. **Add battery management** via CAN

### Next Steps

1. Order rusEFI uaEFI121 board ($199)
2. Implement CAN gateway in RX8_CANBUS.ino
3. Configure rusEFI for swap engine
4. Base tune on dyno
5. Enable adaptive learning
6. Road test and refine

---

## References

- rusEFI Wiki: https://wiki.rusefi.com/
- rusEFI GitHub: https://github.com/rusefi/rusefi
- TunerStudio: https://www.tunerstudio.com/
- Speeduino Wiki: https://wiki.speeduino.com/
- Haltech NSP Software: https://www.haltech.com/

---

## Summary

**Our project is a CAN bus emulator for body systems** - it makes dashboard, ABS, P/S work without factory ECU. For full engine control with tuning features, we need to integrate with rusEFI.

**rusEFI provides**:
- All the closed-loop fuel control, knock detection, adaptive learning, and boost control features requested
- Open source, $200, community supported
- More capable than Speeduino (STM32 vs AVR)

**Our RX8 Arduino provides**:
- CAN gateway to translate rusEFI data to RX8 format
- Body electronics control
- PDM integration

**Together**: Complete open-source ECU solution for RX8 engine swaps at 72% lower cost than commercial alternatives.
