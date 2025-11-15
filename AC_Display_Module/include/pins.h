#pragma once

// Pin definitions for S1 RX8 AC Display Controller
// Based on Arduino Mega 2560

// ========================================
// ROTARY ENCODER INPUTS
// ========================================
#define fanInTwo 2         // Fan encoder interrupt pin
#define tempInOne 3        // Temperature encoder interrupt pin
#define fanInOne 18        // Fan encoder second pin
#define tempInTwo 19       // Temperature encoder second pin

// ========================================
// BACKLIGHT CONTROL
// ========================================
#define hazardBacklight 9   // Hazard warning backlight
#define footBacklight 12    // Footwell backlight

// ========================================
// DSP CONTROL
// ========================================
#define dspWriteProtect 11  // DSP write protection pin

// ========================================
// AC AMPLIFIER SERIAL COMMUNICATION
// ========================================
#define acAmpTX 16         // AC amplifier transmit
#define acAmpRX 17         // AC amplifier receive

// ========================================
// BUTTON MATRIX - PORTA INPUTS
// ========================================
#define matrixRow1 24      // Button matrix row 1
#define matrixRow2 22      // Button matrix row 2
#define matrixRow3 25      // Button matrix row 3
#define matrixRow4 23      // Button matrix row 4

// ========================================
// LED OUTPUTS - PORTA
// ========================================
#define rearDemistLED 26   // Rear demist indicator LED
#define AirConLED 27       // A/C indicator LED
#define reCircLED 28       // Recirculation indicator LED
#define matrixColB 29      // Button matrix column B

// ========================================
// LED OUTPUTS - PORTC
// ========================================
#define freshAirLED 30     // Fresh air indicator LED
#define matrixColA 31      // Button matrix column A
#define frontDemistLED 32  // Front demist indicator LED
#define autoLED 34         // Auto mode indicator LED

// ========================================
// DISPLAY MODE SELECT - PORT L
// ========================================
#define mode0 49           // Display mode select bit 0
#define mode1 48           // Display mode select bit 1

// ========================================
// SPI COMMUNICATION - PORTB
// ========================================
#define DATAOUT 51         // MOSI - Master Out Slave In
#define SPICLOCK 52        // SCK - Serial Clock
#define ssPin 53           // SS - Slave Select (PB0)

// ========================================
// ANALOG INPUTS
// ========================================
#define ignitionVoltage A4     // Battery/ignition voltage sense
#define backlightPositive A8   // Backlight positive feedback
#define backlightNegative A11  // Backlight negative feedback

// ========================================
// PIN USAGE NOTES
// ========================================
/*
 * ENCODER PINS:
 * - Pins 2 and 3 are interrupt-capable for responsive encoder reading
 * - Fan encoder: pins 2, 18
 * - Temp encoder: pins 3, 19
 *
 * BUTTON MATRIX:
 * - 4x2 matrix for 8 buttons
 * - Rows: 22, 23, 24, 25
 * - Columns: 29, 31
 *
 * SPI DISPLAY:
 * - Standard SPI pins on Mega (51, 52, 53)
 * - Controls 7-segment displays and LED matrix
 *
 * SERIAL PORTS:
 * - Serial (USB): Debug output at 115200 baud
 * - Serial1 (pins 16, 17): AC amplifier communication
 * - Serial3: ESP8266 companion (if used)
 *
 * ANALOG SENSING:
 * - A4: Voltage divider for 12V battery monitoring
 * - A8, A11: Backlight feedback for PWM control
 */
