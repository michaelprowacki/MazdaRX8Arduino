/*
 * AC Display Module with CAN Bus Integration - Example
 *
 * This example shows how to integrate RX8 CAN bus reading into the
 * AC Display Module to show live engine data alongside AC controls.
 *
 * Features:
 * - Read ECU data from CAN bus (0x201, 0x420)
 * - Display RPM, speed, throttle, temperature
 * - Add custom menu pages for engine data
 * - Flash warnings on AC display
 * - Send CAN data to ESP8266 for logging
 *
 * Hardware:
 * - Arduino Mega 2560
 * - Factory AC display unit
 * - MCP2515 CAN module (NEW!)
 * - RTC module (I2C)
 * - Button panel
 * - ESP8266 Companion (optional)
 *
 * Wiring (MCP2515):
 * - VCC → 5V
 * - GND → GND
 * - CS → Pin 53 (Mega SS)
 * - SI → Pin 51 (Mega MOSI)
 * - SO → Pin 50 (Mega MISO)
 * - SCK → Pin 52 (Mega SCK)
 * - INT → Pin 21 (Mega INT0)
 *
 * Repository: https://github.com/michaelprowacki/MazdaRX8Arduino
 * Based on: S1-RX8-AC-Display-controller by NES-FM
 * CAN integration: 2025-11-15
 */

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <mcp_can.h>
#include <Smoothed.h>
#include "RX8_CAN_Messages.h"

// ========================================
// CONFIGURATION
// ========================================

// Enable/disable CAN bus integration
#define ENABLE_CAN_BUS 1

// CAN bus pins (Arduino Mega)
#define CAN_CS 53   // Mega default SS pin
#define CAN_INT 21  // INT0 on Mega

// Pin definitions (simplified - adjust for your setup)
#define IGNITION_VOLTAGE A4

// Serial settings
#define DEBUG_BAUD 115200

// Display update rate
#define DISPLAY_UPDATE_MS 100

// ========================================
// GLOBAL OBJECTS
// ========================================

// CAN bus objects
MCP_CAN CAN0(CAN_CS);
RX8_CAN_Decoder canDecoder;

// Battery voltage smoothing
Smoothed<double> bat_volt;

// ========================================
// LIVE CAN DATA
// ========================================

// From 0x201 (PCM Status)
int liveRPM = 0;
int liveSpeed = 0;         // MPH
int liveThrottle = 0;      // 0-100%

// From 0x420 (Warning Lights)
int liveCoolantTemp = 0;   // Celsius
bool liveCheckEngine = false;
bool liveOilPressureWarning = false;
bool liveLowCoolant = false;
bool liveOverheat = false;

// From 0x4B1 (Wheel Speeds)
int liveFrontLeft = 0;     // kph * 100
int liveFrontRight = 0;    // kph * 100

// Additional calculated values
float liveBatteryVoltage = 0.0;

// ========================================
// MENU SYSTEM STATE
// ========================================

enum MenuPage {
    PAGE_AC_CONTROL,
    PAGE_RPM,
    PAGE_SPEED,
    PAGE_TEMPERATURE,
    PAGE_THROTTLE,
    PAGE_BATTERY,
    PAGE_COUNT  // Total number of pages
};

MenuPage currentPage = PAGE_AC_CONTROL;
unsigned long lastDisplayUpdate = 0;
unsigned long lastWarningBlink = 0;
bool warningBlinkState = false;

// ========================================
// FUNCTION DECLARATIONS
// ========================================

void initCAN();
void readCANMessages();
void updateDisplay();
void displayCurrentPage();
void checkWarnings();
float getBatVolt();

// Simulated menu navigation (replace with actual button handling)
void nextPage();
void previousPage();

// ========================================
// SETUP
// ========================================

void setup() {
    Serial.begin(DEBUG_BAUD);
    Serial.println(F("================================="));
    Serial.println(F("AC Display with CAN Integration"));
    Serial.println(F("================================="));

    // Initialize I2C bus (for RTC and other I2C devices)
    Wire.begin();
    Serial.println(F("I2C initialized"));

    // Initialize battery voltage smoothing
    bat_volt.begin(SMOOTHED_AVERAGE, 10);
    Serial.println(F("Battery voltage monitoring initialized"));

#if ENABLE_CAN_BUS
    // Initialize CAN bus
    initCAN();
#endif

    // Initialize your custom libraries here
    // buttons.init();
    // time.init();
    // ac.init();
    // disp.init();
    // esp.init();
    // etc.

    Serial.println(F("Initialization complete"));
    Serial.println(F("================================="));
    Serial.println();
    Serial.println(F("Menu Navigation (Serial):"));
    Serial.println(F("  Send 'n' for next page"));
    Serial.println(F("  Send 'p' for previous page"));
    Serial.println();
}

