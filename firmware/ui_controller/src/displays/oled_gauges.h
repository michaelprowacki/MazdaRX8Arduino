/**
 * @file oled_gauges.h
 * @brief Aftermarket OLED display gauges
 *
 * Displays vehicle telemetry on OLED screens:
 * - RPM gauge
 * - Speed gauge
 * - Temperature (coolant, oil, intake)
 * - Pressure (oil, boost)
 * - Voltage
 * - Custom warnings
 *
 * Supports multiple display types:
 * - SSD1306 (128x64, 128x32)
 * - SH1106 (128x64)
 * - SSD1351 (128x128 RGB OLED)
 *
 * @author Ported for Phase 5 unified architecture
 * @date 2025-11-16
 */

#ifndef OLED_GAUGES_H
#define OLED_GAUGES_H

#include <stdint.h>

namespace OLEDGauges {

/**
 * @brief Display type enumeration
 */
enum DisplayType {
    DISPLAY_SSD1306_128x64,  // Monochrome 128x64
    DISPLAY_SSD1306_128x32,  // Monochrome 128x32
    DISPLAY_SH1106_128x64,   // Monochrome 128x64 (different controller)
    DISPLAY_SSD1351_128x128  // RGB OLED 128x128
};

/**
 * @brief Gauge page enumeration
 */
enum GaugePage {
    PAGE_RPM_SPEED,      // RPM + Speed
    PAGE_TEMPERATURES,   // Coolant, oil, intake temps
    PAGE_PRESSURES,      // Oil pressure, boost pressure
    PAGE_ELECTRICAL,     // Voltage, current, power
    PAGE_WARNINGS,       // Warning lights, error codes
    PAGE_CUSTOM,         // User-defined
    PAGE_COUNT
};

/**
 * @brief Initialize OLED displays
 *
 * @param type Display controller type
 * @param i2c_addr I2C address (typically 0x3C or 0x3D)
 * @param sda_pin I2C SDA pin (default: 21)
 * @param scl_pin I2C SCL pin (default: 22)
 */
void init(DisplayType type = DISPLAY_SSD1306_128x64,
          uint8_t i2c_addr = 0x3C,
          int sda_pin = 21,
          int scl_pin = 22);

/**
 * @brief Update OLED displays (call from FreeRTOS task)
 */
void update();

/**
 * @brief Set current display page
 * @param page Page to display
 */
void setPage(GaugePage page);

/**
 * @brief Get current page
 * @return Current page
 */
GaugePage getPage();

/**
 * @brief Cycle to next page
 */
void nextPage();

/**
 * @brief Set RPM value
 * @param rpm Engine RPM (0-9000+)
 */
void setRPM(uint16_t rpm);

/**
 * @brief Set speed value
 * @param speed Vehicle speed in km/h
 */
void setSpeed(uint16_t speed);

/**
 * @brief Set coolant temperature
 * @param temp Temperature in °C
 */
void setCoolantTemp(int16_t temp);

/**
 * @brief Set oil temperature
 * @param temp Temperature in °C
 */
void setOilTemp(int16_t temp);

/**
 * @brief Set intake air temperature
 * @param temp Temperature in °C
 */
void setIntakeTemp(int16_t temp);

/**
 * @brief Set oil pressure
 * @param pressure Pressure in PSI
 */
void setOilPressure(uint16_t pressure);

/**
 * @brief Set boost pressure
 * @param pressure Pressure in PSI (can be negative for vacuum)
 */
void setBoostPressure(int16_t pressure);

/**
 * @brief Set battery voltage
 * @param voltage Voltage in V * 100 (e.g., 1380 = 13.80V)
 */
void setVoltage(uint16_t voltage);

/**
 * @brief Set throttle position
 * @param throttle Throttle percentage (0-100)
 */
void setThrottle(uint8_t throttle);

/**
 * @brief Add warning message
 * @param warning Warning text (max 20 chars)
 */
void addWarning(const char* warning);

/**
 * @brief Clear all warnings
 */
void clearWarnings();

/**
 * @brief Display splash screen
 * @param message Message to display
 * @param duration_ms Duration in milliseconds
 */
void showSplash(const char* message, uint16_t duration_ms = 2000);

/**
 * @brief Set display brightness
 * @param brightness Brightness level (0-255)
 */
void setBrightness(uint8_t brightness);

/**
 * @brief Enable/disable display
 * @param enabled true to enable display
 */
void setEnabled(bool enabled);

/**
 * @brief Draw gauge needle (for analog-style gauges)
 * @param x Center X
 * @param y Center Y
 * @param radius Radius
 * @param value Current value
 * @param min Minimum value
 * @param max Maximum value
 */
void drawGauge(int16_t x, int16_t y, uint8_t radius,
               float value, float min, float max);

/**
 * @brief Draw bar graph
 * @param x X position
 * @param y Y position
 * @param width Width in pixels
 * @param height Height in pixels
 * @param value Current value (0-100%)
 */
void drawBar(int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t value);

} // namespace OLEDGauges

#endif // OLED_GAUGES_H
