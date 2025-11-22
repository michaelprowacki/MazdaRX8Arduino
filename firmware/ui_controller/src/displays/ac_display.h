/**
 * @file ac_display.h
 * @brief Factory AC display controller for ESP32
 *
 * Controls the factory Mazda RX8 AC display unit:
 * - 7-segment display (temperature, fan speed)
 * - LED matrix (mode icons, status indicators)
 * - Button input (4x2 matrix + 2 rotary encoders)
 * - Real-time clock display
 * - Menu system
 *
 * Ported from legacy AC_Display_Module (Arduino Mega 2560)
 * to unified ESP32 firmware.
 *
 * @author Ported for Phase 5 unified architecture
 * @date 2025-11-16
 */

#ifndef AC_DISPLAY_H
#define AC_DISPLAY_H

#include <stdint.h>

namespace ACDisplay {

/**
 * @brief AC button enumeration
 */
enum Button {
    BTN_AUTO = 0,
    BTN_MODE,
    BTN_AC,
    BTN_FRONT_DEMIST,
    BTN_REAR_DEMIST,
    BTN_AIR_SOURCE,
    BTN_OFF,
    BTN_NONE
};

/**
 * @brief AC display state
 */
struct State {
    // Fan control
    uint8_t fan_speed;          // 0-7
    int8_t fan_rotation;        // Encoder delta

    // Temperature control
    uint8_t temp_digits[3];     // Temperature display (e.g., "72°F")
    int8_t temp_rotation;       // Encoder delta

    // Mode indicators
    bool auto_mode;
    bool ac_mode;
    bool front_demist;
    bool rear_demist;
    bool recirculate;
    bool mode_face;
    bool mode_feet;
    bool mode_defrost;

    // Button state
    Button short_press;
    Button long_press;

    // Display flags
    bool display_on;
    bool backlight_on;
};

/**
 * @brief Initialize AC display
 *
 * Sets up:
 * - SPI for 7-segment display
 * - GPIO for LED matrix
 * - Button matrix scanning
 * - Rotary encoders (interrupts)
 * - I2C for RTC
 */
void init();

/**
 * @brief Update AC display (call from FreeRTOS task)
 *
 * - Scans button matrix
 * - Reads encoder values
 * - Updates display
 * - Handles menu navigation
 */
void update();

/**
 * @brief Get current AC state
 * @return Pointer to current state
 */
const State* getState();

/**
 * @brief Set fan speed (0-7)
 * @param speed Fan speed level
 */
void setFanSpeed(uint8_t speed);

/**
 * @brief Set temperature display
 * @param temp Temperature value (for display formatting)
 */
void setTemperature(uint8_t temp);

/**
 * @brief Set auto mode indicator
 * @param enabled true to enable auto mode
 */
void setAutoMode(bool enabled);

/**
 * @brief Set AC mode indicator
 * @param enabled true to enable AC compressor
 */
void setACMode(bool enabled);

/**
 * @brief Set front demist indicator
 * @param enabled true to enable front demist
 */
void setFrontDemist(bool enabled);

/**
 * @brief Set rear demist indicator
 * @param enabled true to enable rear demist
 */
void setRearDemist(bool enabled);

/**
 * @brief Set air recirculation indicator
 * @param enabled true for recirculate mode
 */
void setRecirculate(bool enabled);

/**
 * @brief Update display with current state
 *
 * Sends data to:
 * - 7-segment display (SPI)
 * - LED matrix (GPIO)
 */
void refreshDisplay();

/**
 * @brief Display real-time clock
 * @param hours Hours (0-23)
 * @param minutes Minutes (0-59)
 * @param format_24h true for 24-hour format
 */
void displayClock(uint8_t hours, uint8_t minutes, bool format_24h);

/**
 * @brief Display custom message (menu system)
 * @param message 4-character message (e.g., "BATT", "TEMP")
 */
void displayMessage(const char* message);

/**
 * @brief Display numeric value (menu system)
 * @param value Value to display
 * @param unit Unit string (e.g., "°C", "PSI", "V")
 */
void displayValue(float value, const char* unit);

/**
 * @brief Set backlight brightness
 * @param brightness PWM value (0-255)
 */
void setBacklight(uint8_t brightness);

/**
 * @brief Handle button press (called by button scanner)
 * @param button Button that was pressed
 * @param long_press true if long press
 */
void handleButtonPress(Button button, bool long_press);

} // namespace ACDisplay

#endif // AC_DISPLAY_H
