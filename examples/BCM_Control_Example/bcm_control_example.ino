/**
 * @file bcm_control_example.ino
 * @brief Example: Body Control Module (BCM) control via CAN bus
 *
 * This example demonstrates how to discover and control BCM functions
 * on the Mazda RX8 via CAN bus messages.
 *
 * Features demonstrated:
 * - CAN bus sniffing mode
 * - Message replay
 * - Door lock control (example - needs discovery)
 * - Power window control (example - needs discovery)
 * - Remote control via serial commands
 *
 * Hardware Required:
 * - Arduino Mega 2560
 * - MCP2515 CAN bus module
 * - Connection to RX8 OBD2 port
 *
 * Usage:
 * 1. Upload this sketch
 * 2. Open Serial Monitor (115200 baud)
 * 3. Choose mode: 'S' for sniff, 'T' for test control
 *
 * @author MazdaRX8Arduino Project
 * @date 2025-11-16
 */

#include <mcp_can.h>
#include <SPI.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

#define CAN_CS_PIN 10
#define CAN_INT_PIN 2

MCP_CAN CAN0(CAN_CS_PIN);

// ============================================================================
// BCM CAN MESSAGE IDs (HYPOTHETICAL - NEED DISCOVERY!)
// ============================================================================

// ⚠️ WARNING: These are PLACEHOLDER values for demonstration
// You MUST discover actual values for your RX8 using sniffing mode

const uint16_t CAN_ID_DOOR_LOCKS = 0x410;     // Hypothetical
const uint16_t CAN_ID_POWER_WINDOWS = 0x420;  // Hypothetical
const uint16_t CAN_ID_LIGHTING = 0x430;       // Hypothetical
const uint16_t CAN_ID_TRUNK = 0x440;          // Hypothetical

// ============================================================================
// OPERATING MODES
// ============================================================================

enum Mode {
  MODE_SNIFF,      // CAN bus sniffer (discovery mode)
  MODE_TEST,       // Test BCM control
  MODE_REMOTE      // Remote control via serial
};

Mode currentMode = MODE_SNIFF;

// ============================================================================
// CAN MESSAGE BUFFER
// ============================================================================

struct CANMessage {
  unsigned long id;
  byte len;
  byte data[8];
  unsigned long timestamp;
};

const int BUFFER_SIZE = 100;
CANMessage messageBuffer[BUFFER_SIZE];
int bufferIndex = 0;

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);

  // Wait for serial
  delay(1000);

  Serial.println(F("========================================"));
  Serial.println(F("  RX8 BCM Control Example"));
  Serial.println(F("========================================"));
  Serial.println();

  // Initialize CAN bus
  Serial.println(F("Initializing MCP2515 CAN controller..."));

  if(CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println(F("✓ CAN Init OK!"));
  } else {
    Serial.println(F("✗ CAN Init FAILED!"));
    Serial.println(F("Check wiring:"));
    Serial.println(F("  MCP2515 CS  → Pin 10"));
    Serial.println(F("  MCP2515 SI  → Pin 11 (MOSI)"));
    Serial.println(F("  MCP2515 SO  → Pin 12 (MISO)"));
    Serial.println(F("  MCP2515 SCK → Pin 13"));
    Serial.println(F("  MCP2515 INT → Pin 2"));
    while(1);  // Halt
  }

  CAN0.setMode(MCP_NORMAL);
  pinMode(CAN_INT_PIN, INPUT);

  Serial.println();
  printMenu();
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // Check for serial commands
  if(Serial.available()) {
    handleSerialCommand();
  }

  // Process based on mode
  switch(currentMode) {
    case MODE_SNIFF:
      sniffMode();
      break;

    case MODE_TEST:
      testMode();
      break;

    case MODE_REMOTE:
      remoteMode();
      break;
  }
}

// ============================================================================
// MENU
// ============================================================================

