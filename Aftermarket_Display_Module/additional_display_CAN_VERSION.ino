/*
 * RX8 Aftermarket Display - CAN Bus Version
 *
 * IMPROVED VERSION: Uses RX8_CAN_Messages library to read directly from ECU_Module
 * Much faster and more efficient than OBD-II polling!
 *
 * Features:
 * - Direct CAN bus reading (no OBD-II overhead)
 * - Real-time updates at 100ms (10Hz)
 * - Uses RX8_CAN_Messages decoder library
 * - 8 configurable display cells on RGB OLED
 * - Warning color indicators
 *
 * Hardware:
 * - Arduino Uno or Nano
 * - MCP2515 CAN module
 * - SSD1351 RGB OLED display (128x128)
 * - Connection to RX8 CAN bus
 *
 * Advantages over OBD2 version:
 * - 10x faster updates (100ms vs 1000ms+)
 * - No OBD-II polling overhead
 * - Reads same data as dashboard sees
 * - Works with ECU_Module or factory ECU
 *
 * Repository: https://github.com/michaelprowacki/MazdaRX8Arduino
 * Based on original by: Radivv
 * CAN integration: 2025-11-15
 */

#include <mcp_can.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include "RX8_CAN_Messages.h"  // Our new decoder library!

// CAN bus configuration
#define CAN_CS 10
#define CAN_INT 2

// Screen dimensions
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128

// OLED pin configuration
const uint8_t OLED_pin_scl_sck  = 13;
const uint8_t OLED_pin_sda_mosi = 11;
const uint8_t OLED_pin_cs_ss    = 6;
const uint8_t OLED_pin_res_rst  = 8;
const uint8_t OLED_pin_dc_rs    = 7;

// Colors
const uint16_t COLOR_BLACK = 0x0000;
const uint16_t COLOR_WHITE = 0xFFFF;
const uint16_t COLOR_RED   = 0xF800;

// CAN and decoder objects
MCP_CAN CAN0(CAN_CS);
RX8_CAN_Decoder decoder;

// Display object
Adafruit_SSD1351 oled = Adafruit_SSD1351(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &SPI,
    OLED_pin_cs_ss,
    OLED_pin_dc_rs,
    OLED_pin_res_rst
);

// Configuration - 8 display cells
enum DisplayValue {
    NOTHING,
    COOLANT_TEMP,
    VEHICLE_SPEED,
    ENGINE_RPM,
    THROTTLE_POS,
    OIL_PRESSURE,    // From 0x420 (OK/LOW status)
    VOLTAGE,         // Would need additional analog input or custom CAN message
    WHEEL_SPEED_FL,  // From 0x4B1
    WHEEL_SPEED_FR   // From 0x4B1
};

// Configure which values to display (matching 8 cells)
DisplayValue config_Display[9] = {
    NOTHING,         // Index 0 not used
    COOLANT_TEMP,    // Cell 1
    ENGINE_RPM,      // Cell 2
    VEHICLE_SPEED,   // Cell 3
    THROTTLE_POS,    // Cell 4
    OIL_PRESSURE,    // Cell 5
    WHEEL_SPEED_FL,  // Cell 6
    WHEEL_SPEED_FR,  // Cell 7
    COOLANT_TEMP     // Cell 8 (duplicate for demo)
};

// Warning thresholds
int coolantTempWarning = 95;   // Celsius
int oilPressureWarning = 1;    // 0 = warning

// Display state
bool celsiusMode = true;
bool metricMode = true;
unsigned long lastUpdate = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("RX8 Aftermarket Display - CAN Version");

    // Initialize OLED
    oled.begin();
    oled.setRotation(0);
    oled.fillScreen(COLOR_BLACK);
    oled.setTextColor(COLOR_WHITE);
    oled.setTextSize(1);

    // Show startup message
    oled.setCursor(10, 60);
    oled.println("Initializing...");

    // Initialize CAN bus
    if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
        Serial.println("CAN: OK");
        oled.setCursor(10, 70);
        oled.println("CAN: OK");
    } else {
        Serial.println("CAN: FAILED");
        oled.setCursor(10, 70);
        oled.setTextColor(COLOR_RED);
        oled.println("CAN: FAILED");
        while (1);  // Halt
    }

    CAN0.setMode(MCP_NORMAL);
    pinMode(CAN_INT, INPUT);

    delay(2000);

    // Setup display layout
    setupDisplay();
}

