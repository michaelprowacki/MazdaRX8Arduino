/*
 * RX8 BCM Discovery Sniffer
 *
 * Purpose: Discover unknown CAN bus messages for BCM (Body Control Module) functions
 * Target functions: Door locks, power windows, interior lights, trunk release
 *
 * Methodology:
 * 1. Baseline Capture: Record normal CAN traffic
 * 2. Function Trigger: Activate function (lock doors) and observe new/changed messages
 * 3. Message Replay: Send discovered message to verify it works
 *
 * Hardware: Arduino Leonardo + MCP2515 CAN module
 * CAN Bus: 500 kbps (High Speed CAN)
 * Connection: OBD2 pins 6 (CANH) and 14 (CANL)
 *
 * Based on: docs/RX8_BCM_CANBUS_RESEARCH.md
 * Author: AI Assistant + Community
 * Date: 2025-11-17
 *
 * USAGE:
 * 1. Upload to Arduino
 * 2. Connect to OBD2 port (engine can be off or on)
 * 3. Open Serial Monitor (115200 baud)
 * 4. Commands:
 *    - 'b': Start baseline capture (10 seconds)
 *    - 's': Start sniffing mode (continuous)
 *    - 'd': Dump message buffer
 *    - 'f': Filter mode (show only new/changed messages)
 *    - 'r': Replay mode (send test message)
 *    - 'c': Clear buffer
 *    - 'h': Help
 */

#include <Arduino.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>

// ========== CONFIGURATION ==========
#define CAN_CS_PIN 17           // CS pin for MCP2515
#define CAN_INT_PIN 2           // Interrupt pin
#define LED_PIN LED_BUILTIN     // Status LED
#define SERIAL_BAUD 115200      // Serial monitor baud rate

// Message buffer settings
#define MAX_MESSAGES 100        // Maximum unique CAN IDs to track
#define MAX_SAMPLES 10          // Samples per message for averaging

// ========== GLOBAL VARIABLES ==========
MCP_CAN CAN0(CAN_CS_PIN);

// Operating modes
enum Mode {
  MODE_IDLE,
  MODE_BASELINE,
  MODE_SNIFF,
  MODE_FILTER,
  MODE_REPLAY
};

Mode currentMode = MODE_IDLE;

// CAN message structure
struct CANMessage {
  unsigned long id;             // CAN message ID
  byte len;                     // Message length
  byte data[8];                 // Message data
  unsigned long timestamp;      // Last seen timestamp
  unsigned long count;          // Number of times seen
  unsigned long interval;       // Average interval between messages (ms)
  boolean isNew;                // New message (not in baseline)
  boolean hasChanged;           // Data changed since baseline
};

// Message database
CANMessage messageDB[MAX_MESSAGES];
int messageCount = 0;

// Baseline database (for comparison)
CANMessage baselineDB[MAX_MESSAGES];
int baselineCount = 0;

// Statistics
unsigned long totalMessages = 0;
unsigned long unknownMessages = 0;
unsigned long startTime = 0;

// ========== SETUP ==========
void setup() {
  Serial.begin(SERIAL_BAUD);
  while (!Serial); // Wait for serial connection

  pinMode(LED_PIN, OUTPUT);
  pinMode(CAN_INT_PIN, INPUT);

  Serial.println(F("========================================"));
  Serial.println(F("  RX8 BCM Discovery Sniffer v1.0"));
  Serial.println(F("========================================"));
  Serial.println();

  // Initialize CAN bus
  Serial.print(F("Initializing CAN bus at 500 kbps... "));
  if (CAN0.begin(CAN_500KBPS) == CAN_OK) {
    Serial.println(F("OK"));
  } else {
    Serial.println(F("FAILED"));
    Serial.println(F("ERROR: Could not initialize CAN bus!"));
    Serial.println(F("Check wiring:"));
    Serial.println(F("  - MCP2515 CS  -> Pin 17"));
    Serial.println(F("  - MCP2515 INT -> Pin 2"));
    Serial.println(F("  - MCP2515 VCC -> 5V"));
    Serial.println(F("  - MCP2515 GND -> GND"));
    while (1) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(200);
    }
  }

  Serial.println();
  printHelp();
}

