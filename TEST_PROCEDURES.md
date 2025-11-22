# Test Procedures - RX8 Unified ECU

**Comprehensive testing guide from bench to vehicle**

⚠️ **SAFETY WARNING**: This code controls critical vehicle systems. Test thoroughly before vehicle installation!

---

## Table of Contents

1. [Bench Testing](#bench-testing)
2. [CAN Bus Testing](#can-bus-testing)
3. [Integration Testing](#integration-testing)
4. [Pre-Vehicle Checklist](#pre-vehicle-checklist)

---

## Bench Testing

**Goal**: Verify firmware functionality without vehicle

### Equipment Needed

- Development board (STM32/C2000/S32K + ESP32)
- USB power + cables
- Multimeter
- Breadboard + jumper wires
- Logic analyzer or oscilloscope (optional)
- 12V power supply (optional, for realistic testing)

### Test 1: Power-On Self Test (POST)

#### Procedure

1. **Power on board**
   ```bash
   # Connect USB
   # Open serial monitor @ 115200 baud
   ```

2. **Verify startup messages**
   ```
   Expected output:
   - HAL initialization (CPU, Flash, RAM info)
   - Module initialization messages
   - "=== AUTOMOTIVE ECU READY ===" or "=== UI CONTROLLER READY ==="
   ```

3. **Check for errors**
   ```
   ❌ FAIL if you see:
   - "Failed to find High Speed CAN"
   - Watchdog reset messages
   - Hard fault errors
   ```

#### Pass Criteria

- ✅ All modules initialize successfully
- ✅ No error messages in serial output
- ✅ Watchdog kicks without reset
- ✅ Free heap > 50% remaining

---

### Test 2: GPIO Functionality

#### Test LEDs (Output)

```cpp
// In setup() or loop()
HAL_GPIO_SetMode(LED_STATUS_1, HAL_GPIO_MODE_OUTPUT);
HAL_GPIO_Write(LED_STATUS_1, true);   // LED on
delay(1000);
HAL_GPIO_Write(LED_STATUS_1, false);  // LED off
```

**Expected**: LED blinks on/off

#### Test Buttons (Input)

```cpp
HAL_GPIO_SetMode(INPUT_BRAKE_SWITCH, HAL_GPIO_MODE_INPUT_PULLUP);
bool brake_pressed = HAL_GPIO_Read(INPUT_BRAKE_SWITCH);
Serial.printf("Brake: %s\n", brake_pressed ? "PRESSED" : "RELEASED");
```

**Expected**: Serial output changes when button pressed

#### Pass Criteria

- ✅ All configured LEDs can be controlled
- ✅ Button inputs read correctly (with pull-ups)
- ✅ No GPIO conflicts or shorts

---

### Test 3: ADC (Analog Input)

#### Test with Potentiometer

```cpp
// Connect potentiometer:
// - One end to 3.3V
// - Middle (wiper) to THROTTLE_ADC_PIN
// - Other end to GND

uint16_t adc_value = HAL_ADC_Read(THROTTLE_ADC_PIN);
uint16_t voltage_mv = HAL_ADC_ReadMillivolts(THROTTLE_ADC_PIN);

Serial.printf("ADC: %d (raw) = %d mV\n", adc_value, voltage_mv);
```

**Expected**:
- Turning pot changes ADC value smoothly
- 0% pot = ~0 mV
- 50% pot = ~1650 mV (3.3V / 2)
- 100% pot = ~3300 mV

#### RX8 Throttle Pedal Simulation

```cpp
// Connect voltage source: 1.7V - 4.0V range
// Use potentiometer + voltage divider

uint8_t throttle_percent = ThrottlePedal::readPercent();
Serial.printf("Throttle: %d%%\n", throttle_percent);
```

**Expected**:
- 1.7V = 0% throttle
- 2.85V = 50% throttle
- 4.0V = 100% throttle

#### Pass Criteria

- ✅ ADC reads 12-bit values (0-4095) correctly
- ✅ Voltage conversion accurate (±50mV tolerance)
- ✅ No noise or jitter (readings stable ±10 counts)

---

### Test 4: PWM (Pulse Width Modulation)

#### Visual Test with LED

```cpp
HAL_GPIO_SetMode(THROTTLE_PWM_PIN, HAL_GPIO_MODE_PWM);

// Fade LED from 0-100%
for (uint8_t duty = 0; duty <= 255; duty++) {
    HAL_PWM_Write(THROTTLE_PWM_PIN, duty);
    delay(10);
}
```

**Expected**: LED fades from off to full brightness

#### Oscilloscope/Logic Analyzer Test

```
Settings:
- Frequency: ~1 kHz (typical for throttle)
- Duty cycle: Variable 0-100%

Expected waveform:
- Clean square wave
- No overshoot/ringing
- Voltage swing: 0-3.3V (or 0-5V if level shifted)
```

#### Multimeter Test (DC voltage)

```cpp
HAL_PWM_Write(THROTTLE_PWM_PIN, 128);  // 50% duty
// Measure with multimeter (DC mode)
```

**Expected**: ~1.65V (50% of 3.3V)

#### Pass Criteria

- ✅ PWM frequency correct (±10%)
- ✅ Duty cycle linear (0% = 0V, 100% = Vcc)
- ✅ No excessive noise or ripple

---

### Test 5: UART Communication

#### Loopback Test

```cpp
// Connect TX → RX (short circuit for loopback)

HAL_UART_Init(UART_ESP32_PORT, 115200);

const char* test_msg = "Hello UART!";
HAL_UART_Write(UART_ESP32_PORT, (uint8_t*)test_msg, strlen(test_msg));

delay(10);

uint8_t rx_buffer[32];
uint32_t bytes_read = HAL_UART_Read(UART_ESP32_PORT, rx_buffer, 32);

Serial.printf("Sent: %s\n", test_msg);
Serial.printf("Received: %.*s (%d bytes)\n", bytes_read, rx_buffer, bytes_read);
```

**Expected**: Sent message matches received message

#### ESP32 ↔ Automotive ECU Bridge Test

```
Setup:
1. Connect UART_ESP32_TX (STM32) → UART_ESP32_RX (ESP32)
2. Connect UART_ESP32_RX (STM32) ← UART_ESP32_TX (ESP32)
3. Common GND

Test:
1. Automotive ECU sends vehicle state packet
2. ESP32 receives and validates checksum
3. ESP32 displays data on OLED/web dashboard
```

**Expected**:
- Packets received every 50ms (20 Hz)
- Checksum errors < 0.1%
- Data displayed matches sent values

#### Pass Criteria

- ✅ Baud rate accurate (115200 ±1%)
- ✅ No framing errors
- ✅ Bidirectional communication works
- ✅ Checksum validation passes

---

### Test 6: Watchdog Timer

#### Test Watchdog Trigger

```cpp
void loop() {
    // SafetyMonitor::kick();  // Comment out to trigger watchdog

    // ... rest of code
}
```

**Expected**: MCU resets after ~4 seconds (watchdog timeout)

**Serial Output**:
```
Watchdog reset detected!
[SAFETY] Watchdog initialized
```

#### Test Watchdog Normal Operation

```cpp
void loop() {
    SafetyMonitor::kick();  // Kick every loop

    // Simulate work
    delay(100);
}
```

**Expected**: No watchdog resets, runs indefinitely

#### Pass Criteria

- ✅ Watchdog triggers reset if not kicked
- ✅ Normal operation continues when kicked regularly
- ✅ Reset reason correctly identified (watchdog vs power-on)

---

## CAN Bus Testing

**Goal**: Verify CAN controller and message transmission

### Equipment Needed

- CAN bus analyzer (~$30): PCAN-USB, PEAK, CANable, etc.
- 120Ω termination resistors (2x)
- CAN transceiver module (TJA1050/MCP2551)
- Jumper wires

### CAN Bus Wiring

```
[Automotive ECU]           [CAN Analyzer]
    CAN_TX  ────────┬───────── CAN_H ───┬─── 120Ω ─── GND
    CAN_RX  ────────┼───────── CAN_L ───┘
    GND     ────────┴───────── GND

Note: 120Ω termination required at BOTH ends of bus
```

### Test 7: CAN Message Transmission

#### Monitor CAN Traffic

```bash
# Using candump (Linux/SocketCAN)
candump can0 -c -a

# Using PCAN-View (Windows)
# Open PCAN-View → Connect → Monitor messages
```

**Expected Messages** (every 100ms):
```
ID: 0x201 (513)  - PCM Status (RPM, speed, throttle)
ID: 0x420 (1056) - MIL/Warnings (temp, lights)
ID: 0x620 (1568) - ABS system data
ID: 0x630 (1584) - ABS configuration
ID: 0x203 (515)  - Traction control
ID: 0x215 (533)  - PCM supplement
ID: 0x231 (561)  - PCM supplement
ID: 0x240 (576)  - PCM supplement
```

#### Verify Message Timing

```
Requirement: All messages transmitted every 100ms (10 Hz)
Tolerance: ±10ms acceptable

Tool: Use CAN analyzer timestamp feature
```

#### Verify Message Content

**Test RPM encoding**:
```cpp
// Set RPM to 3000
EngineControl::setRPM(3000);

// Expected on CAN:
// ID: 0x201
// Byte 0-1: 3000 * 3.85 = 11550 = 0x2D1E
// → Byte 0 = 0x2D, Byte 1 = 0x1E
```

**Test speed encoding**:
```cpp
// Set speed to 100 km/h
vehicle_state.speed_kmh = 1000;  // 100.0 km/h * 10

// Convert to mph: 100 / 1.60934 = 62.14 mph
// Encode: (62.14 * 100) + 10000 = 16214

// Expected on CAN:
// Byte 4-5: 16214 = 0x3F56
// → Byte 4 = 0x3F, Byte 5 = 0x56
```

#### Pass Criteria

- ✅ All required CAN messages transmitting
- ✅ 100ms timing accurate (±10ms)
- ✅ Message content correctly encoded
- ✅ No CAN bus errors (error frames, stuff errors)

---

### Test 8: CAN Message Reception

#### Test Wheel Speed Reception

```cpp
// Send test message from CAN analyzer:
// ID: 0x4B1 (1201)
// Data: 27 10 27 10 00 00 00 00
// (represents 10000 + 0x2710 = 20000 → 200 km/h on all wheels)

// Check serial output:
WheelSpeed::read(&fl, &fr, &rl, &rr);
Serial.printf("Wheels: FL=%d FR=%d RL=%d RR=%d\n", fl, fr, rl, rr);
```

**Expected**: All wheel speeds = 200 km/h

#### Test Immobilizer Handshake

```cpp
// Send immobilizer request from CAN analyzer:
// ID: 0x047 (71)
// Data: 00 7F 02 00 00 00 00 00

// Expected response on CAN:
// ID: 0x041 (65)
// Data: 07 0C 30 F2 17 00 00 00

// Then send second request:
// ID: 0x047
// Data: 00 5C F4 00 00 00 00 00

// Expected response:
// ID: 0x041
// Data: 81 7F 00 00 00 00 00 00
```

**Expected**: Immobilizer unlocked, vehicle can start

#### Pass Criteria

- ✅ CAN RX interrupts firing correctly
- ✅ Message filtering works (only process relevant IDs)
- ✅ Message parsing correct (byte order, encoding)
- ✅ Immobilizer handshake completes successfully

---

## Integration Testing

**Goal**: Test multi-module interaction

### Test 9: Automotive ECU + ESP32 Communication

#### Setup

1. Connect UART between automotive ECU and ESP32
2. Power both boards
3. Monitor serial output on both

#### Test Procedure

```cpp
// Automotive ECU side:
g_vehicle_state.rpm = 3000;
g_vehicle_state.speed_kmh = 1000;  // 100.0 km/h
g_vehicle_state.throttle_percent = 75;
UARTBridge::sendVehicleState(g_vehicle_state);

// ESP32 side:
UARTBridge::update();
if (UARTBridge::isValid()) {
    const auto* state = UARTBridge::getVehicleState();
    Serial.printf("Received: RPM=%d, Speed=%.1f km/h\n",
                 state->rpm, state->speed_kmh / 10.0f);
}
```

**Expected**:
- ESP32 receives packets every 50ms
- Data matches sent values
- Checksum validation passes

#### Pass Criteria

- ✅ Packet reception rate = 20 Hz (50ms interval)
- ✅ Data integrity 100% (checksums pass)
- ✅ Latency < 100ms (send to display update)

---

### Test 10: Display Integration

#### OLED Gauges

```cpp
// Update displays with test data
OLEDGauges::setRPM(6000);
OLEDGauges::setSpeed(120);  // km/h
OLEDGauges::setCoolantTemp(90);  // °C
OLEDGauges::setVoltage(1380);  // 13.80V
OLEDGauges::update();
```

**Visual Check**:
- ✅ RPM displays 6000
- ✅ Speed displays 120 km/h
- ✅ Temperature displays 90°C
- ✅ Voltage displays 13.80V
- ✅ No screen artifacts or flickering

#### Web Dashboard

```
1. Connect to WiFi network
2. Open browser: http://rx8-ecu.local or http://[ESP32_IP]
3. Verify real-time gauges update every second
```

**Visual Check**:
- ✅ Gauges animate smoothly
- ✅ Values match serial output
- ✅ Warning lights reflect actual state
- ✅ No connection errors or timeouts

---

### Test 11: Failsafe Mode

#### Test CAN Timeout Failsafe

```cpp
// Simulate CAN timeout by stopping CAN transmissions
void loop() {
    // Comment out CAN transmission code
    // transmitCANMessages();

    SafetyMonitor::update(false);  // false = no CAN activity

    if (SafetyMonitor::getState() == SafetyState::FAILSAFE) {
        Serial.println("FAILSAFE MODE ACTIVE!");
    }
}
```

**Expected** (after 500ms timeout):
```
[SAFETY] WARNING: CAN timeout detected!
╔════════════════════════════════╗
║  FAILSAFE MODE ACTIVATED       ║
╚════════════════════════════════╝
[SAFETY] Reason: CAN RX timeout
[SAFETY] Actions:
[SAFETY]   - Throttle → 0%
[SAFETY]   - RPM → 0
[SAFETY]   - All warning lights ON
```

#### Pass Criteria

- ✅ Failsafe triggers after configured timeout
- ✅ Throttle forced to 0%
- ✅ All warning lights activate
- ✅ Can be cleared when CAN resumes

---

## Pre-Vehicle Checklist

Before installing in vehicle, verify ALL of the following:

### Hardware Checks

- [ ] All solder joints inspected (no cold joints, bridges)
- [ ] Power supply stable (12V ±0.5V under load)
- [ ] All connections secure (no loose wires)
- [ ] Proper heatsinking (if using voltage regulators)
- [ ] CAN termination resistors installed (120Ω at both ends)
- [ ] Fuses installed on all power lines
- [ ] Enclosure protects from moisture/vibration

### Software Checks

- [ ] Correct MCU platform selected (`MCU_PLATFORM`)
- [ ] Correct vehicle type selected (`VEHICLE_TYPE`)
- [ ] Transmission type configured correctly
- [ ] ABS variant tested and confirmed
- [ ] Watchdog enabled and tested
- [ ] Failsafe mode tested
- [ ] Serial debug disabled (production build)

### CAN Bus Checks

- [ ] All required messages transmitting (0x201, 0x420, etc.)
- [ ] 100ms timing verified
- [ ] Message encoding validated
- [ ] Immobilizer handshake tested
- [ ] No CAN bus errors

### Safety Checks

- [ ] Throttle safety limits tested (0-100% range)
- [ ] Watchdog triggers on hang
- [ ] Failsafe activates on timeout
- [ ] Emergency stop tested (kill switch)
- [ ] Temperature limits configured
- [ ] Voltage limits configured

### Documentation

- [ ] Pin connections documented
- [ ] Configuration settings saved
- [ ] Backup firmware created
- [ ] Original ECU backed up (if replacing)
- [ ] Wiring diagram created

---

## Next Steps

After all tests pass:
1. ✅ **Vehicle Testing** → See `VEHICLE_TESTING.md`
2. ✅ **Validation** → See `VEHICLE_TESTING.md`

---

**Last Updated**: 2025-11-16
**Part of Phase 5 unified architecture**