void loop() {
    // Read all available CAN messages
    readCANMessages();

    // Update display periodically
    if (millis() - lastUpdate >= 100) {  // 10Hz update rate
        updateDisplay();
        lastUpdate = millis();
    }
}

/*
 * Read all available CAN messages and decode them
 */
void readCANMessages() {
    long unsigned int rxId;
    unsigned char len = 0;
    unsigned char rxBuf[8];

    // Read all available messages
    while (!digitalRead(CAN_INT)) {
        if (CAN0.readMsgBuf(&rxId, &len, rxBuf) == CAN_OK) {
            // Decode based on message ID
            switch(rxId) {
                case CAN_ID_PCM_STATUS:  // 0x201 - RPM, Speed, Throttle
                    decoder.decode0x201(rxBuf);
                    break;

                case CAN_ID_WARNING_LIGHTS:  // 0x420 - Temp, Oil, Warnings
                    decoder.decode0x420(rxBuf);
                    break;

                case CAN_ID_WHEEL_SPEEDS_DASH:  // 0x4B1 - Wheel speeds
                    decoder.decode0x4B1(rxBuf);
                    break;
            }
        }
    }
}

/*
 * Setup display grid layout
 */
void setupDisplay() {
    oled.fillScreen(COLOR_BLACK);

    // Draw grid lines
    oled.drawLine(64, 0, 64, 128, COLOR_WHITE);   // Vertical center
    oled.drawLine(0, 32, 128, 32, COLOR_WHITE);   // Horizontal 1/4
    oled.drawLine(0, 64, 128, 64, COLOR_WHITE);   // Horizontal 1/2
    oled.drawLine(0, 96, 128, 96, COLOR_WHITE);   // Horizontal 3/4

    // Draw labels for each cell
    for (byte cell = 1; cell <= 8; cell++) {
        drawCellLabel(cell);
    }
}

/*
 * Draw label for a display cell
 */
void drawCellLabel(byte cell) {
    byte pos_x, pos_y;
    getCellPosition(cell, pos_x, pos_y);

    oled.setTextSize(1);
    oled.setTextColor(COLOR_WHITE);

    switch(config_Display[cell]) {
        case COOLANT_TEMP:
            oled.setCursor(pos_x, pos_y + 22);
            oled.print("COOLANT");
            oled.setCursor(pos_x + 42, pos_y + 22);
            oled.print(celsiusMode ? "C" : "F");
            break;

        case ENGINE_RPM:
            oled.setCursor(pos_x + 12, pos_y + 22);
            oled.print("RPM");
            break;

        case VEHICLE_SPEED:
            oled.setCursor(pos_x + 8, pos_y + 22);
            oled.print("SPEED");
            oled.setCursor(pos_x + 28, pos_y + 22 + 8);
            oled.print(metricMode ? "kph" : "mph");
            break;

        case THROTTLE_POS:
            oled.setCursor(pos_x + 4, pos_y + 22);
            oled.print("THROTTLE");
            oled.setCursor(pos_x + 46, pos_y + 22);
            oled.print("%");
            break;

        case OIL_PRESSURE:
            oled.setCursor(pos_x + 6, pos_y + 22);
            oled.print("OIL OK");
            break;

        case WHEEL_SPEED_FL:
            oled.setCursor(pos_x + 4, pos_y + 22);
            oled.print("WHEEL FL");
            break;

        case WHEEL_SPEED_FR:
            oled.setCursor(pos_x + 4, pos_y + 22);
            oled.print("WHEEL FR");
            break;

        default:
            break;
    }
}

/*
 * Update all display cells with current values
 */
void updateDisplay() {
    for (byte cell = 1; cell <= 8; cell++) {
        drawCellValue(cell);
    }
}

/*
 * Draw value for a display cell
 */
