#pragma once

#include <stdint.h>

// ========================================
// BUTTON ENUMERATION
// ========================================
/**
 * @brief Enumeration of all AC control buttons
 *
 * These values correspond to the button matrix positions
 * on the factory RX8 AC control panel.
 */
enum btn_enum {
    no_button = 0,      // No button pressed
    Auto = 1,           // Auto climate control switch
    Mode = 2,           // Vent mode selection switch
    AC = 3,             // A/C compressor on/off switch
    frontDemist = 4,    // Front windshield demist/defrost
    rearDemist = 5,     // Rear window defrost
    AirSource = 6,      // Fresh air / recirculation switch
    _invalid = 7,       // Invalid state (should not occur)
    Off = 8             // AC system off switch
};

// ========================================
// BUTTON STATE STRUCTURE
// ========================================
/**
 * @brief Current state of all button inputs
 *
 * This structure is updated every loop iteration with
 * the current state of all user inputs.
 */
struct buttonState {
    int fanRotation;              // Fan speed encoder rotation delta
    int tempRotation;             // Temperature encoder rotation delta
    btn_enum shortPushButton;     // Button that was short-pressed
    btn_enum longPushButton;      // Button that was long-pressed
};

// ========================================
// AC DISPLAY STATE STRUCTURE
// ========================================
/**
 * @brief Complete state of the AC system for display
 *
 * This structure contains all information needed to
 * render the AC display correctly, including fan speed,
 * temperature, mode indicators, and status lights.
 */
struct acShow {
    // Temperature and Fan
    uint8_t fanSpeed;             // Fan speed level (0-7 typically)
    uint8_t tempDigits[3];        // Temperature display digits [tens, ones, decimal]

    // Display Modes
    bool displayAmbient;          // Show ambient temp instead of setpoint

    // System States
    bool stateAuto;               // Auto mode active
    bool stateAc;                 // A/C compressor active
    bool stateEco;                // Economy mode active (if applicable)

    // Vent Modes
    bool modeFrontDemist;         // Front windshield demist active
    bool modeRearDemist;          // Rear window defrost active
    bool modeRecirculate;         // Recirculation mode active
    bool modeFeet;                // Feet vent active
    bool modeFace;                // Face vent active

    // Amplifier Status
    bool ampOn;                   // Amplifier powered on
    bool ampRunning;              // Amplifier running normally
};

// ========================================
// TIME STRUCTURE
// ========================================
/**
 * @brief Real-time clock display information
 *
 * Contains current time and flags for display updates.
 */
struct timeObj {
    uint8_t curMinute;            // Current minute (0-59)
    uint8_t curHour;              // Current hour (0-23 or 1-12 depending on format)
    bool minuteChange;            // Flag: minute has changed since last update
};

// ========================================
// DISPLAY ICON STATE STRUCTURE
// ========================================
/**
 * @brief State of all dashboard icons in the center section
 *
 * Controls various audio system and status indicators
 * on the AC display unit.
 */
struct midsectionIcons {
    // Media Status
    bool CD_IN;                   // CD inserted indicator
    bool MD_IN;                   // MiniDisc inserted indicator

    // Audio Processing
    bool ST;                      // Stereo indicator
    bool Dolby;                   // Dolby processing active

    // Radio Data System (RDS)
    bool AF;                      // Alternative Frequency search
    bool PTY;                     // Program Type search
    bool TA;                      // Traffic Announcement standby
    bool TP;                      // Traffic Program station

    // Playback Modes
    bool RPT;                     // Repeat mode
    bool RDM;                     // Random/shuffle mode
    bool Auto_M;                  // Auto memory/store

    // Punctuation and Separators
    bool mid_section_colon;       // Colon separator (:)
    bool fullstop_char_10_11;     // Decimal point between chars 10-11
    bool fullstop_char_11_12;     // Decimal point between chars 11-12
    bool min_sec_prime_marks;     // Prime marks for minutes/seconds (', ")
};

// ========================================
// UTILITY CONSTANTS
// ========================================

// Button long-press duration (milliseconds)
#define LONG_PRESS_DURATION 800

// Fan speed levels
#define FAN_SPEED_MIN 0
#define FAN_SPEED_MAX 7

// Temperature range (Celsius)
#define TEMP_MIN_C 18
#define TEMP_MAX_C 32

// Temperature range (Fahrenheit)
#define TEMP_MIN_F 64
#define TEMP_MAX_F 90
