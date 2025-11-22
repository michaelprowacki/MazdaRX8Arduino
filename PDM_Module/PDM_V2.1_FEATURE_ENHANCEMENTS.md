# PDM V2.1 Feature Enhancements

Haltech/MoTeC-inspired features to add professional PDM capabilities.

## Current V2 Capabilities

- 8 channels (2x 21A, 6x 7.5A)
- Auto-retry circuit breakers
- Current sensing
- CAN bus control (simple on/off)
- Fault detection

## Missing Features (Commercial PDMs Have)

### Haltech PD16 Features
- PWM outputs (1kHz) for fan/pump speed control
- 8 inputs (4 analog voltage, 4 digital/pulsed)
- Programmable trigger logic (any input → any output)
- Multi-condition logic (temp AND AC on → fan high speed)
- Real-time configuration via NSP software

### MoTeC PDM30 Features
- 200 logic operations (flash, pulse, timers, hysteresis)
- Temperature compensation for fan curves
- Load shedding (priority-based shutdown on low voltage)
- Data logging via CAN
- Soft start (current ramp for motors)

## Proposed V2.1 Enhancements

### 1. PWM Outputs (Easy - Firmware Only)

**Current**: Digital on/off only
**Upgrade**: Variable PWM duty cycle (0-100%)

**Use Cases**:
- Variable speed radiator fans (cool at 50%, hot at 100%)
- Fuel pump speed control (idle at 50%, WOT at 100%)
- LED dimming (dash lights, puddle lights)

**Implementation**:
```cpp
struct Channel {
  // Add PWM fields
  byte pwmDutyCycle;  // 0-100%
  bool pwmEnabled;    // true = PWM mode, false = digital on/off
};

void updateChannel(Channel *ch) {
  if (ch->enabled && !ch->permanentDisable) {
    if (ch->pwmEnabled) {
      analogWrite(ch->controlPin, map(ch->pwmDutyCycle, 0, 100, 0, 255));
    } else {
      digitalWrite(ch->controlPin, HIGH);
    }
  }
}
```

**CAN Protocol Update**:
```
0x600 (Control):
  Byte 0: Enable bits (CH1-CH8)
  Byte 1: CH1 PWM duty (0-100%)
  Byte 2: CH2 PWM duty (0-100%)
  ... (extend to 0x604 for all 8 channels)
```

**Effort**: 2 hours firmware + 1 hour testing
**Hardware**: No changes needed (Arduino PWM pins)

---

### 2. Digital Inputs (Easy - Firmware + Pin Config)

**Current**: No inputs, only CAN control
**Upgrade**: 4 digital inputs for local triggers

**Use Cases**:
- A/C compressor switch → enable radiator fan
- Oil pressure switch → disable engine if low
- Brake light switch → enable brake lights
- Manual override switches

**Implementation**:
```cpp
const int INPUT_PINS[4] = {A4, A5, A6, A7};  // Analog pins as digital input

struct InputTrigger {
  int inputPin;
  bool inverted;      // true = active low
  int targetChannel;  // Which output to trigger
  bool overrideEnable;  // true = force ON, false = force OFF
};

InputTrigger triggers[4];

void processInputs() {
  for (int i = 0; i < 4; i++) {
    bool state = digitalRead(triggers[i].inputPin);
    if (triggers[i].inverted) state = !state;

    if (state && triggers[i].overrideEnable) {
      channels[triggers[i].targetChannel].enabled = true;
    }
  }
}
```

**CAN Protocol Update**:
```
0x605 (Input Configuration):
  Byte 0: Input 1 target channel (0-7)
  Byte 1: Input 1 flags (bit 0 = inverted, bit 1 = override enable)
  Byte 2: Input 2 target channel
  ... (4 inputs total)
```

**Effort**: 3 hours firmware + 2 hours testing
**Hardware**: 4 wire inputs with pull-up resistors

---

### 3. Advanced Logic (Medium - Firmware)

**Current**: No conditional logic
**Upgrade**: If-then conditions, timers, multi-input logic

**Use Cases**:
- IF (coolant temp > 90°C AND AC on) THEN fan = 100%
- IF (coolant temp > 80°C) THEN fan = 50%
- IF (oil pressure < 10psi AND RPM > 1000) THEN check engine light
- Delay-on: AC compressor waits 2s before engaging
- Delay-off: Radiator fan runs 30s after engine off