void drawCellValue(byte cell) {
    byte pos_x, pos_y;
    getCellPosition(cell, pos_x, pos_y);

    // Clear value area (not label)
    oled.fillRect(pos_x, pos_y, 60, 16, COLOR_BLACK);

    oled.setTextSize(2);
    uint16_t color = COLOR_WHITE;
    int value = 0;

    switch(config_Display[cell]) {
        case COOLANT_TEMP:
            value = decoder.tempToCelsius(decoder.warningLights.coolantTemperature);
            if (!celsiusMode) {
                value = decoder.tempToFahrenheit(decoder.warningLights.coolantTemperature);
            }
            if (value > coolantTempWarning) {
                color = COLOR_RED;
            }
            oled.setTextColor(color);
            oled.setCursor(pos_x + 8, pos_y);
            if (value >= 100) {
                oled.print(value);
            } else {
                oled.print(" ");
                oled.print(value);
            }
            break;

        case ENGINE_RPM:
            value = decoder.pcmStatus.engineRPM;
            oled.setTextColor(COLOR_WHITE);
            oled.setCursor(pos_x, pos_y);
            if (value >= 1000) {
                oled.print(value / 1000);
                oled.print("k");
            } else {
                oled.print(value);
            }
            break;

        case VEHICLE_SPEED:
            value = decoder.pcmStatus.vehicleSpeed;
            if (metricMode) {
                value = decoder.speedMPHtoKPH(value);
            }
            oled.setTextColor(COLOR_WHITE);
            oled.setCursor(pos_x + 8, pos_y);
            if (value >= 100) {
                oled.print(value);
            } else {
                oled.print(" ");
                oled.print(value);
            }
            break;

        case THROTTLE_POS:
            value = decoder.pcmStatus.throttlePosition;
            oled.setTextColor(COLOR_WHITE);
            oled.setCursor(pos_x + 8, pos_y);
            if (value >= 100) {
                oled.print(value);
            } else {
                oled.print(" ");
                oled.print(value);
            }
            break;

        case OIL_PRESSURE:
            if (decoder.warningLights.oilPressureMIL) {
                color = COLOR_RED;
                oled.setTextColor(color);
                oled.setCursor(pos_x + 4, pos_y);
                oled.print(" LOW");
            } else {
                oled.setTextColor(COLOR_WHITE);
                oled.setCursor(pos_x + 8, pos_y);
                oled.print(" OK");
            }
            break;

        case WHEEL_SPEED_FL:
            value = decoder.wheelSpeeds.frontLeft / 100;  // kph
            if (!metricMode) {
                value = decoder.getWheelSpeedMPH(decoder.wheelSpeeds.frontLeft);
            }
            oled.setTextColor(COLOR_WHITE);
            oled.setCursor(pos_x + 8, pos_y);
            if (value >= 100) {
                oled.print(value);
            } else {
                oled.print(" ");
                oled.print(value);
            }
            break;

        case WHEEL_SPEED_FR:
            value = decoder.wheelSpeeds.frontRight / 100;  // kph
            if (!metricMode) {
                value = decoder.getWheelSpeedMPH(decoder.wheelSpeeds.frontRight);
            }
            oled.setTextColor(COLOR_WHITE);
            oled.setCursor(pos_x + 8, pos_y);
            if (value >= 100) {
                oled.print(value);
            } else {
                oled.print(" ");
                oled.print(value);
            }
            break;

        default:
            break;
    }

    oled.setTextColor(COLOR_WHITE);
}

/*
 * Get screen position for a cell (1-8)
 */
void getCellPosition(byte cell, byte &pos_x, byte &pos_y) {
    // Grid layout:
    // [1][2]
    // [3][4]
    // [5][6]
    // [7][8]

    // Determine row
    if (cell == 1 || cell == 2) {
        pos_y = 2;
    } else if (cell == 3 || cell == 4) {
        pos_y = 34;
    } else if (cell == 5 || cell == 6) {
        pos_y = 66;
    } else {
        pos_y = 98;
    }

    // Determine column
    if (cell % 2 == 0) {
        pos_x = 68;  // Right column
    } else {
        pos_x = 4;   // Left column
    }
}