// ========================================
// MAIN LOOP
// ========================================

void loop() {
#if ENABLE_CAN_BUS
    // Read CAN messages
    readCANMessages();

    // Check for warnings
    checkWarnings();
#endif

    // Read battery voltage
    liveBatteryVoltage = getBatVolt();

    // Update display periodically
    if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_MS) {
        updateDisplay();
        lastDisplayUpdate = millis();
    }

    // Handle serial input for menu navigation (demo)
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 'n') nextPage();
        if (cmd == 'p') previousPage();
    }

    // Your existing AC display logic would go here
    // buttons.tick();
    // ac.tick();
    // time.tick();
    // backlight.tick();
    // esp.tick();
}

// ========================================
// CAN BUS FUNCTIONS
// ========================================

/**
 * Initialize CAN bus
 */
void initCAN() {
    pinMode(CAN_INT, INPUT);

    Serial.print(F("Initializing CAN bus... "));

    if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
        Serial.println(F("OK"));
        CAN0.setMode(MCP_NORMAL);
    } else {
        Serial.println(F("FAILED"));
        Serial.println(F("Running without CAN - will show zero values"));
    }
}

/**
 * Read and decode all available CAN messages
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
                    canDecoder.decode0x201(rxBuf);
                    liveRPM = canDecoder.pcmStatus.engineRPM;
                    liveSpeed = canDecoder.pcmStatus.vehicleSpeed;
                    liveThrottle = canDecoder.pcmStatus.throttlePosition;
                    break;

                case CAN_ID_WARNING_LIGHTS:  // 0x420 - Temp, Warnings
                    canDecoder.decode0x420(rxBuf);
                    liveCoolantTemp = canDecoder.tempToCelsius(
                        canDecoder.warningLights.coolantTemperature
                    );
                    liveCheckEngine = canDecoder.warningLights.checkEngineMIL;
                    liveOilPressureWarning = canDecoder.warningLights.oilPressureMIL;
                    liveLowCoolant = canDecoder.warningLights.lowCoolantMIL;
                    liveOverheat = canDecoder.warningLights.engineOverheat;
                    break;

                case CAN_ID_WHEEL_SPEEDS_DASH:  // 0x4B1 - Wheel Speeds
                    canDecoder.decode0x4B1(rxBuf);
                    liveFrontLeft = canDecoder.wheelSpeeds.frontLeft;
                    liveFrontRight = canDecoder.wheelSpeeds.frontRight;
                    break;
            }
        }
    }
}

/**
 * Check for active warnings and flash display
 */
void checkWarnings() {
    // Flash warning every 500ms if any warning active
    if (liveCheckEngine || liveOilPressureWarning ||
        liveOverheat || liveLowCoolant) {

        if (millis() - lastWarningBlink >= 500) {
            warningBlinkState = !warningBlinkState;
            lastWarningBlink = millis();

            // In real implementation, toggle backlight or display icon
            // backlight.setWarningBlink(warningBlinkState);
        }
    }
}

// ========================================
// DISPLAY FUNCTIONS
// ========================================

/**
 * Update display based on current menu page
 */
void updateDisplay() {
    // Clear display (in real implementation)
    // disp.clearDisplay();

    // Show current page
    displayCurrentPage();

    // Send to display hardware (in real implementation)
    // disp.sendSevenSeg();
    // disp.sendIcons();
}

/**
 * Display content for current menu page
 */
