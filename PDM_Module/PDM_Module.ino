/*
 * RX8 Power Distribution Module (PDM)
 *
 * Full-featured electronic power distribution with:
 * - 8-channel solid-state switching (30A per channel)
 * - Current monitoring and telemetry
 * - Progressive fan control (temperature-based)
 * - Soft-start (prevent voltage sag)
 * - Load shedding (alternator failure protection)
 * - Kill switch support
 * - Data logging via Serial/CAN
 * - EV conversion support
 * - Race car telemetry
 *
 * Hardware: Arduino Mega 2560 + MCP2515 CAN + MOSFET array + Current sensors
 *
 * Author: MazdaRX8Arduino Project
 * Date: 2025-11-17
 * License: MIT
 */

#include <Arduino.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>
#include <EEPROM.h>

// ========== CONFIGURATION ==========
// Uncomment one of these to select your build type:
#define BUILD_ENGINE_SWAP      // LS1, 2JZ, etc.
// #define BUILD_EV_CONVERSION    // Electric motor conversion
// #define BUILD_RACE_CAR         // Track/race setup
// #define BUILD_STOCK_ROTARY     // Factory rotary with upgrades

// ========== PIN DEFINITIONS ==========
#define CAN_CS_PIN 53
#define CAN_INT_PIN 2

// MOSFET control pins (8 channels, PWM-capable)
const byte CHANNEL_PINS[8] = {22, 24, 26, 28, 30, 32, 34, 36};

// Current sensor analog pins (8 channels)
const byte CURRENT_PINS[8] = {A0, A1, A2, A3, A4, A5, A6, A7};

// Status LEDs (8 channels)
const byte LED_PINS[8] = {38, 40, 42, 44, 46, 48, 50, 52};

// Special inputs
#define KILL_SWITCH_PIN 3         // Digital input, active LOW (interrupt-capable)
#define ALTERNATOR_SENSE_PIN A8   // Voltage sensing (battery voltage)
#define EMERGENCY_STOP_PIN 4      // Digital input, active LOW

// ========== CONSTANTS ==========
#define NUM_CHANNELS 8
#define SERIAL_BAUD 115200
#define CAN_UPDATE_INTERVAL 100   // ms (10 Hz)
#define CURRENT_CHECK_INTERVAL 50 // ms (20 Hz for faster fault detection)
#define TELEMETRY_INTERVAL 1000   // ms (1 Hz for data logging)

// Voltage thresholds
#define BATTERY_NOMINAL 13.8      // V (alternator charging)
#define BATTERY_LOW 12.5          // V (engine off)
#define BATTERY_CRITICAL 11.0     // V (load shedding trigger)
#define BATTERY_OVERCHARGE 15.0   // V (alternator fault)

// Soft-start parameters
#define SOFTSTART_RAMP_TIME 500   // ms (0 → 100% over 500ms)
#define SOFTSTART_STEP_INTERVAL 10 // ms (update PWM every 10ms)

// Load shedding priority (higher = shed first)
#define PRIORITY_CRITICAL 0       // Never shed (fuel pump, BMS)
#define PRIORITY_ESSENTIAL 1      // Shed only in emergency (fans, water pump)
#define PRIORITY_COMFORT 2        // Shed early (A/C, heated seats)
#define PRIORITY_AUXILIARY 3      // Shed first (lights, accessories)

// ========== DATA STRUCTURES ==========

struct Channel {
  // Configuration
  const char* name;        // Channel name (for logging)
  byte pin;                // MOSFET control pin
  byte currentPin;         // Analog current sensor pin
  byte ledPin;             // Status LED pin
  float currentLimit;      // Current limit (Amps)
  byte priority;           // Load shedding priority (0-3)
  bool softStart;          // Enable soft-start ramp

  // Runtime state
  bool enabled;            // Enable command from ECU
  byte targetPWM;          // Target PWM value (0-255)
  byte currentPWM;         // Current PWM value (for soft-start)
  float current;           // Measured current (Amps)
  float peakCurrent;       // Peak current since last reset
  unsigned long totalOnTime; // Total time ON (milliseconds)
  bool faulted;            // Overcurrent fault flag
  bool loadShed;           // Load shedding active

  // Soft-start state
  unsigned long softStartTime; // Time when soft-start began
  bool softStartActive;    // Soft-start in progress
};

