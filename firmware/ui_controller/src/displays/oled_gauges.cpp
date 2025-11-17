/**
 * @file oled_gauges.cpp
 * @brief Aftermarket OLED display gauges implementation
 *
 * @author Ported for Phase 5 unified architecture
 * @date 2025-11-16
 */

#include "oled_gauges.h"
#include "../../config/features.h"
#include <Arduino.h>

#if ENABLE_OLED_GAUGES

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

namespace OLEDGauges {

// Display configuration
static const uint8_t SCREEN_WIDTH = 128;
static const uint8_t SCREEN_HEIGHT = 64;
static const uint8_t OLED_RESET = -1;  // No reset pin

// Display instance
static Adafruit_SSD1306* display = nullptr;

// Current page
static GaugePage current_page = PAGE_RPM_SPEED;

// Vehicle data
struct VehicleData {
    uint16_t rpm;
    uint16_t speed;
    int16_t coolant_temp;
    int16_t oil_temp;
    int16_t intake_temp;
    uint16_t oil_pressure;
    int16_t boost_pressure;
    uint16_t voltage;
    uint8_t throttle;
} vehicle_data = {0};

// Warnings
static String warnings[5];
static uint8_t warning_count = 0;

// Forward declarations
static void drawPageRPMSpeed();
static void drawPageTemperatures();
static void drawPagePressures();
static void drawPageElectrical();
static void drawPageWarnings();

// ============================================================================
// INITIALIZATION
// ============================================================================

void init(DisplayType type, uint8_t i2c_addr, int sda_pin, int scl_pin) {
    // Initialize I2C with custom pins
    Wire.begin(sda_pin, scl_pin);

    // Create display instance
    display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

    // Initialize display
    if (!display->begin(SSD1306_SWITCHCAPVCC, i2c_addr)) {
        Serial.println("[OLED] Display initialization failed!");
        return;
    }

    // Clear display
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);
    display->println("RX8 OLED Gauges");
    display->println("Initializing...");
    display->display();

    delay(1000);

    Serial.println("[OLED] OLED gauges initialized");
}

// ============================================================================
// UPDATE
// ============================================================================

void update() {
    if (!display) return;

    display->clearDisplay();

    // Draw current page
    switch (current_page) {
        case PAGE_RPM_SPEED:
            drawPageRPMSpeed();
            break;
        case PAGE_TEMPERATURES:
            drawPageTemperatures();
            break;
        case PAGE_PRESSURES:
            drawPagePressures();
            break;
        case PAGE_ELECTRICAL:
            drawPageElectrical();
            break;
        case PAGE_WARNINGS:
            drawPageWarnings();
            break;
        default:
            break;
    }

    display->display();
}

// ============================================================================
// PAGE DRAWING
// ============================================================================

static void drawPageRPMSpeed() {
    // Title
    display->setTextSize(1);
    display->setCursor(0, 0);
    display->print("RPM / SPEED");

    // RPM (large)
    display->setTextSize(2);
    display->setCursor(0, 16);
    display->printf("%4d", vehicle_data.rpm);
    display->setTextSize(1);
    display->print(" RPM");

    // Speed (large)
    display->setTextSize(2);
    display->setCursor(0, 38);
    display->printf("%3d", vehicle_data.speed);
    display->setTextSize(1);
    display->print(" km/h");

    // RPM bar graph
    uint8_t rpm_percent = (vehicle_data.rpm * 100) / 9000;
    drawBar(0, 56, 128, 8, rpm_percent);
}

static void drawPageTemperatures() {
    display->setTextSize(1);
    display->setCursor(0, 0);
    display->print("TEMPERATURES");

    display->setCursor(0, 16);
    display->printf("Coolant: %3d C", vehicle_data.coolant_temp / 10);

    display->setCursor(0, 32);
    display->printf("Oil:     %3d C", vehicle_data.oil_temp / 10);

    display->setCursor(0, 48);
    display->printf("Intake:  %3d C", vehicle_data.intake_temp / 10);
}

static void drawPagePressures() {
    display->setTextSize(1);
    display->setCursor(0, 0);
    display->print("PRESSURES");

    display->setCursor(0, 16);
    display->printf("Oil:   %3d PSI", vehicle_data.oil_pressure);

    display->setCursor(0, 32);
    display->printf("Boost: %+4d PSI", vehicle_data.boost_pressure);

    // Boost gauge (visual)
    int16_t boost_pixel = map(vehicle_data.boost_pressure, -15, 30, 0, 128);
    boost_pixel = constrain(boost_pixel, 0, 127);
    display->drawFastVLine(boost_pixel, 48, 12, SSD1306_WHITE);
    display->drawFastHLine(0, 54, 128, SSD1306_WHITE);
    display->drawFastVLine(64, 52, 8, SSD1306_WHITE);  // Zero mark
}