**Implementation**:
```cpp
enum LogicOp {
  OP_NONE, OP_GREATER, OP_LESS, OP_EQUAL, OP_AND, OP_OR, OP_TIMER
};

struct LogicRule {
  LogicOp operation;
  float threshold;        // For comparisons
  int sourceChannel1;     // Input or sensor
  int sourceChannel2;     // For AND/OR
  int targetChannel;      // Output channel
  byte targetPWM;         // PWM duty cycle to apply
  unsigned long delayMs;  // For timers
  unsigned long lastTrigger;
  bool active;
};

LogicRule rules[16];  // Up to 16 programmable rules

void processLogic() {
  for (int i = 0; i < 16; i++) {
    if (!rules[i].active) continue;

    bool condition = false;

    switch (rules[i].operation) {
      case OP_GREATER:
        // Example: If CAN temp sensor > 90°C
        condition = (canTempSensor > rules[i].threshold);
        break;

      case OP_TIMER:
        // Delay-on: wait delayMs before enabling
        if (millis() - rules[i].lastTrigger > rules[i].delayMs) {
          condition = true;
        }
        break;

      case OP_AND:
        // Example: Input 1 AND Input 2
        condition = digitalRead(INPUT_PINS[rules[i].sourceChannel1]) &&
                    digitalRead(INPUT_PINS[rules[i].sourceChannel2]);
        break;
    }

    if (condition) {
      channels[rules[i].targetChannel].enabled = true;
      channels[rules[i].targetChannel].pwmDutyCycle = rules[i].targetPWM;
    }
  }
}
```

**CAN Protocol Update**:
```
0x610-0x61F (Logic Rules):
  Each rule uses 1 CAN message:
  Byte 0: Operation type (0-6)
  Byte 1: Source channel 1
  Byte 2: Source channel 2 (for AND/OR)
  Byte 3: Target channel
  Byte 4: Target PWM (0-100%)
  Byte 5-6: Threshold (scaled float, 0.1 resolution)
  Byte 7: Flags (bit 0 = active, bit 1 = inverted)
```

**Effort**: 8 hours firmware + 4 hours testing
**Hardware**: No changes needed

---

### 4. Load Shedding (Easy - Firmware)

**Current**: No priority system
**Upgrade**: Disable low-priority loads on low battery voltage

**Use Cases**:
- Battery < 11V → disable aux lights, heated seats
- Battery < 10V → disable A/C compressor
- Battery < 9V → disable everything except fuel pump, ignition

**Implementation**:
```cpp
enum Priority {
  CRITICAL,    // Fuel pump, ignition (never disable)
  ESSENTIAL,   // Radiator fan, water pump
  COMFORT,     // A/C, heated seats
  AUXILIARY    // Aux lights, stereo
};

struct Channel {
  // Add priority field
  Priority priority;
};

void loadShedding() {
  float batteryVoltage = readBatteryVoltage();

  if (batteryVoltage < 11.0) {
    // Disable AUXILIARY loads
    for (int i = 0; i < 8; i++) {
      if (channels[i].priority == AUXILIARY) {
        channels[i].enabled = false;
      }
    }
  }

  if (batteryVoltage < 10.0) {
    // Disable COMFORT loads
    for (int i = 0; i < 8; i++) {
      if (channels[i].priority == COMFORT) {
        channels[i].enabled = false;
      }
    }
  }

  if (batteryVoltage < 9.0) {
    // Disable ESSENTIAL loads (keep CRITICAL only)
    for (int i = 0; i < 8; i++) {
      if (channels[i].priority == ESSENTIAL) {
        channels[i].enabled = false;
      }
    }
  }
}

float readBatteryVoltage() {
  // Measure battery voltage via voltage divider (12V → 5V range)
  // R1 = 10k, R2 = 5.6k (divider ratio = 2.786)
  int adcValue = analogRead(A7);  // Dedicated battery sense pin
  float voltage = (adcValue / 1023.0) * 5.0 * 2.786;
  return voltage;
}
```

**Effort**: 3 hours firmware + 2 hours testing
**Hardware**: Voltage divider (2 resistors) on battery input

---

### 5. Soft Start / Current Ramping (Easy - Firmware)

**Current**: Instant on (inrush current spike)
**Upgrade**: Gradual PWM ramp (0% → 100% over 500ms)

**Use Cases**:
- Electric motor soft start (reduces mechanical stress)
- High-power loads (reduces voltage sag)
- Halogen bulbs (extends lifespan)