void printMenu() {
  Serial.println(F("========================================"));
  Serial.println(F("  MENU"));
  Serial.println(F("========================================"));
  Serial.println(F("S - Sniff Mode (discover CAN messages)"));
  Serial.println(F("T - Test Mode (test BCM control)"));
  Serial.println(F("R - Remote Mode (serial control)"));
  Serial.println(F("M - Show this menu"));
  Serial.println(F("C - Clear message buffer"));
  Serial.println(F("D - Dump message buffer"));
  Serial.println();
  Serial.println(F("⚠️  WARNING: Test mode sends messages!"));
  Serial.println(F("   Only use if you know what you're doing."));
  Serial.println(F("========================================"));
  Serial.println();
}

void handleSerialCommand() {
  char cmd = Serial.read();

  switch(cmd) {
    case 'S':
    case 's':
      currentMode = MODE_SNIFF;
      Serial.println(F("\n→ SNIFF MODE"));
      Serial.println(F("Listening for CAN messages..."));
      Serial.println(F("Try triggering BCM functions (lock doors, etc.)"));
      Serial.println();
      break;

    case 'T':
    case 't':
      currentMode = MODE_TEST;
      Serial.println(F("\n→ TEST MODE"));
      Serial.println(F("⚠️  This will send CAN messages!"));
      Serial.println(F("Commands:"));
      Serial.println(F("  1 - Lock all doors"));
      Serial.println(F("  2 - Unlock all doors"));
      Serial.println(F("  3 - Driver window up"));
      Serial.println(F("  4 - Driver window down"));
      Serial.println(F("  0 - Back to menu"));
      Serial.println();
      break;

    case 'R':
    case 'r':
      currentMode = MODE_REMOTE;
      Serial.println(F("\n→ REMOTE CONTROL MODE"));
      Serial.println(F("Send commands via serial"));
      Serial.println();
      break;

    case 'M':
    case 'm':
      printMenu();
      break;

    case 'C':
    case 'c':
      bufferIndex = 0;
      Serial.println(F("✓ Message buffer cleared"));
      break;

    case 'D':
    case 'd':
      dumpMessageBuffer();
      break;

    // Test mode commands
    case '1':
      if(currentMode == MODE_TEST) lockAllDoors();
      break;
    case '2':
      if(currentMode == MODE_TEST) unlockAllDoors();
      break;
    case '3':
      if(currentMode == MODE_TEST) driverWindowUp();
      break;
    case '4':
      if(currentMode == MODE_TEST) driverWindowDown();
      break;
    case '0':
      printMenu();
      break;
  }
}

// ============================================================================
// SNIFF MODE - Discover CAN Messages
// ============================================================================

void sniffMode() {
  static unsigned long lastMessageTime = 0;
  static int messageCount = 0;

  if(!digitalRead(CAN_INT_PIN)) {  // CAN message available
    unsigned long rxId;
    byte len;
    byte rxBuf[8];

    CAN0.readMsgBuf(&rxId, &len, rxBuf);

    // Store in buffer
    if(bufferIndex < BUFFER_SIZE) {
      messageBuffer[bufferIndex].id = rxId;
      messageBuffer[bufferIndex].len = len;
      memcpy(messageBuffer[bufferIndex].data, rxBuf, 8);
      messageBuffer[bufferIndex].timestamp = millis();
      bufferIndex++;
    }

    // Print message
    Serial.print(millis());
    Serial.print(F("\t0x"));
    Serial.print(rxId, HEX);
    Serial.print(F("\t["));
    Serial.print(len);
    Serial.print(F("]\t"));

    for(int i = 0; i < len; i++) {
      if(rxBuf[i] < 16) Serial.print(F("0"));
      Serial.print(rxBuf[i], HEX);
      Serial.print(F(" "));
    }

    Serial.println();

    messageCount++;
    lastMessageTime = millis();
  }

  // Print stats every 5 seconds
  static unsigned long lastStatsTime = 0;
  if(millis() - lastStatsTime > 5000) {
    Serial.print(F("Messages received: "));
    Serial.print(messageCount);
    Serial.print(F(" (buffer: "));
    Serial.print(bufferIndex);
    Serial.print(F("/"));
    Serial.print(BUFFER_SIZE);
    Serial.println(F(")"));

    lastStatsTime = millis();
  }
}