static void drawPageElectrical() {
    display->setTextSize(1);
    display->setCursor(0, 0);
    display->print("ELECTRICAL");

    // Voltage (large)
    display->setTextSize(2);
    display->setCursor(0, 16);
    display->printf("%2d.%02d", vehicle_data.voltage / 100, vehicle_data.voltage % 100);
    display->setTextSize(1);
    display->print(" V");

    // Throttle
    display->setCursor(0, 40);
    display->printf("Throttle: %3d%%", vehicle_data.throttle);

    // Throttle bar
    drawBar(0, 56, 128, 8, vehicle_data.throttle);
}

static void drawPageWarnings() {
    display->setTextSize(1);
    display->setCursor(0, 0);
    display->print("WARNINGS");

    if (warning_count == 0) {
        display->setCursor(0, 28);
        display->print("  No warnings");
    } else {
        for (uint8_t i = 0; i < warning_count && i < 5; i++) {
            display->setCursor(0, 16 + i * 10);
            display->print(warnings[i]);
        }
    }
}

// ============================================================================
// PAGE CONTROL
// ============================================================================

void setPage(GaugePage page) {
    if (page < PAGE_COUNT) {
        current_page = page;
    }
}

GaugePage getPage() {
    return current_page;
}

void nextPage() {
    current_page = (GaugePage)((current_page + 1) % PAGE_COUNT);
}

// ============================================================================
// DATA SETTERS
// ============================================================================

void setRPM(uint16_t rpm) {
    vehicle_data.rpm = rpm;
}

void setSpeed(uint16_t speed) {
    vehicle_data.speed = speed;
}

void setCoolantTemp(int16_t temp) {
    vehicle_data.coolant_temp = temp * 10;
}

void setOilTemp(int16_t temp) {
    vehicle_data.oil_temp = temp * 10;
}

void setIntakeTemp(int16_t temp) {
    vehicle_data.intake_temp = temp * 10;
}

void setOilPressure(uint16_t pressure) {
    vehicle_data.oil_pressure = pressure;
}

void setBoostPressure(int16_t pressure) {
    vehicle_data.boost_pressure = pressure;
}

void setVoltage(uint16_t voltage) {
    vehicle_data.voltage = voltage;
}

void setThrottle(uint8_t throttle) {
    vehicle_data.throttle = throttle;
}

void addWarning(const char* warning) {
    if (warning_count < 5) {
        warnings[warning_count++] = String(warning);
    }
}

void clearWarnings() {
    warning_count = 0;
}

// ============================================================================
// DISPLAY CONTROL
// ============================================================================

void showSplash(const char* message, uint16_t duration_ms) {
    if (!display) return;

    display->clearDisplay();
    display->setTextSize(2);
    display->setCursor(0, 24);
    display->print(message);
    display->display();

    delay(duration_ms);
}

void setBrightness(uint8_t brightness) {
    if (!display) return;
    // Map 0-255 to 0-255 (OLED contrast)
    display->ssd1306_command(SSD1306_SETCONTRAST);
    display->ssd1306_command(brightness);
}

void setEnabled(bool enabled) {
    if (!display) return;
    if (enabled) {
        display->ssd1306_command(SSD1306_DISPLAYON);
    } else {
        display->ssd1306_command(SSD1306_DISPLAYOFF);
    }
}

// ============================================================================
// DRAWING PRIMITIVES
// ============================================================================

void drawGauge(int16_t x, int16_t y, uint8_t radius,
               float value, float min, float max) {
    if (!display) return;

    // Draw gauge arc
    display->drawCircle(x, y, radius, SSD1306_WHITE);

    // Calculate needle angle (180 degrees arc)
    float percent = (value - min) / (max - min);
    float angle = percent * 180.0 - 90.0;  // -90 to +90 degrees
    float rad = angle * 3.14159 / 180.0;

    // Draw needle
    int16_t x2 = x + radius * cos(rad);
    int16_t y2 = y + radius * sin(rad);
    display->drawLine(x, y, x2, y2, SSD1306_WHITE);
}

void drawBar(int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t value) {
    if (!display) return;

    // Draw border
    display->drawRect(x, y, width, height, SSD1306_WHITE);

    // Draw filled portion
    uint16_t fill_width = (width - 2) * value / 100;
    if (fill_width > 0) {
        display->fillRect(x + 1, y + 1, fill_width, height - 2, SSD1306_WHITE);
    }
}

} // namespace OLEDGauges

#endif // ENABLE_OLED_GAUGES