**Implementation**:
```cpp
struct Channel {
  // Add ramp fields
  bool softStartEnabled;
  unsigned long rampDuration;  // milliseconds
  unsigned long rampStartTime;
};

void updateChannel(Channel *ch) {
  if (ch->enabled && !ch->permanentDisable) {
    if (ch->softStartEnabled && ch->pwmEnabled) {
      unsigned long elapsed = millis() - ch->rampStartTime;

      if (elapsed < ch->rampDuration) {
        // Ramp from 0 to target PWM over rampDuration
        byte rampPWM = map(elapsed, 0, ch->rampDuration, 0, ch->pwmDutyCycle);
        analogWrite(ch->controlPin, map(rampPWM, 0, 100, 0, 255));
      } else {
        // Ramp complete, run at target PWM
        analogWrite(ch->controlPin, map(ch->pwmDutyCycle, 0, 100, 0, 255));
      }
    } else {
      digitalWrite(ch->controlPin, HIGH);
    }
  }
}
```

**Effort**: 2 hours firmware + 1 hour testing
**Hardware**: No changes needed

---

### 6. Temperature-Based Fan Control (Medium - Firmware + Sensor)

**Current**: Manual fan control via CAN
**Upgrade**: Automatic fan speed based on coolant temperature

**Use Cases**:
- 60°C → fan off
- 80°C → fan 30%
- 90°C → fan 60%
- 100°C → fan 100%

**Implementation**:
```cpp
float readCoolantTemp() {
  // Read from CAN (ECU sends coolant temp on 0x420 byte 0)
  // Or read from local NTC thermistor sensor
  return canCoolantTemp;
}

void autoFanControl() {
  float temp = readCoolantTemp();

  // Fan curve (linear interpolation)
  byte fanPWM = 0;

  if (temp < 80) {
    fanPWM = 0;  // Off
  } else if (temp < 90) {
    fanPWM = map(temp, 80, 90, 30, 60);  // 30-60%
  } else if (temp < 100) {
    fanPWM = map(temp, 90, 100, 60, 100);  // 60-100%
  } else {
    fanPWM = 100;  // Full blast
  }

  channels[1].pwmDutyCycle = fanPWM;  // CH2 = Radiator Fan 1
  channels[1].enabled = (fanPWM > 0);
}
```

**Effort**: 3 hours firmware + 2 hours testing
**Hardware**: Optional NTC thermistor (if not using CAN temp)

---

### 7. Data Logging via CAN (Easy - Firmware)

**Current**: Real-time status only
**Upgrade**: Log min/max/avg current, fault history

**Use Cases**:
- Track peak current draw per channel
- Count fault events
- Energy consumption (Amp-hours)
- Uptime statistics

**Implementation**:
```cpp
struct ChannelStats {
  float currentMin;
  float currentMax;
  float currentAvg;
  unsigned long faultCountTotal;
  float energyAh;  // Amp-hours consumed
};

ChannelStats stats[8];

void updateStats() {
  for (int i = 0; i < 8; i++) {
    float current = channels[i].current;

    // Track min/max
    if (current < stats[i].currentMin) stats[i].currentMin = current;
    if (current > stats[i].currentMax) stats[i].currentMax = current;

    // Running average
    stats[i].currentAvg = (stats[i].currentAvg * 0.99) + (current * 0.01);

    // Energy consumption (integrate current over time)
    // 100Hz loop = 0.01s interval
    stats[i].energyAh += (current * 0.01 / 3600.0);  // Amp-hours

    // Fault count
    if (channels[i].faulted) {
      stats[i].faultCountTotal++;
    }
  }
}

void sendCANDataLog() {
  // 0x620-0x627 (Data Log per channel)
  for (int i = 0; i < 8; i++) {
    byte msg[8] = {0};

    msg[0] = (byte)(stats[i].currentMin * 10);
    msg[1] = (byte)(stats[i].currentMax * 10);
    msg[2] = (byte)(stats[i].currentAvg * 10);
    msg[3] = lowByte(stats[i].faultCountTotal);
    msg[4] = highByte(stats[i].faultCountTotal);
    msg[5] = (byte)(stats[i].energyAh * 10);  // 0.1Ah resolution

    CAN0.sendMsgBuf(0x620 + i, 0, 8, msg);
  }
}
```

**Effort**: 4 hours firmware + 2 hours testing
**Hardware**: No changes needed

---

### 8. Flash/Pulse Outputs (Easy - Firmware)

**Current**: Steady on/off only
**Upgrade**: Flashing patterns (turn signals, warning lights)

**Use Cases**:
- Turn signals (500ms on/off)
- Hazard lights (synchronized flash)
- Warning flashers (engine overheat)

