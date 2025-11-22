/*
 * RX8 Speed-Sensitive Wipers with CAN Bus
 *
 * Enhanced version of RX8 wipers module that reads vehicle speed
 * from CAN bus (0x201) to adjust wiper timing based on speed.
 *
 * This is an EXAMPLE showing how to integrate RX8_CAN_Messages library
 * with the wipers module for speed-dependent wiper delay.
 *
 * Hardware:
 * - ATtiny441/841 or Arduino Nano/Uno
 * - MCP2515 CAN module (NEW!)
 * - Connection to RX8 CAN bus
 * - Connection to wiper control signals
 *
 * Features:
 * - Faster wiping at higher speeds
 * - Slower wiping at low speeds
 * - Automatic adjustment
 * - Uses RX8_CAN_Messages decoder library
 *
 * Integration: Add this to existing wipers_Module firmware
 *
 * Repository: https://github.com/michaelprowacki/MazdaRX8Arduino
 * Based on: basilhussain/rx8-wipers
 * CAN integration: 2025-11-15
 */

#include <mcp_can.h>
#include <SPI.h>
#include "RX8_CAN_Messages.h"

// CAN bus configuration
#define CAN_CS 10
#define CAN_INT 2

// Wiper control pins (adjust for your setup)
#define WIPER_CONTROL_PIN 3
#define WIPER_SENSE_PIN 4

// CAN objects
MCP_CAN CAN0(CAN_CS);
RX8_CAN_Decoder decoder;

// Wiper timing
int wiperDelay = 2000;  // Default 2 seconds
unsigned long lastWipe = 0;
int vehicleSpeed = 0;

void setup() {
    Serial.begin(115200);

    // Initialize wiper control pins
    pinMode(WIPER_CONTROL_PIN, OUTPUT);
    pinMode(WIPER_SENSE_PIN, INPUT);

    // Initialize CAN bus
    if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
        Serial.println("CAN: OK");
    } else {
        Serial.println("CAN: FAILED - Using default timing");
        // Continue without CAN - use fixed delay
    }

    CAN0.setMode(MCP_NORMAL);
    pinMode(CAN_INT, INPUT);

    Serial.println("Speed-Sensitive Wipers Active");
}

void loop() {
    // Read vehicle speed from CAN bus
    readCANSpeed();

    // Adjust wiper timing based on speed
    adjustWiperTiming();

    // Control wipers
    controlWipers();
}

/*
 * Read vehicle speed from CAN bus (0x201)
 */
void readCANSpeed() {
    long unsigned int rxId;
    unsigned char len = 0;
    unsigned char rxBuf[8];

    // Read all available CAN messages
    while (!digitalRead(CAN_INT)) {
        if (CAN0.readMsgBuf(&rxId, &len, rxBuf) == CAN_OK) {
            if (rxId == CAN_ID_PCM_STATUS) {  // 0x201
                decoder.decode0x201(rxBuf);
                vehicleSpeed = decoder.pcmStatus.vehicleSpeed;  // MPH

                // Debug output
                Serial.print("Speed: ");
                Serial.print(vehicleSpeed);
                Serial.print(" mph | Delay: ");
                Serial.println(wiperDelay);
            }
        }
    }
}

/*
 * Adjust wiper delay based on vehicle speed
 */
void adjustWiperTiming() {
    if (vehicleSpeed == 0) {
        // Stopped - slow wiping
        wiperDelay = 3000;  // 3 seconds
    }
    else if (vehicleSpeed < 20) {
        // City driving - moderate wiping
        wiperDelay = 2000;  // 2 seconds
    }
    else if (vehicleSpeed < 40) {
        // Suburban - faster wiping
        wiperDelay = 1500;  // 1.5 seconds
    }
    else if (vehicleSpeed < 60) {
        // Highway - fast wiping
        wiperDelay = 1000;  // 1 second
    }
    else {
        // High speed - very fast wiping
        wiperDelay = 500;   // 0.5 seconds
    }
}

/*
 * Control wiper operation with adjusted timing
 */
void controlWipers() {
    // Check if it's time for next wipe
    if (millis() - lastWipe >= wiperDelay) {
        // Trigger wiper
        digitalWrite(WIPER_CONTROL_PIN, HIGH);
        delay(100);  // Short pulse
        digitalWrite(WIPER_CONTROL_PIN, LOW);

        lastWipe = millis();

        Serial.print("Wipe at ");
        Serial.print(vehicleSpeed);
        Serial.println(" mph");
    }
}

/*
 * Alternative: Map speed to delay using formula
 */
int calculateWiperDelay(int speed) {
    // Formula: delay = 3000 - (speed * 25)
    // At 0 mph: 3000ms
    // At 20 mph: 2500ms
    // At 40 mph: 2000ms
    // At 60 mph: 1500ms
    // At 80 mph: 1000ms
    // At 100 mph: 500ms

    int delay = 3000 - (speed * 25);
    delay = constrain(delay, 500, 3000);  // Limit 0.5-3 seconds
    return delay;
}
