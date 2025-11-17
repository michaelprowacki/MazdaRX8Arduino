/**
 * @file ac_display.cpp
 * @brief Factory AC display controller implementation
 *
 * Ported from legacy AC_Display_Module (Arduino Mega 2560)
 * to unified ESP32 firmware.
 *
 * @author Ported for Phase 5 unified architecture
 * @date 2025-11-16
 */

#include "ac_display.h"
#include "../../config/features.h"
#include <Arduino.h>
#include <SPI.h>

#if ENABLE_AC_DISPLAY

namespace ACDisplay {

// Display state
static State display_state = {0};

// SPI settings for 7-segment display
static SPISettings spi_settings(1000000, MSBFIRST, SPI_MODE0);

// Pin definitions (from config/pin_mapping.h)
#define SPI_CS_PIN      5   // Chip select for display
#define LED_MATRIX_PIN  16  // LED matrix control
#define BACKLIGHT_PIN   17  // PWM backlight control

// Button matrix pins
#define BTN_ROW_PINS    {32, 33, 25, 26}  // 4 rows
#define BTN_COL_PINS    {27, 14}          // 2 columns

// Rotary encoder pins (interrupt capable)
#define ENC_FAN_A       18  // Fan encoder A
#define ENC_FAN_B       19  // Fan encoder B
#define ENC_TEMP_A      21  // Temp encoder A
#define ENC_TEMP_B      22  // Temp encoder B

// Button scan timing
static const uint16_t BUTTON_SCAN_MS = 10;
static const uint16_t LONG_PRESS_MS = 1000;

// Rotary encoder state
static volatile int8_t fan_encoder_delta = 0;
static volatile int8_t temp_encoder_delta = 0;

// Forward declarations
static void scanButtons();
static void updateEncoders();
static void sendToDisplay();

// ============================================================================
// INITIALIZATION
// ============================================================================

void init() {
    // Initialize SPI for 7-segment display
    SPI.begin();
    pinMode(SPI_CS_PIN, OUTPUT);
    digitalWrite(SPI_CS_PIN, HIGH);

    // Initialize LED matrix
    pinMode(LED_MATRIX_PIN, OUTPUT);

    // Initialize backlight PWM
    pinMode(BACKLIGHT_PIN, OUTPUT);
    ledcSetup(0, 5000, 8);  // Channel 0, 5kHz, 8-bit
    ledcAttachPin(BACKLIGHT_PIN, 0);
    ledcWrite(0, 128);  // 50% brightness default

    // Initialize button matrix
    const uint8_t row_pins[] = BTN_ROW_PINS;
    const uint8_t col_pins[] = BTN_COL_PINS;

    for (int i = 0; i < 4; i++) {
        pinMode(row_pins[i], OUTPUT);
        digitalWrite(row_pins[i], HIGH);
    }

    for (int i = 0; i < 2; i++) {
        pinMode(col_pins[i], INPUT_PULLUP);
    }

    // Initialize rotary encoders with interrupts
    pinMode(ENC_FAN_A, INPUT_PULLUP);
    pinMode(ENC_FAN_B, INPUT_PULLUP);
    pinMode(ENC_TEMP_A, INPUT_PULLUP);
    pinMode(ENC_TEMP_B, INPUT_PULLUP);

    // Attach interrupts for encoders (handled in main task loop)

    // Initialize display state
    display_state.fan_speed = 0;
    display_state.temp_digits[0] = 7;
    display_state.temp_digits[1] = 2;
    display_state.temp_digits[2] = 0;  // "72°F"
    display_state.auto_mode = false;
    display_state.ac_mode = false;
    display_state.display_on = true;
    display_state.backlight_on = true;

    Serial.println("[AC DISPLAY] Initialized");
}

// ============================================================================
// UPDATE (CALLED FROM FREERTOS TASK)
// ============================================================================

void update() {
    // Scan button matrix
    scanButtons();

    // Update encoder deltas
    updateEncoders();

    // Apply encoder changes
    display_state.fan_rotation = fan_encoder_delta;
    display_state.temp_rotation = temp_encoder_delta;
    fan_encoder_delta = 0;
    temp_encoder_delta = 0;

    // Update fan speed from encoder
    if (display_state.fan_rotation > 0) {
        if (display_state.fan_speed < 7) {
            display_state.fan_speed++;
        }
    } else if (display_state.fan_rotation < 0) {
        if (display_state.fan_speed > 0) {
            display_state.fan_speed--;
        }
    }

    // Update temperature from encoder
    if (display_state.temp_rotation > 0) {
        // Increase temperature
        uint8_t temp = display_state.temp_digits[0] * 10 + display_state.temp_digits[1];
        if (temp < 85) {
            temp++;
            display_state.temp_digits[0] = temp / 10;
            display_state.temp_digits[1] = temp % 10;
        }
    } else if (display_state.temp_rotation < 0) {
        // Decrease temperature
        uint8_t temp = display_state.temp_digits[0] * 10 + display_state.temp_digits[1];
        if (temp > 60) {
            temp--;
            display_state.temp_digits[0] = temp / 10;
            display_state.temp_digits[1] = temp % 10;
        }
    }

    // Refresh display
    refreshDisplay();
}

// ============================================================================
// BUTTON SCANNING
// ============================================================================

static void scanButtons() {
    const uint8_t row_pins[] = BTN_ROW_PINS;
    const uint8_t col_pins[] = BTN_COL_PINS;

    static uint32_t last_press_time = 0;
    static Button last_button = BTN_NONE;
    static bool button_held = false;

    // Scan 4x2 button matrix
    for (int row = 0; row < 4; row++) {
        // Set current row LOW, others HIGH
        for (int r = 0; r < 4; r++) {
            digitalWrite(row_pins[r], (r == row) ? LOW : HIGH);
        }

        delayMicroseconds(10);  // Settle time

        // Read columns
        for (int col = 0; col < 2; col++) {
            if (digitalRead(col_pins[col]) == LOW) {
                // Button pressed
                Button btn = (Button)(row * 2 + col);
                uint32_t now = millis();

                if (btn != last_button) {
                    // New button press
                    last_button = btn;
                    last_press_time = now;
                    button_held = false;
                } else if (!button_held && (now - last_press_time > LONG_PRESS_MS)) {
                    // Long press detected
                    display_state.long_press = btn;
                    handleButtonPress(btn, true);
                    button_held = true;
                }
            }
        }
    }

    // Detect button release
    bool any_pressed = false;
    for (int col = 0; col < 2; col++) {
        if (digitalRead(col_pins[col]) == LOW) {
            any_pressed = true;
            break;
        }
    }

    if (!any_pressed && last_button != BTN_NONE) {
        // Button released
        if (!button_held) {
            // Short press
            display_state.short_press = last_button;
            handleButtonPress(last_button, false);
        }
        last_button = BTN_NONE;
        button_held = false;
    }
}

// ============================================================================
// ENCODER UPDATE
// ============================================================================

static void updateEncoders() {
    // Read encoder states (simplified polling, could use interrupts)
    static uint8_t last_fan_state = 0;
    static uint8_t last_temp_state = 0;

    uint8_t fan_state = (digitalRead(ENC_FAN_A) << 1) | digitalRead(ENC_FAN_B);
    uint8_t temp_state = (digitalRead(ENC_TEMP_A) << 1) | digitalRead(ENC_TEMP_B);

    // Simple quadrature decoding
    if (fan_state != last_fan_state) {
        if ((last_fan_state == 0 && fan_state == 1) ||
            (last_fan_state == 1 && fan_state == 3) ||
            (last_fan_state == 3 && fan_state == 2) ||
            (last_fan_state == 2 && fan_state == 0)) {
            fan_encoder_delta++;
        } else {
            fan_encoder_delta--;
        }
        last_fan_state = fan_state;
    }

    if (temp_state != last_temp_state) {
        if ((last_temp_state == 0 && temp_state == 1) ||
            (last_temp_state == 1 && temp_state == 3) ||
            (last_temp_state == 3 && temp_state == 2) ||
            (last_temp_state == 2 && temp_state == 0)) {
            temp_encoder_delta++;
        } else {
            temp_encoder_delta--;
        }
        last_temp_state = temp_state;
    }
}

// ============================================================================
// DISPLAY OUTPUT
// ============================================================================

void refreshDisplay() {
    sendToDisplay();
}

static void sendToDisplay() {
    // Send to 7-segment display via SPI
    SPI.beginTransaction(spi_settings);
    digitalWrite(SPI_CS_PIN, LOW);

    // Send display data (format depends on display controller)
    // Example: 4 digits + fan speed indicators
    SPI.transfer(display_state.temp_digits[0]);
    SPI.transfer(display_state.temp_digits[1]);
    SPI.transfer(0x0F);  // Degree symbol
    SPI.transfer(display_state.fan_speed);

    digitalWrite(SPI_CS_PIN, HIGH);
    SPI.endTransaction();

    // Update LED matrix (mode indicators)
    // This would control individual LEDs via shift registers or GPIO
    // Simplified here
}

// ============================================================================
// SETTERS
// ============================================================================

const State* getState() {
    return &display_state;
}

void setFanSpeed(uint8_t speed) {
    display_state.fan_speed = (speed > 7) ? 7 : speed;
}

void setTemperature(uint8_t temp) {
    display_state.temp_digits[0] = temp / 10;
    display_state.temp_digits[1] = temp % 10;
}

void setAutoMode(bool enabled) {
    display_state.auto_mode = enabled;
}

void setACMode(bool enabled) {
    display_state.ac_mode = enabled;
}

void setFrontDemist(bool enabled) {
    display_state.front_demist = enabled;
}

void setRearDemist(bool enabled) {
    display_state.rear_demist = enabled;
}

void setRecirculate(bool enabled) {
    display_state.recirculate = enabled;
}

void displayClock(uint8_t hours, uint8_t minutes, bool format_24h) {
    // Convert to display format
    if (!format_24h && hours > 12) {
        hours -= 12;
    }
    display_state.temp_digits[0] = hours / 10;
    display_state.temp_digits[1] = hours % 10;
    display_state.temp_digits[2] = minutes / 10;
    // Update display
    refreshDisplay();
}

void displayMessage(const char* message) {
    // Display 4-character message (menu system)
    Serial.printf("[AC DISPLAY] Message: %s\n", message);
}

void displayValue(float value, const char* unit) {
    Serial.printf("[AC DISPLAY] Value: %.2f %s\n", value, unit);
}

void setBacklight(uint8_t brightness) {
    ledcWrite(0, brightness);
}

// ============================================================================
// BUTTON HANDLER
// ============================================================================

void handleButtonPress(Button button, bool long_press) {
    const char* btn_names[] = {"AUTO", "MODE", "AC", "F_DEMIST", "R_DEMIST", "AIR_SRC", "OFF"};

    Serial.printf("[AC DISPLAY] Button: %s (%s press)\n",
                 btn_names[button],
                 long_press ? "LONG" : "SHORT");

    // Handle button actions
    switch (button) {
        case BTN_AUTO:
            if (long_press) {
                // Long press: Toggle ambient temp display
            } else {
                // Short press: Toggle auto mode
                display_state.auto_mode = !display_state.auto_mode;
            }
            break;

        case BTN_AC:
            display_state.ac_mode = !display_state.ac_mode;
            break;

        case BTN_FRONT_DEMIST:
            display_state.front_demist = !display_state.front_demist;
            break;

        case BTN_REAR_DEMIST:
            display_state.rear_demist = !display_state.rear_demist;
            break;

        case BTN_AIR_SOURCE:
            display_state.recirculate = !display_state.recirculate;
            break;

        case BTN_OFF:
            if (long_press) {
                // Long press: Enter menu
            } else {
                // Short press: Turn off display
                display_state.display_on = false;
            }
            break;

        default:
            break;
    }
}

} // namespace ACDisplay

#endif // ENABLE_AC_DISPLAY