**Implementation**:
```cpp
struct Channel {
  // Add flash fields
  bool flashEnabled;
  unsigned int flashOnTime;   // milliseconds
  unsigned int flashOffTime;  // milliseconds
  unsigned long flashLastToggle;
  bool flashState;
};

void updateChannel(Channel *ch) {
  if (ch->flashEnabled && ch->enabled) {
    unsigned long now = millis();
    unsigned long elapsed = now - ch->flashLastToggle;

    if (ch->flashState && elapsed > ch->flashOnTime) {
      digitalWrite(ch->controlPin, LOW);
      ch->flashState = false;
      ch->flashLastToggle = now;
    } else if (!ch->flashState && elapsed > ch->flashOffTime) {
      digitalWrite(ch->controlPin, HIGH);
      ch->flashState = true;
      ch->flashLastToggle = now;
    }
  }
}
```

**Effort**: 2 hours firmware + 1 hour testing
**Hardware**: No changes needed

---

## Summary: V2 vs V2.1

| Feature | V2 (Current) | V2.1 (Upgrade) | Effort |
|---------|-------------|----------------|--------|
| **Outputs** | Digital on/off | PWM variable speed | Easy (2h) |
| **Inputs** | CAN only | 4 digital inputs | Easy (3h) |
| **Logic** | None | 16 programmable rules | Medium (8h) |
| **Load Shedding** | No | Priority-based | Easy (3h) |
| **Soft Start** | No | Current ramping | Easy (2h) |
| **Fan Control** | Manual | Temp-based auto | Medium (3h) |
| **Data Logging** | Real-time only | Min/max/avg/energy | Easy (4h) |
| **Flash/Pulse** | No | Programmable patterns | Easy (2h) |

**Total Development Time**: ~30 hours (1 week part-time)

**Hardware Changes**: Minimal
- 4 digital input wires (optional)
- 1 battery voltage sense (2 resistors)
- No PCB redesign needed

---

## Implementation Priority

### Phase 1 (High Value, Low Effort)
1. PWM outputs (variable fan/pump speed)
2. Load shedding (battery protection)
3. Soft start (motor protection)

**Time**: 1 weekend (7 hours)

### Phase 2 (Medium Value, Medium Effort)
4. Digital inputs (local triggers)
5. Temperature-based fan control
6. Data logging

**Time**: 1 week part-time (12 hours)

### Phase 3 (Advanced Features)
7. Advanced logic rules
8. Flash/pulse outputs

**Time**: 1 week part-time (10 hours)

---

## CAN Protocol Extension

### New Message IDs

| ID | Name | Direction | Purpose |
|----|------|-----------|---------|
| 0x600 | PDM_CONTROL | ECU → PDM | Enable bits (existing) |
| 0x604 | PDM_PWM | ECU → PDM | PWM duty cycles (new) |
| 0x605 | PDM_INPUT_CONFIG | ECU → PDM | Input trigger config (new) |
| 0x610-0x61F | PDM_LOGIC_RULES | ECU → PDM | Logic rule programming (new) |
| 0x620-0x627 | PDM_DATA_LOG | PDM → ECU | Per-channel statistics (new) |

### Backward Compatibility
- V2 firmware still works with existing 0x600/0x601 messages
- V2.1 features are opt-in (default disabled)
- ECU can detect V2.1 via 0x603 telemetry byte 6 (version flag)

---

## Comparison: DIY V2.1 vs Haltech PD16

| Feature | DIY V2.1 | Haltech PD16 | Notes |
|---------|----------|--------------|-------|
| **Channels** | 8 (2x21A + 6x7.5A) | 16 (10x8A + 4x25A) | Haltech has more channels |
| **PWM** | Yes (8 channels) | Yes (4 channels) | We have more PWM outputs |
| **Inputs** | 4 digital | 8 (4 analog + 4 pulsed) | Haltech has analog inputs |
| **Logic Rules** | 16 rules | Unlimited | Haltech has visual editor |
| **Data Logging** | Via CAN | Via CAN | Equal |
| **Configuration** | CAN messages | NSP software | Haltech easier to configure |
| **Cost** | $177-227 | $1,099 + $1,500 ECU | 93% cheaper |
| **Open Source** | Yes | No | We can customize |

**Conclusion**: V2.1 matches 80% of Haltech functionality at 7% of the cost.

---

## Next Steps

1. Update `PDM_V2_Firmware.ino` with Phase 1 features (PWM, load shedding, soft start)
2. Test on bench with resistive loads and light bulbs
3. Document new CAN protocol in `PDM_V2.1_CAN_PROTOCOL.md`
4. Update ECU firmware to send PWM commands
5. Vehicle integration and real-world testing

**Timeline**: 2-3 weeks from start to production-ready V2.1
