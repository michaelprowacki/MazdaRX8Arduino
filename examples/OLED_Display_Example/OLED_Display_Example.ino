/*
 * RX8 OLED Display Example
 *
 * Demonstrates displaying RX8 CAN data on multiple OLED displays
 * Shows RPM, Speed, Temperature, Throttle, and Warnings
 *
 * Hardware Required:
 * - Arduino Uno or Nano
 * - MCP2515 CAN bus module
 * - 3x SSD1306 OLED displays (128x64, I2C or SPI)
 * - Connection to RX8 CAN bus (500 kbps)
 *
 * Display Layout:
 * - Display 1: RPM and Speed
 * - Display 2: Coolant Temperature and Throttle
 * - Display 3: Warning Lights
 *
 * Repository: https://github.com/michaelprowacki/MazdaRX8Arduino
 * License: MIT
 */

#include <mcp_can.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "RX8_CAN_Messages.h"

// CAN bus configuration
#define CAN_CS 10
#define CAN_INT 2

// OLED display configuration (I2C)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Create display objects (different I2C addresses)
Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_SSD1306 display3(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// I2C addresses for displays
#define DISPLAY1_ADDR 0x3C
#define DISPLAY2_ADDR 0x3D
#define DISPLAY3_ADDR 0x3E

// Create CAN and decoder objects
MCP_CAN CAN0(CAN_CS);
RX8_CAN_Decoder decoder;

// Timing
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 100;  // Update displays at 10Hz

void setup() {
  Serial.begin(115200);
  Serial.println("RX8 OLED Display Example");

  // Initialize CAN bus
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
    Serial.println("CAN: OK");
  } else {
    Serial.println("CAN: FAILED");
    while(1);
  }
  CAN0.setMode(MCP_NORMAL);
  pinMode(CAN_INT, INPUT);

  // Initialize OLED displays
  if (!display1.begin(SSD1306_SWITCHCAPVCC, DISPLAY1_ADDR)) {
    Serial.println("Display 1: FAILED");
  } else {
    Serial.println("Display 1: OK");
  }

  if (!display2.begin(SSD1306_SWITCHCAPVCC, DISPLAY2_ADDR)) {
    Serial.println("Display 2: FAILED");
  } else {
    Serial.println("Display 2: OK");
  }

  if (!display3.begin(SSD1306_SWITCHCAPVCC, DISPLAY3_ADDR)) {
    Serial.println("Display 3: FAILED");
  } else {
    Serial.println("Display 3: OK");
  }

  // Show startup message
  showStartupMessage();
  delay(2000);
}

void loop() {
  // Read CAN messages
  readCANMessages();

  // Update displays periodically
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    updateAllDisplays();
    lastUpdate = millis();
  }
}

void readCANMessages() {
  long unsigned int rxId;
  unsigned char len = 0;
  unsigned char rxBuf[8];

  while (!digitalRead(CAN_INT)) {
    if (CAN0.readMsgBuf(&rxId, &len, rxBuf) == CAN_OK) {
      switch(rxId) {
        case CAN_ID_PCM_STATUS:
          decoder.decode0x201(rxBuf);
          break;
        case CAN_ID_WARNING_LIGHTS:
          decoder.decode0x420(rxBuf);
          break;
        case CAN_ID_WHEEL_SPEEDS_DASH:
          decoder.decode0x4B1(rxBuf);
          break;
      }
    }
  }
}

void updateAllDisplays() {
  updateDisplay1();  // RPM and Speed
  updateDisplay2();  // Temperature and Throttle
  updateDisplay3();  // Warning Lights
}

/*
 * Display 1: RPM and Speed
 */
void updateDisplay1() {
  display1.clearDisplay();
  display1.setTextColor(WHITE);

  // RPM (large text)
  display1.setTextSize(3);
  display1.setCursor(0, 0);
  display1.print(decoder.pcmStatus.engineRPM);

  display1.setTextSize(1);
  display1.setCursor(95, 5);
  display1.print("RPM");

  // Horizontal line
  display1.drawLine(0, 30, 128, 30, WHITE);

  // Speed (large text)
  display1.setTextSize(3);
  display1.setCursor(0, 35);
  display1.print(decoder.pcmStatus.vehicleSpeed);

  display1.setTextSize(1);
  display1.setCursor(95, 50);
  display1.print("MPH");

  display1.display();
}