// ========== MAIN LOOP ==========
void loop() {
  // Check for serial commands
  if (Serial.available()) {
    char cmd = Serial.read();
    handleCommand(cmd);
  }

  // Process CAN messages based on current mode
  if (currentMode != MODE_IDLE && currentMode != MODE_REPLAY) {
    if (CAN_MSGAVAIL == CAN0.checkReceive()) {
      processingCANMessage();
    }
  }

  // Mode-specific behavior
  switch (currentMode) {
    case MODE_BASELINE:
      // Check if baseline capture time expired (10 seconds)
      if (millis() - startTime > 10000) {
        finishBaseline();
      }
      break;

    case MODE_SNIFF:
    case MODE_FILTER:
      // Blink LED to show activity
      if (totalMessages % 100 == 0) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      }
      break;

    default:
      break;
  }
}

// ========== CAN MESSAGE PROCESSING ==========
void processingCANMessage() {
  unsigned long id;
  byte len;
  byte buf[8];

  CAN0.readMsgBufID(&id, &len, buf);
  totalMessages++;

  // Find or create message entry
  int index = findOrCreateMessage(id, len, buf);

  if (index >= 0) {
    // Update message data
    CANMessage* msg = &messageDB[index];

    // Check if data changed
    boolean dataChanged = false;
    for (int i = 0; i < len; i++) {
      if (msg->data[i] != buf[i]) {
        dataChanged = true;
        msg->data[i] = buf[i];
      }
    }

    // Update statistics
    unsigned long currentTime = millis();
    if (msg->count > 0) {
      unsigned long newInterval = currentTime - msg->timestamp;
      msg->interval = (msg->interval * (msg->count - 1) + newInterval) / msg->count;
    }
    msg->timestamp = currentTime;
    msg->count++;

    // Check against baseline
    if (currentMode == MODE_FILTER) {
      msg->isNew = !isInBaseline(id);
      msg->hasChanged = dataChanged && !msg->isNew;
    }

    // Print message based on mode
    if (currentMode == MODE_SNIFF) {
      printMessage(msg, false);  // Print all messages
    } else if (currentMode == MODE_FILTER) {
      if (msg->isNew || msg->hasChanged) {
        printMessage(msg, true);  // Print only new/changed
      }
    }
  }
}

// ========== MESSAGE DATABASE MANAGEMENT ==========
int findOrCreateMessage(unsigned long id, byte len, byte buf[]) {
  // Find existing message
  for (int i = 0; i < messageCount; i++) {
    if (messageDB[i].id == id) {
      return i;
    }
  }

  // Create new message entry (if space available)
  if (messageCount < MAX_MESSAGES) {
    CANMessage* msg = &messageDB[messageCount];
    msg->id = id;
    msg->len = len;
    for (int i = 0; i < len; i++) {
      msg->data[i] = buf[i];
    }
    msg->timestamp = millis();
    msg->count = 1;
    msg->interval = 0;
    msg->isNew = false;
    msg->hasChanged = false;

    messageCount++;
    return messageCount - 1;
  }

  // Buffer full
  return -1;
}

boolean isInBaseline(unsigned long id) {
  for (int i = 0; i < baselineCount; i++) {
    if (baselineDB[i].id == id) {
      return true;
    }
  }
  return false;
}

// ========== COMMAND HANDLING ==========
void handleCommand(char cmd) {
  switch (cmd) {
    case 'b':
    case 'B':
      startBaseline();
      break;

    case 's':
    case 'S':
      startSniffing();
      break;

    case 'f':
    case 'F':
      startFiltering();
      break;

    case 'd':
    case 'D':
      dumpBuffer();
      break;

    case 'c':
    case 'C':
      clearBuffer();
      break;

    case 'r':
    case 'R':
      enterReplayMode();
      break;

    case 'h':
    case 'H':
    case '?':
      printHelp();
      break;

    default:
      // Ignore unknown commands
      break;
  }
}

void startBaseline() {
  Serial.println(F("\n========== BASELINE CAPTURE =========="));
  Serial.println(F("Recording normal CAN traffic for 10 seconds..."));
  Serial.println(F("Keep vehicle in normal state (doors closed, windows up, lights off)"));
  Serial.println();

  clearBuffer();
  currentMode = MODE_BASELINE;
  startTime = millis();
  totalMessages = 0;
}

