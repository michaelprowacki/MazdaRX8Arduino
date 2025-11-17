/*
 * PDM V2 Firmware - Infineon PROFET-Based Compact PDM
 *
 * Hardware: Arduino Nano Every + MCP2515 CAN + PROFET ICs
 * Channels: 2x 21A (BTS7002) + 6x 7.5A (BTS7008)
 * Features: Auto-retry circuit breakers, current sensing, CAN bus control
 *
 * Author: MazdaRX8Arduino Project
 * License: MIT
 */

#include <mcp_can.h>
#include <SPI.h>

// Pin Definitions
#define CAN_CS 10

// PROFET Control Pins (Digital OUT)
const int CHANNEL_PINS[8] = {2, 3, 4, 5, 6, 7, 8, 9};

// PROFET Current Sense Pins (Analog IN)
const int SENSE_PINS[8] = {A0, A1, A2, A3, A4, A5, A6, A7};

// CAN Message IDs (same as PDM V1 for compatibility)
#define CAN_PDM_CONTROL 0x600  // ECU -> PDM (enable/PWM)
#define CAN_PDM_STATUS  0x601  // PDM -> ECU (current/faults)
#define CAN_PDM_COMMAND 0x602  // ECU -> PDM (reset/config)
#define CAN_PDM_TELEMETRY 0x603 // PDM -> ECU (uptime/voltage)

// Channel Configuration
struct Channel {
  const char* name;
  int controlPin;
  int sensePin;
  float currentLimit;    // Amps
  float kILIS;           // Current sense ratio (PROFET-specific)

  // State
  bool enabled;
  bool faulted;
  bool permanentDisable;
  int faultCount;
  unsigned long lastFaultTime;
  float current;         // Measured current (Amps)

  // Auto-retry parameters
  const int MAX_FAULTS = 3;          // Max faults before permanent disable
  const unsigned long FAULT_WINDOW = 1000;  // 1 second
  const unsigned long FAULT_RESET = 5000;   // 5 seconds
};

Channel channels[8];
MCP_CAN CAN0(CAN_CS);

// Timing
unsigned long lastCANStatus = 0;
unsigned long lastTelemetry = 0;
const unsigned long CAN_STATUS_INTERVAL = 100;   // 100ms = 10Hz
const unsigned long TELEMETRY_INTERVAL = 1000;   // 1s

// System state
unsigned long uptime = 0;
float batteryVoltage = 12.0;

void setup() {
  Serial.begin(115200);
  Serial.println(F("PDM V2 - PROFET-Based Compact PDM"));

  // Initialize CAN
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println(F("CAN: OK"));
  } else {
    Serial.println(F("CAN: FAIL"));
    while(1);  // Halt if CAN fails
  }
  CAN0.setMode(MCP_NORMAL);

  // Configure channels
  configureChannels();

  // Initialize pins
  for (int i = 0; i < 8; i++) {
    pinMode(channels[i].controlPin, OUTPUT);
    digitalWrite(channels[i].controlPin, LOW);  // All OFF
    pinMode(channels[i].sensePin, INPUT);
  }

  Serial.println(F("PDM Ready"));
}

