/*
 * RX8 CAN Decoder Example
 *
 * Demonstrates how to use the RX8_CAN_Messages library to receive
 * and decode all major CAN bus messages from a Mazda RX8.
 *
 * Hardware Required:
 * - Arduino Uno, Nano, Leonardo, or Mega
 * - MCP2515 CAN bus module
 * - Connection to RX8 CAN bus (500 kbps)
 *
 * Wiring (MCP2515 to Arduino):
 * - VCC → 5V
 * - GND → GND
 * - CS  → Pin 10 (or change CAN_CS define)
 * - SI  → Pin 11 (MOSI)
 * - SO  → Pin 12 (MISO)
 * - SCK → Pin 13
 * - INT → Pin 2 (or change CAN_INT define)
 *
 * Repository: https://github.com/michaelprowacki/MazdaRX8Arduino
 * License: MIT
 */

#include <mcp_can.h>
#include <SPI.h>
#include "RX8_CAN_Messages.h"

// CAN bus configuration
#define CAN_CS 10
#define CAN_INT 2

// Create CAN and decoder objects
MCP_CAN CAN0(CAN_CS);
RX8_CAN_Decoder decoder;

// Timing variables
unsigned long lastPrint = 0;
const unsigned long PRINT_INTERVAL = 500;  // Print every 500ms

void setup() {
  Serial.begin(115200);
  Serial.println("RX8 CAN Decoder Example");
  Serial.println("=======================");

  // Initialize MCP2515 at 500 kbps
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
    Serial.println("CAN Bus Initialized Successfully!");
  } else {
    Serial.println("ERROR: CAN Bus Init Failed");
    Serial.println("Check wiring and MCP2515 module");
    while(1);  // Halt if CAN init fails
  }

  // Set to normal mode (required after initialization)
  CAN0.setMode(MCP_NORMAL);

  // Configure interrupt pin
  pinMode(CAN_INT, INPUT);

  Serial.println("Listening for CAN messages...");
  Serial.println();
}

void loop() {
  // Read all available CAN messages
  readAllCANMessages();

  // Print decoded data periodically
  if (millis() - lastPrint >= PRINT_INTERVAL) {
    printDecodedData();
    lastPrint = millis();
  }
}

/*
 * Read all available CAN messages and decode them
 */
void readAllCANMessages() {
  long unsigned int rxId;
  unsigned char len = 0;
  unsigned char rxBuf[8];

  // Check if CAN interrupt pin is low (message available)
  if (!digitalRead(CAN_INT)) {
    // Read the message
    if (CAN0.readMsgBuf(&rxId, &len, rxBuf) == CAN_OK) {
      // Decode based on CAN ID
      switch(rxId) {
        case CAN_ID_PCM_STATUS:           // 0x201
          decoder.decode0x201(rxBuf);
          break;

        case CAN_ID_WARNING_LIGHTS:       // 0x420
          decoder.decode0x420(rxBuf);
          break;

        case CAN_ID_WHEEL_SPEEDS_ABS:     // 0x4B0
          decoder.decode0x4B0(rxBuf);
          break;

        case CAN_ID_WHEEL_SPEEDS_DASH:    // 0x4B1
          decoder.decode0x4B1(rxBuf);
          break;

        case CAN_ID_STEERING_ANGLE:       // 0x4BE
          decoder.decode0x4BE(rxBuf);
          break;

        case CAN_ID_ACCELEROMETER:        // 0x075
          decoder.decode0x075(rxBuf);
          break;

        case CAN_ID_ABS_DATA:             // 0x620
          decoder.decode0x620(rxBuf);
          break;

        case CAN_ID_ABS_CONFIG:           // 0x630
          decoder.decode0x630(rxBuf);
          break;

        case CAN_ID_DSC_ABS:              // 0x212
          decoder.decode0x212(rxBuf);
          break;

        default:
          // Unknown message - optionally print for debugging
          // printRawMessage(rxId, rxBuf, len);
          break;
      }
    }
  }
}

/*
 * Print all decoded data in a formatted way
 */