void finishBaseline() {
  currentMode = MODE_IDLE;

  // Copy current messages to baseline
  baselineCount = messageCount;
  for (int i = 0; i < messageCount; i++) {
    baselineDB[i] = messageDB[i];
  }

  Serial.println();
  Serial.println(F("========== BASELINE COMPLETE =========="));
  Serial.print(F("Captured "));
  Serial.print(baselineCount);
  Serial.print(F(" unique CAN IDs, "));
  Serial.print(totalMessages);
  Serial.println(F(" total messages"));
  Serial.println();
  Serial.println(F("Now you can:"));
  Serial.println(F("  1. Activate a function (lock doors, press trunk button, etc.)"));
  Serial.println(F("  2. Type 'f' to filter and show only NEW or CHANGED messages"));
  Serial.println();

  dumpBuffer();
}

void startSniffing() {
  Serial.println(F("\n========== SNIFF MODE =========="));
  Serial.println(F("Showing ALL CAN messages (press 'c' to stop)"));
  Serial.println(F("Format: ID (Hex) | ID (Dec) | Len | Data | Count | Interval"));
  Serial.println();

  clearBuffer();
  currentMode = MODE_SNIFF;
  totalMessages = 0;
}

void startFiltering() {
  if (baselineCount == 0) {
    Serial.println(F("\nERROR: No baseline captured! Run 'b' first."));
    return;
  }

  Serial.println(F("\n========== FILTER MODE =========="));
  Serial.println(F("Showing only NEW or CHANGED messages (press 'c' to stop)"));
  Serial.println(F("Format: ID (Hex) | ID (Dec) | Len | Data | Status"));
  Serial.println();

  // Keep current buffer but reset counts
  for (int i = 0; i < messageCount; i++) {
    messageDB[i].count = 0;
    messageDB[i].isNew = false;
    messageDB[i].hasChanged = false;
  }

  currentMode = MODE_FILTER;
  totalMessages = 0;
}

void dumpBuffer() {
  Serial.println(F("\n========== MESSAGE BUFFER DUMP =========="));
  Serial.print(F("Total unique messages: "));
  Serial.println(messageCount);
  Serial.println();
  Serial.println(F("ID (Hex) | ID (Dec) | Len | Data                   | Count  | Interval"));
  Serial.println(F("---------|----------|-----|------------------------|--------|----------"));

  for (int i = 0; i < messageCount; i++) {
    printMessage(&messageDB[i], false);
  }

  Serial.println();
}

void clearBuffer() {
  messageCount = 0;
  totalMessages = 0;
  currentMode = MODE_IDLE;
  Serial.println(F("Buffer cleared"));
}

void enterReplayMode() {
  Serial.println(F("\n========== REPLAY MODE =========="));
  Serial.println(F("Enter CAN message to send:"));
  Serial.println(F("Format: ID,Len,B0,B1,B2,B3,B4,B5,B6,B7"));
  Serial.println(F("Example: 3B5,8,01,00,00,00,00,00,00,00"));
  Serial.println(F("(Type 'c' to cancel)"));
  Serial.println();

  currentMode = MODE_REPLAY;

  // Wait for input (timeout after 30 seconds)
  unsigned long startWait = millis();
  String input = "";

  while (currentMode == MODE_REPLAY) {
    if (Serial.available()) {
      char c = Serial.read();

      if (c == 'c' || c == 'C') {
        Serial.println(F("Replay canceled"));
        currentMode = MODE_IDLE;
        return;
      } else if (c == '\n') {
        // Process input
        replayMessage(input);
        currentMode = MODE_IDLE;
        return;
      } else if (c != '\r') {
        input += c;
      }
    }

    // Timeout after 30 seconds
    if (millis() - startWait > 30000) {
      Serial.println(F("Timeout - replay canceled"));
      currentMode = MODE_IDLE;
      return;
    }
  }
}