void configureChannels() {
  // CH1: Fuel Pump (21A, BTS7002)
  channels[0] = {"Fuel Pump", CHANNEL_PINS[0], SENSE_PINS[0], 20.0, 2000.0,
                 false, false, false, 0, 0, 0.0};

  // CH2: Radiator Fan 1 (21A, BTS7002)
  channels[1] = {"Radiator Fan1", CHANNEL_PINS[1], SENSE_PINS[1], 25.0, 2000.0,
                 false, false, false, 0, 0, 0.0};

  // CH3: Radiator Fan 2 (7.5A, BTS7008)
  channels[2] = {"Radiator Fan2", CHANNEL_PINS[2], SENSE_PINS[2], 7.5, 1850.0,
                 false, false, false, 0, 0, 0.0};

  // CH4: Water Pump (7.5A, BTS7008)
  channels[3] = {"Water Pump", CHANNEL_PINS[3], SENSE_PINS[3], 7.5, 1850.0,
                 false, false, false, 0, 0, 0.0};

  // CH5: A/C Clutch (7.5A, BTS7008)
  channels[4] = {"A/C Clutch", CHANNEL_PINS[4], SENSE_PINS[4], 7.5, 1850.0,
                 false, false, false, 0, 0, 0.0};

  // CH6: Trans Cooler (7.5A, BTS7008)
  channels[5] = {"Trans Cooler", CHANNEL_PINS[5], SENSE_PINS[5], 7.5, 1850.0,
                 false, false, false, 0, 0, 0.0};

  // CH7: Aux Lights (7.5A, BTS7008)
  channels[6] = {"Aux Lights", CHANNEL_PINS[6], SENSE_PINS[6], 7.5, 1850.0,
                 false, false, false, 0, 0, 0.0};

  // CH8: Accessory (7.5A, BTS7008)
  channels[7] = {"Accessory", CHANNEL_PINS[7], SENSE_PINS[7], 7.5, 1850.0,
                 false, false, false, 0, 0, 0.0};
}

void loop() {
  // Process CAN messages
  processCANMessages();

  // Update channels
  for (int i = 0; i < 8; i++) {
    updateChannel(&channels[i]);
  }

  // Read current sensors
  for (int i = 0; i < 8; i++) {
    channels[i].current = readCurrent(i);
  }

  // Send CAN status (10Hz)
  if (millis() - lastCANStatus >= CAN_STATUS_INTERVAL) {
    sendCANStatus();
    lastCANStatus = millis();
  }

  // Send CAN telemetry (1Hz)
  if (millis() - lastTelemetry >= TELEMETRY_INTERVAL) {
    sendCANTelemetry();
    lastTelemetry = millis();
    uptime++;
  }

  delay(10);  // 100Hz main loop
}

void processCANMessages() {
  byte len;
  byte buf[8];
  unsigned long id;

  if (CAN_MSGAVAIL == CAN0.checkReceive()) {
    CAN0.readMsgBuf(&id, &len, buf);

    switch (id) {
      case CAN_PDM_CONTROL:
        // Byte 0: Enable bits (CH1-CH8)
        for (int i = 0; i < 8; i++) {
          channels[i].enabled = (buf[0] & (1 << i)) != 0;
        }
        break;

      case CAN_PDM_COMMAND:
        // Byte 0: Command
        if (buf[0] == 0x01) {  // RESET_FAULTS
          resetAllFaults();
        }
        break;
    }
  }
}

void updateChannel(Channel *ch) {
  // Check if channel should be enabled
  if (ch->enabled && !ch->permanentDisable) {
    digitalWrite(ch->controlPin, HIGH);  // PROFET handles auto-retry in hardware

    // Monitor for overcurrent (software layer)
    if (ch->current > ch->currentLimit) {
      ch->faulted = true;
      ch->faultCount++;
      ch->lastFaultTime = millis();

      Serial.print(ch->name);
      Serial.print(F(" FAULT: "));
      Serial.print(ch->current);
      Serial.println(F("A"));

      // Check for hard fault (too many retries)
      if (ch->faultCount >= ch->MAX_FAULTS &&
          (millis() - ch->lastFaultTime < ch->FAULT_WINDOW)) {
        ch->permanentDisable = true;
        digitalWrite(ch->controlPin, LOW);  // Force OFF

        Serial.print(ch->name);
        Serial.println(F(" PERMANENT DISABLE"));
      }
    } else {
      ch->faulted = false;
    }
  } else {
    digitalWrite(ch->controlPin, LOW);
  }

  // Reset fault count after stable operation
  if (!ch->faulted && (millis() - ch->lastFaultTime > ch->FAULT_RESET)) {
    ch->faultCount = 0;
  }
}

float readCurrent(int channel) {
  // Read ADC (10-bit on Nano Every)
  int adcValue = analogRead(channels[channel].sensePin);

  // Convert to voltage (0-5V reference)
  float senseVoltage = (adcValue / 1023.0) * 5.0;

  // PROFET current sense with 10k pull-down
  // IS pin sources current proportional to load current (kILIS ratio)
  // Current through 10k resistor = voltage / 10k
  float senseCurrent = senseVoltage / 10000.0;  // Amps

  // Load current = sense current × kILIS ratio
  float loadCurrent = senseCurrent * channels[channel].kILIS;

  return loadCurrent;
}