/*
 * Display 2: Coolant Temperature and Throttle
 */
void updateDisplay2() {
  display2.clearDisplay();
  display2.setTextColor(WHITE);

  // Coolant Temperature
  display2.setTextSize(1);
  display2.setCursor(0, 0);
  display2.print("COOLANT");

  display2.setTextSize(2);
  display2.setCursor(0, 12);
  int tempC = decoder.tempToCelsius(decoder.warningLights.coolantTemperature);
  display2.print(tempC);
  display2.print((char)247);  // Degree symbol
  display2.print("C");

  // Temperature bar graph
  int tempBarWidth = map(tempC, 50, 110, 0, 128);
  tempBarWidth = constrain(tempBarWidth, 0, 128);
  display2.fillRect(0, 32, tempBarWidth, 4, WHITE);

  // Horizontal line
  display2.drawLine(0, 40, 128, 40, WHITE);

  // Throttle Position
  display2.setTextSize(1);
  display2.setCursor(0, 44);
  display2.print("THROTTLE");

  display2.setTextSize(2);
  display2.setCursor(0, 54);
  display2.print(decoder.pcmStatus.throttlePosition);
  display2.print("%");

  // Throttle bar graph
  int throttleBarWidth = map(decoder.pcmStatus.throttlePosition, 0, 100, 0, 128);
  display2.drawRect(0, 48, 128, 6, WHITE);
  display2.fillRect(0, 48, throttleBarWidth, 6, WHITE);

  display2.display();
}

/*
 * Display 3: Warning Lights and Status
 */
void updateDisplay3() {
  display3.clearDisplay();
  display3.setTextColor(WHITE);
  display3.setTextSize(1);

  display3.setCursor(0, 0);
  display3.print("WARNINGS");
  display3.drawLine(0, 10, 128, 10, WHITE);

  int yPos = 15;

  if (decoder.hasActiveWarnings()) {
    // Show active warnings
    if (decoder.warningLights.checkEngineMIL) {
      display3.setCursor(0, yPos);
      display3.print("! CHECK ENGINE");
      yPos += 10;
    }
    if (decoder.warningLights.lowCoolantMIL) {
      display3.setCursor(0, yPos);
      display3.print("! LOW COOLANT");
      yPos += 10;
    }
    if (decoder.warningLights.batteryChargeMIL) {
      display3.setCursor(0, yPos);
      display3.print("! BATTERY");
      yPos += 10;
    }
    if (decoder.warningLights.oilPressureMIL) {
      display3.setCursor(0, yPos);
      display3.print("! OIL PRESSURE");
      yPos += 10;
    }
    if (decoder.warningLights.engineOverheat) {
      display3.setCursor(0, yPos);
      display3.print("! OVERHEAT");
      yPos += 10;
    }
  } else {
    // All OK
    display3.setTextSize(2);
    display3.setCursor(20, 28);
    display3.print("ALL OK");
  }

  // Oil pressure indicator
  display3.setTextSize(1);
  display3.setCursor(0, 56);
  display3.print("OIL: ");
  display3.print(decoder.warningLights.oilPressureOK ? "OK" : "LOW");

  // Coolant temp indicator
  display3.setCursor(70, 56);
  display3.print("TEMP: ");
  display3.print(decoder.isCoolantTempNormal() ? "OK" : "!!");

  display3.display();
}

void showStartupMessage() {
  // Display 1
  display1.clearDisplay();
  display1.setTextSize(2);
  display1.setTextColor(WHITE);
  display1.setCursor(10, 20);
  display1.print("RX8 CAN");
  display1.setCursor(10, 40);
  display1.print("Display");
  display1.display();

  // Display 2
  display2.clearDisplay();
  display2.setTextSize(1);
  display2.setTextColor(WHITE);
  display2.setCursor(20, 28);
  display2.print("Initializing...");
  display2.display();

  // Display 3
  display3.clearDisplay();
  display3.setTextSize(1);
  display3.setTextColor(WHITE);
  display3.setCursor(10, 20);
  display3.print("Listening for");
  display3.setCursor(10, 35);
  display3.print("CAN messages");
  display3.display();
}