void replayMessage(String input) {
  // Parse input: ID,Len,B0,B1,B2,B3,B4,B5,B6,B7
  int commaIndex[10];
  int commaCount = 0;

  for (unsigned int i = 0; i < input.length() && commaCount < 10; i++) {
    if (input.charAt(i) == ',') {
      commaIndex[commaCount++] = i;
    }
  }

  if (commaCount < 1) {
    Serial.println(F("ERROR: Invalid format (need at least ID,Len)"));
    return;
  }

  // Parse ID (hex)
  String idStr = input.substring(0, commaIndex[0]);
  unsigned long id = strtoul(idStr.c_str(), NULL, 16);

  // Parse length
  String lenStr = input.substring(commaIndex[0] + 1, commaIndex[1]);
  byte len = lenStr.toInt();

  if (len > 8) {
    Serial.println(F("ERROR: Length must be ≤ 8"));
    return;
  }

  // Parse data bytes (hex)
  byte buf[8] = {0};
  for (int i = 0; i < len && i + 2 <= commaCount; i++) {
    String byteStr = input.substring(commaIndex[i + 1] + 1, commaIndex[i + 2]);
    buf[i] = strtoul(byteStr.c_str(), NULL, 16);
  }

  // Send message
  Serial.print(F("Sending: 0x"));
  Serial.print(id, HEX);
  Serial.print(F(" ["));
  for (int i = 0; i < len; i++) {
    if (i > 0) Serial.print(F(" "));
    if (buf[i] < 16) Serial.print(F("0"));
    Serial.print(buf[i], HEX);
  }
  Serial.println(F("]"));

  byte result = CAN0.sendMsgBuf(id, 0, len, buf);

  if (result == CAN_OK) {
    Serial.println(F("SUCCESS: Message sent"));
  } else {
    Serial.print(F("ERROR: Send failed (code "));
    Serial.print(result);
    Serial.println(F(")"));
  }
}

// ========== OUTPUT FORMATTING ==========
void printMessage(CANMessage* msg, boolean showStatus) {
  // ID (Hex)
  Serial.print(F("0x"));
  if (msg->id < 0x100) Serial.print(F("0"));
  if (msg->id < 0x10) Serial.print(F("0"));
  Serial.print(msg->id, HEX);
  Serial.print(F(" | "));

  // ID (Dec)
  if (msg->id < 1000) Serial.print(F(" "));
  if (msg->id < 100) Serial.print(F(" "));
  if (msg->id < 10) Serial.print(F(" "));
  Serial.print(msg->id);
  Serial.print(F(" | "));

  // Length
  Serial.print(msg->len);
  Serial.print(F("   | "));

  // Data bytes
  for (int i = 0; i < 8; i++) {
    if (i < msg->len) {
      if (msg->data[i] < 16) Serial.print(F("0"));
      Serial.print(msg->data[i], HEX);
    } else {
      Serial.print(F("--"));
    }
    Serial.print(F(" "));
  }
  Serial.print(F("| "));

  // Count
  if (msg->count < 100000) Serial.print(F(" "));
  if (msg->count < 10000) Serial.print(F(" "));
  if (msg->count < 1000) Serial.print(F(" "));
  if (msg->count < 100) Serial.print(F(" "));
  if (msg->count < 10) Serial.print(F(" "));
  Serial.print(msg->count);
  Serial.print(F(" | "));

  // Interval (average time between messages)
  if (msg->interval > 0) {
    if (msg->interval < 10000) Serial.print(F(" "));
    if (msg->interval < 1000) Serial.print(F(" "));
    if (msg->interval < 100) Serial.print(F(" "));
    if (msg->interval < 10) Serial.print(F(" "));
    Serial.print(msg->interval);
    Serial.print(F(" ms"));
  } else {
    Serial.print(F("     N/A"));
  }

  // Status (only in filter mode)
  if (showStatus) {
    if (msg->isNew) {
      Serial.print(F(" [NEW]"));
    }
    if (msg->hasChanged) {
      Serial.print(F(" [CHANGED]"));
    }
  }

  Serial.println();
}

void printHelp() {
  Serial.println(F("========== COMMANDS =========="));
  Serial.println(F("b - Start baseline capture (10 seconds)"));
  Serial.println(F("s - Start sniffing (show ALL messages)"));
  Serial.println(F("f - Filter mode (show only NEW/CHANGED messages)"));
  Serial.println(F("d - Dump current message buffer"));
  Serial.println(F("c - Clear buffer and stop"));
  Serial.println(F("r - Replay mode (send test message)"));
  Serial.println(F("h - Show this help"));
  Serial.println();
  Serial.println(F("========== DISCOVERY WORKFLOW =========="));
  Serial.println(F("1. Type 'b' to capture baseline (10 sec)"));
  Serial.println(F("2. Wait for baseline to complete"));
  Serial.println(F("3. Perform action (lock doors, press button, etc.)"));
  Serial.println(F("4. Type 'f' to see NEW/CHANGED messages"));
  Serial.println(F("5. Note the CAN ID and data bytes"));
  Serial.println(F("6. Type 'r' to test replay message"));
  Serial.println(F("7. Type 'd' to dump full buffer"));
  Serial.println(F("=============================="));
  Serial.println();
}