void dumpMessageBuffer() {
  Serial.println(F("\n========================================"));
  Serial.println(F("  MESSAGE BUFFER DUMP"));
  Serial.println(F("========================================"));
  Serial.println(F("Time(ms)\tID\t\tData"));

  for(int i = 0; i < bufferIndex; i++) {
    Serial.print(messageBuffer[i].timestamp);
    Serial.print(F("\t\t0x"));
    Serial.print(messageBuffer[i].id, HEX);
    Serial.print(F("\t"));

    for(int j = 0; j < messageBuffer[i].len; j++) {
      if(messageBuffer[i].data[j] < 16) Serial.print(F("0"));
      Serial.print(messageBuffer[i].data[j], HEX);
      Serial.print(F(" "));
    }

    Serial.println();
  }

  Serial.println(F("========================================"));
  Serial.println();
}

// ============================================================================
// TEST MODE - Test BCM Control
// ============================================================================

void testMode() {
  // Wait for serial commands (handled in handleSerialCommand)
  delay(10);
}

// ============================================================================
// BCM CONTROL FUNCTIONS (HYPOTHETICAL)
// ============================================================================

void lockAllDoors() {
  Serial.println(F("→ Sending: Lock all doors"));

  byte msg[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  byte result = CAN0.sendMsgBuf(CAN_ID_DOOR_LOCKS, 0, 8, msg);

  if(result == CAN_OK) {
    Serial.println(F("✓ Message sent"));
  } else {
    Serial.println(F("✗ Send failed"));
  }
}

void unlockAllDoors() {
  Serial.println(F("→ Sending: Unlock all doors"));

  byte msg[8] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  byte result = CAN0.sendMsgBuf(CAN_ID_DOOR_LOCKS, 0, 8, msg);

  if(result == CAN_OK) {
    Serial.println(F("✓ Message sent"));
  } else {
    Serial.println(F("✗ Send failed"));
  }
}

void driverWindowUp() {
  Serial.println(F("→ Sending: Driver window up"));

  byte msg[8] = {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  //            ↑     ↑
  //         Window  Action
  //         (0=DRV) (1=UP)

  byte result = CAN0.sendMsgBuf(CAN_ID_POWER_WINDOWS, 0, 8, msg);

  if(result == CAN_OK) {
    Serial.println(F("✓ Message sent"));
  } else {
    Serial.println(F("✗ Send failed"));
  }
}

void driverWindowDown() {
  Serial.println(F("→ Sending: Driver window down"));

  byte msg[8] = {0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  //            ↑     ↑
  //         Window  Action
  //         (0=DRV) (2=DOWN)

  byte result = CAN0.sendMsgBuf(CAN_ID_POWER_WINDOWS, 0, 8, msg);

  if(result == CAN_OK) {
    Serial.println(F("✓ Message sent"));
  } else {
    Serial.println(F("✗ Send failed"));
  }
}

// ============================================================================
// REMOTE MODE - Serial Control
// ============================================================================

void remoteMode() {
  // Remote control commands can be added here
  // Example: Parse JSON commands from serial
  delay(10);
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void printMessageStats() {
  // Analyze message buffer for patterns
  // Count unique IDs
  // Find frequently changing messages
  // etc.
}

/**
 * Compare two captures to find differences
 *
 * Usage:
 * 1. Capture baseline (normal operation)
 * 2. Clear buffer ('C')
 * 3. Trigger function (e.g., lock doors)
 * 4. Capture event
 * 5. Dump buffer ('D')
 * 6. Compare the two dumps manually or via script
 */