void printDecodedData() {
  Serial.println("================================");
  Serial.println("RX8 CAN Bus Data");
  Serial.println("================================");

  // PCM Status (0x201)
  Serial.println("--- PCM Status ---");
  Serial.print("Engine RPM:     ");
  Serial.print(decoder.pcmStatus.engineRPM);
  Serial.println(" RPM");

  Serial.print("Vehicle Speed:  ");
  Serial.print(decoder.pcmStatus.vehicleSpeed);
  Serial.println(" MPH");

  Serial.print("Throttle Pos:   ");
  Serial.print(decoder.pcmStatus.throttlePosition);
  Serial.println(" %");

  // Warning Lights (0x420)
  Serial.println("\n--- Engine Status ---");
  Serial.print("Coolant Temp:   ");
  Serial.print(decoder.warningLights.coolantTemperature);
  Serial.print(" (raw) / ");
  Serial.print(decoder.tempToCelsius(decoder.warningLights.coolantTemperature));
  Serial.print("°C / ");
  Serial.print(decoder.tempToFahrenheit(decoder.warningLights.coolantTemperature));
  Serial.println("°F");

  Serial.print("Temp Status:    ");
  if (decoder.isCoolantTempNormal()) {
    Serial.println("NORMAL");
  } else {
    Serial.println("OUT OF RANGE");
  }

  Serial.print("Oil Pressure:   ");
  Serial.println(decoder.warningLights.oilPressureOK ? "OK" : "LOW");

  // Warning Lights
  Serial.println("\n--- Warning Lights ---");
  if (decoder.hasActiveWarnings()) {
    if (decoder.warningLights.checkEngineMIL) {
      Serial.println("⚠️  CHECK ENGINE LIGHT");
    }
    if (decoder.warningLights.lowCoolantMIL) {
      Serial.println("⚠️  LOW COOLANT");
    }
    if (decoder.warningLights.batteryChargeMIL) {
      Serial.println("⚠️  BATTERY CHARGE");
    }
    if (decoder.warningLights.oilPressureMIL) {
      Serial.println("⚠️  OIL PRESSURE");
    }
    if (decoder.warningLights.engineOverheat) {
      Serial.println("⚠️  ENGINE OVERHEAT");
    }
    if (decoder.warningLights.catalystTempHigh) {
      Serial.println("⚠️  CATALYST TEMP HIGH");
    }
  } else {
    Serial.println("No warnings active");
  }

  // Wheel Speeds (0x4B1)
  Serial.println("\n--- Wheel Speeds ---");
  Serial.print("Front Left:     ");
  Serial.print(decoder.wheelSpeeds.frontLeft / 100);
  Serial.print(" kph (");
  Serial.print(decoder.getWheelSpeedMPH(decoder.wheelSpeeds.frontLeft));
  Serial.println(" mph)");

  Serial.print("Front Right:    ");
  Serial.print(decoder.wheelSpeeds.frontRight / 100);
  Serial.print(" kph (");
  Serial.print(decoder.getWheelSpeedMPH(decoder.wheelSpeeds.frontRight));
  Serial.println(" mph)");

  Serial.print("Rear Left:      ");
  Serial.print(decoder.wheelSpeeds.rearLeft / 100);
  Serial.println(" kph");

  Serial.print("Rear Right:     ");
  Serial.print(decoder.wheelSpeeds.rearRight / 100);
  Serial.println(" kph");

  Serial.print("Average Speed:  ");
  Serial.print(decoder.wheelSpeeds.averageFront / 100);
  Serial.println(" kph");

  if (decoder.wheelSpeeds.wheelSpeedMismatch) {
    Serial.println("⚠️  WHEEL SPEED MISMATCH DETECTED");
  }

  // Steering Angle (0x4BE)
  Serial.println("\n--- Steering ---");
  Serial.print("Steering Angle: ");
  Serial.print(decoder.steeringAngle.steeringAngle, 1);
  Serial.println("°");

  Serial.print("Centered:       ");
  Serial.println(decoder.steeringAngle.centerCalibrated ? "YES" : "NO");

  // Accelerometer (0x075)
  Serial.println("\n--- G-Forces ---");
  Serial.print("Lateral:        ");
  Serial.print(decoder.accelerometer.lateralG, 2);
  Serial.println("G (left/right)");

  Serial.print("Longitudinal:   ");
  Serial.print(decoder.accelerometer.longitudinalG, 2);
  Serial.println("G (forward/back)");

  Serial.print("Vertical:       ");
  Serial.print(decoder.accelerometer.verticalG, 2);
  Serial.println("G (up/down)");

  Serial.println("================================\n");
}

/*
 * Optional: Print raw CAN message for debugging unknown messages
 */
void printRawMessage(long unsigned int id, unsigned char buf[], unsigned char len) {
  Serial.print("Unknown CAN ID: 0x");
  Serial.print(id, HEX);
  Serial.print(" [");
  Serial.print(len);
  Serial.print("] ");

  for (int i = 0; i < len; i++) {
    if (buf[i] < 0x10) Serial.print("0");
    Serial.print(buf[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}