void sendCANStatus() {
  byte msg[8] = {0};

  // Byte 0: Channel status bits (ON = 1, OFF = 0)
  for (int i = 0; i < 8; i++) {
    if (digitalRead(channels[i].controlPin) == HIGH) {
      msg[0] |= (1 << i);
    }
  }

  // Byte 1-4: Current readings (0.1A resolution, 0-25.5A range)
  for (int i = 0; i < 4; i++) {
    msg[1 + i] = (byte)(channels[i].current * 10.0);
  }

  // Byte 5: Fault flags
  for (int i = 0; i < 8; i++) {
    if (channels[i].faulted || channels[i].permanentDisable) {
      msg[5] |= (1 << i);
    }
  }

  // Byte 6-7: Total current (0.1A resolution, 0-6553.5A range)
  float totalCurrent = 0;
  for (int i = 0; i < 8; i++) {
    totalCurrent += channels[i].current;
  }
  int totalCurrentScaled = (int)(totalCurrent * 10.0);
  msg[6] = highByte(totalCurrentScaled);
  msg[7] = lowByte(totalCurrentScaled);

  CAN0.sendMsgBuf(CAN_PDM_STATUS, 0, 8, msg);
}

void sendCANTelemetry() {
  byte msg[8] = {0};

  // Byte 0-3: Uptime (seconds)
  msg[0] = (uptime >> 24) & 0xFF;
  msg[1] = (uptime >> 16) & 0xFF;
  msg[2] = (uptime >> 8) & 0xFF;
  msg[3] = uptime & 0xFF;

  // Byte 4-5: Battery voltage (0.01V resolution, 0-655.35V)
  int voltageScaled = (int)(batteryVoltage * 100.0);
  msg[4] = highByte(voltageScaled);
  msg[5] = lowByte(voltageScaled);

  // Byte 6-7: Reserved (future: temperature, energy consumption)

  CAN0.sendMsgBuf(CAN_PDM_TELEMETRY, 0, 8, msg);
}

void resetAllFaults() {
  for (int i = 0; i < 8; i++) {
    channels[i].faulted = false;
    channels[i].permanentDisable = false;
    channels[i].faultCount = 0;
  }
  Serial.println(F("All faults reset"));
}

// Serial Commands (for debugging/testing)
void serialEvent() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.startsWith("ENABLE ")) {
      int ch = cmd.substring(7).toInt() - 1;
      if (ch >= 0 && ch < 8) {
        channels[ch].enabled = true;
        Serial.print(F("Enabled: "));
        Serial.println(channels[ch].name);
      }
    }
    else if (cmd.startsWith("DISABLE ")) {
      int ch = cmd.substring(8).toInt() - 1;
      if (ch >= 0 && ch < 8) {
        channels[ch].enabled = false;
        Serial.print(F("Disabled: "));
        Serial.println(channels[ch].name);
      }
    }
    else if (cmd == "STATUS") {
      printStatus();
    }
    else if (cmd == "RESET") {
      resetAllFaults();
    }
  }
}

void printStatus() {
  Serial.println(F("\n=== PDM V2 Status ==="));
  Serial.print(F("Uptime: "));
  Serial.print(uptime);
  Serial.println(F("s"));

  for (int i = 0; i < 8; i++) {
    Serial.print(F("CH"));
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.print(channels[i].name);
    Serial.print(F(" - "));
    Serial.print(channels[i].enabled ? "ON " : "OFF");
    Serial.print(F(" - "));
    Serial.print(channels[i].current, 2);
    Serial.print(F("A"));
    if (channels[i].faulted) Serial.print(F(" [FAULT]"));
    if (channels[i].permanentDisable) Serial.print(F(" [PERM_DISABLE]"));
    Serial.println();
  }
  Serial.println();
}