void displayCurrentPage() {
    // This is a serial debug version
    // In real implementation, use your display library

    Serial.print(F("Page "));
    Serial.print(currentPage + 1);
    Serial.print(F("/"));
    Serial.print(PAGE_COUNT);
    Serial.print(F(": "));

    switch(currentPage) {
        case PAGE_AC_CONTROL:
            Serial.println(F("AC CONTROL"));
            Serial.println(F("  (Normal AC controls)"));
            // In real implementation:
            // disp.setAcIcons(ac.iconsLeds);
            // disp.writeToCharDisp(ac.getDisplayText());
            break;

        case PAGE_RPM:
            Serial.print(F("RPM: "));
            Serial.println(liveRPM);
            // In real implementation:
            // disp.writeToCharDisp("RPM");
            // disp.writeNumber(liveRPM);
            break;

        case PAGE_SPEED:
            Serial.print(F("SPEED: "));
            Serial.print(liveSpeed);
            Serial.println(F(" MPH"));
            // In real implementation:
            // disp.writeToCharDisp("SPD");
            // disp.writeNumber(liveSpeed);
            // disp.writeUnit("MPH");
            break;

        case PAGE_TEMPERATURE:
            Serial.print(F("COOLANT: "));
            Serial.print(liveCoolantTemp);
            Serial.println(F(" C"));
            if (liveCoolantTemp > 95) {
                Serial.println(F("  ⚠ HIGH TEMP!"));
            }
            // In real implementation:
            // disp.writeToCharDisp("TMP");
            // disp.writeNumber(liveCoolantTemp);
            // disp.writeUnit("C");
            // if (liveCoolantTemp > 95) disp.setWarningIcon(true);
            break;

        case PAGE_THROTTLE:
            Serial.print(F("THROTTLE: "));
            Serial.print(liveThrottle);
            Serial.println(F("%"));
            // In real implementation:
            // disp.writeToCharDisp("THR");
            // disp.writeNumber(liveThrottle);
            // disp.writeUnit("%");
            break;

        case PAGE_BATTERY:
            Serial.print(F("BATTERY: "));
            Serial.print(liveBatteryVoltage, 1);
            Serial.println(F(" V"));
            // In real implementation:
            // disp.writeToCharDisp("BAT");
            // disp.writeNumber(liveBatteryVoltage, 1);
            // disp.writeUnit("V");
            break;

        default:
            break;
    }

    // Show active warnings
    if (liveCheckEngine || liveOilPressureWarning ||
        liveOverheat || liveLowCoolant) {
        Serial.print(F("  WARNINGS: "));
        if (liveCheckEngine) Serial.print(F("[CHECK] "));
        if (liveOilPressureWarning) Serial.print(F("[OIL] "));
        if (liveOverheat) Serial.print(F("[HOT] "));
        if (liveLowCoolant) Serial.print(F("[COOLANT] "));
        Serial.println();
    }

    Serial.println();
}

// ========================================
// MENU NAVIGATION
// ========================================

/**
 * Go to next menu page
 */
void nextPage() {
    currentPage = (MenuPage)((currentPage + 1) % PAGE_COUNT);
    Serial.println(F("\n>>> Next Page <<<\n"));
}

/**
 * Go to previous menu page
 */
void previousPage() {
    if (currentPage == 0) {
        currentPage = (MenuPage)(PAGE_COUNT - 1);
    } else {
        currentPage = (MenuPage)(currentPage - 1);
    }
    Serial.println(F("\n>>> Previous Page <<<\n"));
}

// ========================================
// SENSOR FUNCTIONS
// ========================================

/**
 * Read and smooth battery voltage
 */
float getBatVolt() {
    // Read analog value (0-1023)
    int rawValue = analogRead(IGNITION_VOLTAGE);

    // Voltage divider conversion factor
    // Adjust based on your resistor values
    // Formula: (R1 + R2) / R2 * 5V / 1024 steps
    const float VOLTAGE_CONVERSION = 0.01487643158529234;

    // Convert to voltage
    double voltage = (double)rawValue * VOLTAGE_CONVERSION;

    // Add to smoothing filter
    bat_volt.add(voltage);

    // Return smoothed value
    return bat_volt.get();
}

// ========================================
// HELPER FUNCTIONS
// ========================================

/**
 * Example: Send CAN data to ESP8266 for logging
 */
void sendCANtoESP() {
    // In real implementation with espComm library:
    // esp.sendCommand("RPM", liveRPM);
    // esp.sendCommand("SPEED", liveSpeed);
    // esp.sendCommand("TEMP", liveCoolantTemp);
    // esp.sendCommand("THROTTLE", liveThrottle);
    // esp.sendCommand("VOLTAGE", liveBatteryVoltage);
}

/**
 * Example: Calculate average wheel speed
 */
int getAvgWheelSpeed() {
    // Convert to MPH and average
    int fl_mph = canDecoder.getWheelSpeedMPH(liveFrontLeft);
    int fr_mph = canDecoder.getWheelSpeedMPH(liveFrontRight);
    return (fl_mph + fr_mph) / 2;
}

/**
 * Example: Check if engine is running
 */
bool isEngineRunning() {
    return liveRPM > 500;  // 500 RPM threshold
}

/**
 * Example: Get temperature in Fahrenheit
 */
int getCoolantTempF() {
    return canDecoder.tempToFahrenheit(
        canDecoder.warningLights.coolantTemperature
    );
}
