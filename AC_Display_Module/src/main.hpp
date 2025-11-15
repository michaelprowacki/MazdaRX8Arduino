#pragma once

// ========================================
// STANDARD LIBRARIES
// ========================================
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

// ========================================
// THIRD-PARTY LIBRARIES
// ========================================
#include <Smoothed.h>              // Data smoothing for analog inputs

// ========================================
// PROJECT INCLUDES
// ========================================
#include "data_types.h"            // Common data structures
#include "pins.h"                  // Pin definitions

// ========================================
// CUSTOM LIBRARY INCLUDES
// ========================================
// Note: These are references to the custom libraries
// Actual implementation files should be placed in lib/ directory

/*
#include "acAmp.hpp"               // AC amplifier control
#include "backlightLed.hpp"        // Backlight LED management
#include "buttonPanel.hpp"         // Button input handling
#include "clock.hpp"               // Real-time clock
#include "confMenu.hpp"            // Configuration menu
#include "display.hpp"             // Display control
#include "espComm.hpp"             // ESP8266 communication
#include "mainMenu.hpp"            // Main menu system
#include "subVolMenu.hpp"          // Subwoofer volume menu
#include "dsp.hpp"                 // DSP control
#include "command_parser.h"        // Command interpreter
#include "logger.h"                // System logger
*/

// ========================================
// FUNCTION DECLARATIONS
// ========================================

/**
 * @brief Handle long button press actions
 * @param longButton The button that was long-pressed
 *
 * Long press actions typically open menus or toggle special modes:
 * - Auto: Toggle ambient temperature display
 * - Mode: Open configuration menu
 * - AC: Next menu page
 * - Front Demist: Previous menu page
 * - Off: Open subwoofer volume menu
 */
void longButtonAction(btn_enum longButton);

/**
 * @brief Toggle between menus
 * @param newMenu Pointer to the menu to activate
 *
 * Switches active menu, handling deactivation of current menu
 * and activation of new menu.
 */
void toggleMenu(void* newMenu);  // Using void* until baseMenu is defined

/**
 * @brief Get battery voltage reading
 * @return Battery voltage in volts (smoothed)
 *
 * Reads analog voltage from ignition sense pin, applies
 * smoothing filter, and returns voltage value.
 */
float getBatVolt();

// ========================================
// CONFIGURATION CONSTANTS
// ========================================

// Voltage divider ratio for battery sensing
// Adjust based on actual resistor values used
// Formula: (R1 + R2) / R2 * 5V / 1024 steps
#define VOLTAGE_CONVERSION_FACTOR 0.01487643158529234

// Smoothing window size for battery voltage
#define BAT_VOLT_SMOOTH_WINDOW 10

// Serial baud rates
#define DEBUG_BAUD_RATE 115200
#define ESP_BAUD_RATE 115200

// Temperature defaults
#define DEFAULT_MOTOR_TEMP 100.0f  // Default motor temperature in Celsius