// Global channel array
Channel channels[NUM_CHANNELS];

// CAN bus
MCP_CAN CAN0(CAN_CS_PIN);

// System state
struct SystemState {
  float batteryVoltage;
  float totalCurrent;
  bool killSwitchActive;
  bool emergencyStopActive;
  bool loadSheddingActive;
  bool canConnected;
  unsigned long lastCANReceive;
  unsigned long uptime;
  byte faultFlags;

  // Statistics
  unsigned long totalEnergy;  // Watt-hours
  float peakTotalCurrent;

  // Data logging
  bool loggingEnabled;
  unsigned long logCounter;
} systemState;

// Timing
unsigned long lastCANSend = 0;
unsigned long lastCurrentCheck = 0;
unsigned long lastTelemetry = 0;

// ========== CHANNEL CONFIGURATIONS ==========

#ifdef BUILD_ENGINE_SWAP
// LS1/2JZ/etc. engine swap configuration
void configureChannels() {
  channels[0] = {"Fuel Pump",    CHANNEL_PINS[0], CURRENT_PINS[0], LED_PINS[0], 20.0, PRIORITY_CRITICAL,   true,  false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[1] = {"Radiator Fan1", CHANNEL_PINS[1], CURRENT_PINS[1], LED_PINS[1], 25.0, PRIORITY_ESSENTIAL,  false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[2] = {"Radiator Fan2", CHANNEL_PINS[2], CURRENT_PINS[2], LED_PINS[2], 25.0, PRIORITY_ESSENTIAL,  false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[3] = {"Water Pump",    CHANNEL_PINS[3], CURRENT_PINS[3], LED_PINS[3], 15.0, PRIORITY_ESSENTIAL,  true,  false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[4] = {"A/C Clutch",    CHANNEL_PINS[4], CURRENT_PINS[4], LED_PINS[4], 10.0, PRIORITY_COMFORT,    false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[5] = {"Trans Cooler",  CHANNEL_PINS[5], CURRENT_PINS[5], LED_PINS[5], 15.0, PRIORITY_ESSENTIAL,  false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[6] = {"Aux Lights",    CHANNEL_PINS[6], CURRENT_PINS[6], LED_PINS[6], 10.0, PRIORITY_AUXILIARY,  false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[7] = {"Aux Power",     CHANNEL_PINS[7], CURRENT_PINS[7], LED_PINS[7], 10.0, PRIORITY_AUXILIARY,  false, false, 0, 0, 0, 0, 0, false, false, 0, false};
}
#endif

#ifdef BUILD_EV_CONVERSION
// Electric vehicle conversion configuration
void configureChannels() {
  channels[0] = {"BMS Power",     CHANNEL_PINS[0], CURRENT_PINS[0], LED_PINS[0], 10.0, PRIORITY_CRITICAL,   false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[1] = {"Motor Ctrl 12V",CHANNEL_PINS[1], CURRENT_PINS[1], LED_PINS[1], 15.0, PRIORITY_CRITICAL,   false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[2] = {"Motor Coolant", CHANNEL_PINS[2], CURRENT_PINS[2], LED_PINS[2], 15.0, PRIORITY_ESSENTIAL,  true,  false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[3] = {"Battery Coolant",CHANNEL_PINS[3], CURRENT_PINS[3], LED_PINS[3], 15.0, PRIORITY_ESSENTIAL,  true,  false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[4] = {"DC-DC Converter",CHANNEL_PINS[4], CURRENT_PINS[4], LED_PINS[4], 30.0, PRIORITY_CRITICAL,   false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[5] = {"Charging System",CHANNEL_PINS[5], CURRENT_PINS[5], LED_PINS[5], 20.0, PRIORITY_COMFORT,    false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[6] = {"Cabin Heater",  CHANNEL_PINS[6], CURRENT_PINS[6], LED_PINS[6], 25.0, PRIORITY_COMFORT,    true,  false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[7] = {"Aux 12V",       CHANNEL_PINS[7], CURRENT_PINS[7], LED_PINS[7], 10.0, PRIORITY_AUXILIARY,  false, false, 0, 0, 0, 0, 0, false, false, 0, false};
}
#endif

#ifdef BUILD_RACE_CAR
// Race car configuration (telemetry focus)
void configureChannels() {
  channels[0] = {"Fuel Pump",     CHANNEL_PINS[0], CURRENT_PINS[0], LED_PINS[0], 20.0, PRIORITY_CRITICAL,   true,  false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[1] = {"Oil Cooler Fan",CHANNEL_PINS[1], CURRENT_PINS[1], LED_PINS[1], 20.0, PRIORITY_ESSENTIAL,  false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[2] = {"Water Pump",    CHANNEL_PINS[2], CURRENT_PINS[2], LED_PINS[2], 15.0, PRIORITY_CRITICAL,   true,  false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[3] = {"Trans Pump",    CHANNEL_PINS[3], CURRENT_PINS[3], LED_PINS[3], 10.0, PRIORITY_ESSENTIAL,  false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[4] = {"Data Logger",   CHANNEL_PINS[4], CURRENT_PINS[4], LED_PINS[4], 5.0,  PRIORITY_ESSENTIAL,  false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[5] = {"Shift Light",   CHANNEL_PINS[5], CURRENT_PINS[5], LED_PINS[5], 5.0,  PRIORITY_AUXILIARY,  false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[6] = {"Brake Cooler",  CHANNEL_PINS[6], CURRENT_PINS[6], LED_PINS[6], 15.0, PRIORITY_COMFORT,    false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[7] = {"Aux",           CHANNEL_PINS[7], CURRENT_PINS[7], LED_PINS[7], 10.0, PRIORITY_AUXILIARY,  false, false, 0, 0, 0, 0, 0, false, false, 0, false};
}
#endif

#ifdef BUILD_STOCK_ROTARY
// Stock rotary with upgraded cooling
void configureChannels() {
  channels[0] = {"Fuel Pump",     CHANNEL_PINS[0], CURRENT_PINS[0], LED_PINS[0], 15.0, PRIORITY_CRITICAL,   true,  false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[1] = {"Radiator Fan1", CHANNEL_PINS[1], CURRENT_PINS[1], LED_PINS[1], 20.0, PRIORITY_ESSENTIAL,  false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[2] = {"Radiator Fan2", CHANNEL_PINS[2], CURRENT_PINS[2], LED_PINS[2], 20.0, PRIORITY_ESSENTIAL,  false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[3] = {"Oil Cooler Fan",CHANNEL_PINS[3], CURRENT_PINS[3], LED_PINS[3], 15.0, PRIORITY_ESSENTIAL,  false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[4] = {"A/C Clutch",    CHANNEL_PINS[4], CURRENT_PINS[4], LED_PINS[4], 10.0, PRIORITY_COMFORT,    false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[5] = {"Aux 1",         CHANNEL_PINS[5], CURRENT_PINS[5], LED_PINS[5], 10.0, PRIORITY_AUXILIARY,  false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[6] = {"Aux 2",         CHANNEL_PINS[6], CURRENT_PINS[6], LED_PINS[6], 10.0, PRIORITY_AUXILIARY,  false, false, 0, 0, 0, 0, 0, false, false, 0, false};
  channels[7] = {"Aux 3",         CHANNEL_PINS[7], CURRENT_PINS[7], LED_PINS[7], 10.0, PRIORITY_AUXILIARY,  false, false, 0, 0, 0, 0, 0, false, false, 0, false};
}
#endif

// ========== SETUP ==========
void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println(F("========================================"));
  Serial.println(F("  RX8 Power Distribution Module (PDM)"));
  Serial.println(F("========================================"));

  #ifdef BUILD_ENGINE_SWAP
  Serial.println(F("Configuration: ENGINE SWAP"));
  #endif
  #ifdef BUILD_EV_CONVERSION
  Serial.println(F("Configuration: EV CONVERSION"));
  #endif
  #ifdef BUILD_RACE_CAR
  Serial.println(F("Configuration: RACE CAR"));
  #endif
  #ifdef BUILD_STOCK_ROTARY
  Serial.println(F("Configuration: STOCK ROTARY"));
  #endif

  Serial.println();

  // Initialize CAN bus
  Serial.print(F("Initializing CAN bus... "));
  if (CAN0.begin(CAN_500KBPS) == CAN_OK) {
    Serial.println(F("OK"));
    systemState.canConnected = true;
  } else {
    Serial.println(F("FAILED"));
    systemState.canConnected = false;
    // Continue anyway - can operate without CAN in emergency
  }

  pinMode(CAN_INT_PIN, INPUT);

  // Initialize special inputs
  pinMode(KILL_SWITCH_PIN, INPUT_PULLUP);
  pinMode(EMERGENCY_STOP_PIN, INPUT_PULLUP);

  // Attach interrupts for kill switch (fast response)
  attachInterrupt(digitalPinToInterrupt(KILL_SWITCH_PIN), killSwitchISR, CHANGE);

  // Configure channels
  configureChannels();

  // Initialize channel pins
  for (int i = 0; i < NUM_CHANNELS; i++) {
    pinMode(channels[i].pin, OUTPUT);
    pinMode(channels[i].ledPin, OUTPUT);
    digitalWrite(channels[i].pin, LOW);    // All off initially
    digitalWrite(channels[i].ledPin, LOW);
  }

  // Initialize system state
  systemState.batteryVoltage = 12.0;
  systemState.totalCurrent = 0;
  systemState.killSwitchActive = false;
  systemState.emergencyStopActive = false;
  systemState.loadSheddingActive = false;
  systemState.lastCANReceive = 0;
  systemState.uptime = 0;
  systemState.faultFlags = 0;
  systemState.totalEnergy = 0;
  systemState.peakTotalCurrent = 0;
  systemState.loggingEnabled = true;
  systemState.logCounter = 0;

  // Self-test: Blink all LEDs
  Serial.println(F("Running self-test..."));
  for (int blink = 0; blink < 3; blink++) {
    for (int i = 0; i < NUM_CHANNELS; i++) {
      digitalWrite(channels[i].ledPin, HIGH);
    }
    delay(200);
    for (int i = 0; i < NUM_CHANNELS; i++) {
      digitalWrite(channels[i].ledPin, LOW);
    }
    delay(200);
  }

  Serial.println(F("PDM Ready"));
  Serial.println();
  printChannelConfiguration();
}

// ========== MAIN LOOP ==========
void loop() {
  systemState.uptime = millis();

  // 1. Check kill switch and emergency stop
  checkSafetyInputs();

  // 2. Check for CAN messages from ECU
  if (systemState.canConnected && CAN_MSGAVAIL == CAN0.checkReceive()) {
    processCANMessage();
  }

  // 3. Monitor battery voltage and detect alternator failure
  if (millis() - lastCurrentCheck >= CURRENT_CHECK_INTERVAL) {
    lastCurrentCheck = millis();
    monitorBatteryVoltage();
    monitorCurrent();
    updateLoadShedding();
  }

  // 4. Update channel outputs (soft-start, PWM)
  updateChannels();

  // 5. Send status to ECU via CAN
  if (millis() - lastCANSend >= CAN_UPDATE_INTERVAL) {
    lastCANSend = millis();
    if (systemState.canConnected) {
      sendCANStatus();
    }
  }

  // 6. Telemetry and data logging
  if (millis() - lastTelemetry >= TELEMETRY_INTERVAL) {
    lastTelemetry = millis();
    if (systemState.loggingEnabled) {
      logTelemetry();
    }
  }

  // 7. Check for CAN timeout (failsafe)
  checkCANTimeout();
}

// ========== SAFETY INPUTS ==========

// Interrupt service routine for kill switch (immediate response)
void killSwitchISR() {
  systemState.killSwitchActive = !digitalRead(KILL_SWITCH_PIN);
  if (systemState.killSwitchActive) {
    // Immediately shut down all channels
    for (int i = 0; i < NUM_CHANNELS; i++) {
      digitalWrite(channels[i].pin, LOW);
      channels[i].currentPWM = 0;
    }
  }
}

void checkSafetyInputs() {
  // Emergency stop (active LOW)
  systemState.emergencyStopActive = !digitalRead(EMERGENCY_STOP_PIN);

  if (systemState.emergencyStopActive) {
    // Shut down all non-critical channels
    for (int i = 0; i < NUM_CHANNELS; i++) {
      if (channels[i].priority != PRIORITY_CRITICAL) {
        channels[i].enabled = false;
      }
    }
  }
}

// ========== CAN MESSAGE PROCESSING ==========
void processCANMessage() {
  unsigned long id;
  byte len;
  byte buf[8];

  CAN0.readMsgBufID(&id, &len, buf);
  systemState.lastCANReceive = millis();

  if (id == 0x600) {  // PDM Control from ECU
    // Byte 0: Enable bits (8 channels)
    for (int i = 0; i < NUM_CHANNELS; i++) {
      bool newEnable = (buf[0] & (1 << i)) != 0;

      // Only allow enable if not faulted and not in kill/emergency
      if (!channels[i].faulted && !systemState.killSwitchActive && !systemState.emergencyStopActive) {
        channels[i].enabled = newEnable;
      }
    }

    // Bytes 1-4: PWM values for first 4 channels
    if (len >= 5) {
      for (int i = 0; i < 4 && i < NUM_CHANNELS; i++) {
        channels[i].targetPWM = buf[i + 1];
      }
    }

    // Bytes 5-7: Reserved (could be PWM for channels 5-8)
  }

  // Command messages
  else if (id == 0x602) {  // PDM Commands
    byte command = buf[0];
    switch (command) {
      case 0x01:  // Reset faults
        resetFaults();
        break;
      case 0x02:  // Enable data logging
        systemState.loggingEnabled = true;
        break;
      case 0x03:  // Disable data logging
        systemState.loggingEnabled = false;
        break;
      case 0x10:  // Reset peak current
        for (int i = 0; i < NUM_CHANNELS; i++) {
          channels[i].peakCurrent = 0;
        }
        systemState.peakTotalCurrent = 0;
        break;
    }
  }
}

void checkCANTimeout() {
  // If no CAN message received for 1 second, enter failsafe mode
  if (systemState.canConnected && millis() - systemState.lastCANReceive > 1000) {
    // Shut down all channels
    for (int i = 0; i < NUM_CHANNELS; i++) {
      channels[i].enabled = false;
    }

    // Blink all LEDs to indicate failsafe
    if ((millis() / 250) % 2 == 0) {
      for (int i = 0; i < NUM_CHANNELS; i++) {
        digitalWrite(channels[i].ledPin, HIGH);
      }
    } else {
      for (int i = 0; i < NUM_CHANNELS; i++) {
        digitalWrite(channels[i].ledPin, LOW);
      }
    }
  }
}

// ========== CHANNEL CONTROL ==========

void updateChannels() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    Channel* ch = &channels[i];

    // Determine if channel should be ON
    bool shouldBeOn = ch->enabled && !ch->faulted && !ch->loadShed &&
                      !systemState.killSwitchActive && !systemState.emergencyStopActive;

    if (shouldBeOn) {
      // Soft-start logic
      if (ch->softStart && ch->currentPWM < ch->targetPWM) {
        if (!ch->softStartActive) {
          // Begin soft-start
          ch->softStartActive = true;
          ch->softStartTime = millis();
          ch->currentPWM = 0;
        }

        // Calculate ramped PWM value
        unsigned long elapsed = millis() - ch->softStartTime;
        if (elapsed < SOFTSTART_RAMP_TIME) {
          // Linear ramp: 0 → targetPWM over SOFTSTART_RAMP_TIME
          ch->currentPWM = (ch->targetPWM * elapsed) / SOFTSTART_RAMP_TIME;
        } else {
          // Ramp complete
          ch->currentPWM = ch->targetPWM;
          ch->softStartActive = false;
        }
      } else {
        // No soft-start or already at target
        ch->currentPWM = ch->targetPWM;
        ch->softStartActive = false;
      }

      // Apply PWM
      analogWrite(ch->pin, ch->currentPWM);

      // Update total ON time
      if (ch->currentPWM > 0) {
        ch->totalOnTime += (millis() - lastCurrentCheck);
      }

      // LED on solid
      digitalWrite(ch->ledPin, HIGH);

    } else {
      // Channel OFF
      digitalWrite(ch->pin, LOW);
      ch->currentPWM = 0;
      ch->softStartActive = false;

      // LED blink if faulted, off otherwise
      if (ch->faulted) {
        digitalWrite(ch->ledPin, (millis() / 500) % 2);  // Blink 1 Hz
      } else {
        digitalWrite(ch->ledPin, LOW);
      }
    }
  }
}

// ========== CURRENT MONITORING ==========

void monitorCurrent() {
  systemState.totalCurrent = 0;

  for (int i = 0; i < NUM_CHANNELS; i++) {
    Channel* ch = &channels[i];

    // Read current from ACS712 sensor
    int sensorValue = analogRead(ch->currentPin);
    float voltage = sensorValue * (5.0 / 1023.0);

    // ACS712-30A: 2.5V = 0A, 66 mV/A
    ch->current = abs((voltage - 2.5) / 0.066);

    // Update peak current
    if (ch->current > ch->peakCurrent) {
      ch->peakCurrent = ch->current;
    }

    // Check for overcurrent
    if (ch->current > ch->currentLimit) {
      ch->faulted = true;
      ch->enabled = false;  // Force off
      digitalWrite(ch->pin, LOW);

      Serial.print(F("OVERCURRENT Ch"));
      Serial.print(i);
      Serial.print(F(" ("));
      Serial.print(ch->name);
      Serial.print(F("): "));
      Serial.print(ch->current);
      Serial.print(F("A > "));
      Serial.print(ch->currentLimit);
      Serial.println(F("A"));

      systemState.faultFlags |= (1 << i);
    }

    systemState.totalCurrent += ch->current;
  }

  // Update peak total current
  if (systemState.totalCurrent > systemState.peakTotalCurrent) {
    systemState.peakTotalCurrent = systemState.totalCurrent;
  }

  // Calculate energy consumption (Watt-hours)
  // Energy (Wh) = Power (W) × Time (h)
  // Power = Voltage × Current
  float powerWatts = systemState.batteryVoltage * systemState.totalCurrent;
  float timeHours = CURRENT_CHECK_INTERVAL / 3600000.0;  // ms to hours
  systemState.totalEnergy += powerWatts * timeHours;
}

void monitorBatteryVoltage() {
  // Read battery voltage from voltage divider on ALTERNATOR_SENSE_PIN
  // Assumed divider: 10kΩ + 1kΩ (11:1 ratio, max 55V → 5V)
  int sensorValue = analogRead(ALTERNATOR_SENSE_PIN);
  float adcVoltage = sensorValue * (5.0 / 1023.0);
  systemState.batteryVoltage = adcVoltage * 11.0;  // Scale back to battery voltage

  // Detect alternator failure (voltage too low)
  if (systemState.batteryVoltage < BATTERY_CRITICAL && !systemState.loadSheddingActive) {
    Serial.println(F("WARNING: Low battery voltage, load shedding activated!"));
    systemState.loadSheddingActive = true;
  }

  // Detect alternator overvoltage (regulator failure)
  if (systemState.batteryVoltage > BATTERY_OVERCHARGE) {
    Serial.println(F("CRITICAL: Battery overvoltage detected!"));
    // Could shut down high-current devices to protect electronics
  }
}

// ========== LOAD SHEDDING ==========

void updateLoadShedding() {
  if (!systemState.loadSheddingActive) {
    // Normal operation - ensure all channels can run
    for (int i = 0; i < NUM_CHANNELS; i++) {
      channels[i].loadShed = false;
    }
    return;
  }

  // Load shedding active - shed by priority
  // Priority 3 (Auxiliary) shed first, then 2 (Comfort), then 1 (Essential)
  // Priority 0 (Critical) never shed

  for (int priority = PRIORITY_AUXILIARY; priority >= PRIORITY_COMFORT; priority--) {
    for (int i = 0; i < NUM_CHANNELS; i++) {
      if (channels[i].priority == priority) {
        channels[i].loadShed = true;
        channels[i].enabled = false;
      }
    }

    // Check if battery voltage recovered
    if (systemState.batteryVoltage > BATTERY_LOW) {
      Serial.println(F("Battery voltage recovered, load shedding deactivated"));
      systemState.loadSheddingActive = false;
      return;
    }
  }
}

// ========== CAN STATUS TRANSMISSION ==========

void sendCANStatus() {
  byte status[8];

  // Message 1: Channel status and current (0x601)
  status[0] = 0;  // Active channels (bit flags)
  for (int i = 0; i < NUM_CHANNELS; i++) {
    if (channels[i].enabled && !channels[i].faulted && channels[i].currentPWM > 0) {
      status[0] |= (1 << i);
    }
  }

  // Bytes 1-4: Current per channel (scaled to 0-255 for 30A)
  for (int i = 0; i < 4; i++) {
    status[i + 1] = constrain(channels[i].current * 8.5, 0, 255);
  }

  // Byte 5: Fault flags
  status[5] = systemState.faultFlags;

  // Bytes 6-7: Total current (16-bit, 0.1A resolution)
  int totalCurrentInt = (int)(systemState.totalCurrent * 10);
  status[6] = (totalCurrentInt >> 8) & 0xFF;
  status[7] = totalCurrentInt & 0xFF;

  CAN0.sendMsgBuf(0x601, 0, 8, status);

  // Message 2: System telemetry (0x603)
  status[0] = (int)(systemState.batteryVoltage * 10) & 0xFF;  // Battery voltage (0.1V resolution)
  status[1] = systemState.killSwitchActive ? 0x01 : 0x00;
  status[1] |= systemState.emergencyStopActive ? 0x02 : 0x00;
  status[1] |= systemState.loadSheddingActive ? 0x04 : 0x00;

  // Bytes 2-3: Total energy (Watt-hours, 16-bit)
  int energyInt = (int)systemState.totalEnergy;
  status[2] = (energyInt >> 8) & 0xFF;
  status[3] = energyInt & 0xFF;

  // Bytes 4-5: Peak total current (0.1A resolution, 16-bit)
  int peakCurrentInt = (int)(systemState.peakTotalCurrent * 10);
  status[4] = (peakCurrentInt >> 8) & 0xFF;
  status[5] = peakCurrentInt & 0xFF;

  // Bytes 6-7: Uptime (seconds, 16-bit)
  int uptimeSeconds = systemState.uptime / 1000;
  status[6] = (uptimeSeconds >> 8) & 0xFF;
  status[7] = uptimeSeconds & 0xFF;

  CAN0.sendMsgBuf(0x603, 0, 8, status);
}

// ========== FAULT MANAGEMENT ==========

void resetFaults() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    channels[i].faulted = false;
  }
  systemState.faultFlags = 0;
  Serial.println(F("All faults reset"));
}

// ========== TELEMETRY & DATA LOGGING ==========

void logTelemetry() {
  Serial.print(F("LOG,"));
  Serial.print(systemState.logCounter++);
  Serial.print(F(","));
  Serial.print(systemState.uptime);
  Serial.print(F(","));
  Serial.print(systemState.batteryVoltage, 2);
  Serial.print(F(","));
  Serial.print(systemState.totalCurrent, 2);
  Serial.print(F(","));
  Serial.print(systemState.totalEnergy, 1);

  // Per-channel current
  for (int i = 0; i < NUM_CHANNELS; i++) {
    Serial.print(F(","));
    Serial.print(channels[i].current, 2);
  }

  // Flags
  Serial.print(F(","));
  Serial.print(systemState.killSwitchActive ? 1 : 0);
  Serial.print(F(","));
  Serial.print(systemState.loadSheddingActive ? 1 : 0);
  Serial.print(F(","));
  Serial.print(systemState.faultFlags);

  Serial.println();
}

void printChannelConfiguration() {
  Serial.println(F("Channel Configuration:"));
  Serial.println(F("Ch | Name             | Limit | Priority | Soft-Start"));
  Serial.println(F("---|------------------|-------|----------|------------"));
  for (int i = 0; i < NUM_CHANNELS; i++) {
    Serial.print(i);
    Serial.print(F("  | "));
    Serial.print(channels[i].name);

    // Pad name to 16 chars
    for (int j = strlen(channels[i].name); j < 16; j++) {
      Serial.print(F(" "));
    }

    Serial.print(F(" | "));
    Serial.print(channels[i].currentLimit, 1);
    Serial.print(F("A"));

    // Pad current to 5 chars
    int len = String(channels[i].currentLimit, 1).length();
    for (int j = len; j < 4; j++) {
      Serial.print(F(" "));
    }

    Serial.print(F(" | "));
    switch (channels[i].priority) {
      case PRIORITY_CRITICAL:  Serial.print(F("Critical ")); break;
      case PRIORITY_ESSENTIAL: Serial.print(F("Essential")); break;
      case PRIORITY_COMFORT:   Serial.print(F("Comfort  ")); break;
      case PRIORITY_AUXILIARY: Serial.print(F("Auxiliary")); break;
    }

    Serial.print(F(" | "));
    Serial.println(channels[i].softStart ? F("Yes") : F("No "));
  }
  Serial.println();
}
