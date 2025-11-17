/**
 * @file features.h
 * @brief Unified UI controller configuration for RX8 non-critical features
 *
 * This file contains ALL non-critical features for ESP32.
 * All display modules, wipers, WiFi, etc. unified in one firmware.
 *
 * @author Unified Architecture (Phase 5)
 * @date 2025-11-16
 */

#pragma once

// ============================================================================
// DISPLAY FEATURES - Enable the displays you have installed
// ============================================================================
#define ENABLE_AC_DISPLAY       1    // Factory AC display controller
#define ENABLE_OLED_GAUGES      1    // Aftermarket OLED gauge displays
#define ENABLE_COOLANT_MONITOR  1    // Dedicated coolant temp/pressure monitor

// ============================================================================
// CONTROL FEATURES
// ============================================================================
#define ENABLE_WIPERS           1    // Speed-sensitive wiper control
#define ENABLE_BUTTONS          1    // Button/encoder input processing

// ============================================================================
// CONNECTIVITY FEATURES
// ============================================================================
#define ENABLE_WIFI             1    // WiFi connectivity
#define ENABLE_BLUETOOTH        1    // Bluetooth connectivity
#define ENABLE_OTA_UPDATES      1    // Over-the-air firmware updates
#define ENABLE_WEB_DASHBOARD    1    // Web-based dashboard/configuration

// WiFi configuration
#define WIFI_SSID               "RX8-Telemetry"  // Change this!
#define WIFI_PASSWORD           "changeme123"    // Change this!
#define WIFI_HOSTNAME           "rx8-ecu"

// ============================================================================
// DATA LOGGING FEATURES
// ============================================================================
#define ENABLE_SD_LOGGING       0    // SD card data logging
#define ENABLE_CLOUD_UPLOAD     0    // Upload to cloud (MQTT/HTTP)

// ============================================================================
// COMMUNICATION WITH AUTOMOTIVE ECU
// ============================================================================
#define USE_CAN_BRIDGE          0    // Read vehicle data from CAN bus
#define USE_UART_BRIDGE         1    // Read vehicle data from UART

#if USE_UART_BRIDGE
    #define UART_BAUDRATE       115200
    #define UART_RX_PIN         16   // ESP32 GPIO16 (RX2)
    #define UART_TX_PIN         17   // ESP32 GPIO17 (TX2)
#endif

#if USE_CAN_BRIDGE
    #define CAN_TX_PIN          5    // ESP32 GPIO5
    #define CAN_RX_PIN          4    // ESP32 GPIO4
    #define CAN_SPEED           500000  // 500 kbps
#endif

// ============================================================================
// AC DISPLAY CONFIGURATION (if ENABLE_AC_DISPLAY)
// ============================================================================
#if ENABLE_AC_DISPLAY

// Pin definitions for AC display (Arduino Mega 2560 pinout, adapt for ESP32)
#define AC_DISPLAY_SPI_MOSI     23   // ESP32 default SPI MOSI
#define AC_DISPLAY_SPI_SCK      18   // ESP32 default SPI SCK
#define AC_DISPLAY_SPI_SS       5    // ESP32 GPIO5 (chip select)

// AC amplifier serial communication
#define AC_AMP_SERIAL_RX        16   // ESP32 GPIO16
#define AC_AMP_SERIAL_TX        17   // ESP32 GPIO17
#define AC_AMP_BAUDRATE         9600

// Rotary encoders (interrupt-capable pins)
#define FAN_ENCODER_A           19   // ESP32 GPIO19
#define FAN_ENCODER_B           21   // ESP32 GPIO21
#define TEMP_ENCODER_A          22   // ESP32 GPIO22
#define TEMP_ENCODER_B          23   // ESP32 GPIO23

// Button matrix
#define BUTTON_ROW_PINS         {25, 26, 27, 14}  // 4 rows
#define BUTTON_COL_PINS         {12, 13}          // 2 columns

// Backlight PWM
#define BACKLIGHT_PIN           32   // ESP32 GPIO32 (PWM capable)

// RTC (I2C)
#define RTC_SDA                 21   // ESP32 default I2C SDA
#define RTC_SCL                 22   // ESP32 default I2C SCL

#endif // ENABLE_AC_DISPLAY

// ============================================================================
// OLED GAUGES CONFIGURATION (if ENABLE_OLED_GAUGES)
// ============================================================================
#if ENABLE_OLED_GAUGES

#define OLED_COUNT              4    // Number of OLED displays

// I2C addresses for multiple OLEDs (use TCA9548A multiplexer if needed)
#define OLED_I2C_ADDRS          {0x3C, 0x3D, 0x3E, 0x3F}

// Display layout
#define OLED_DISPLAY_RPM        0    // Display index for RPM
#define OLED_DISPLAY_SPEED      1    // Display index for speed
#define OLED_DISPLAY_COOLANT    2    // Display index for coolant temp
#define OLED_DISPLAY_VOLTAGE    3    // Display index for battery voltage

#endif // ENABLE_OLED_GAUGES

// ============================================================================
// COOLANT MONITOR CONFIGURATION (if ENABLE_COOLANT_MONITOR)
// ============================================================================
#if ENABLE_COOLANT_MONITOR

// Dedicated OLED for coolant monitoring
#define COOLANT_OLED_I2C_ADDR   0x3C
#define COOLANT_OLED_WIDTH      128
#define COOLANT_OLED_HEIGHT     32

// Temperature sensor (analog)
#define COOLANT_TEMP_PIN        36   // ESP32 GPIO36 (ADC1_CH0)

// Pressure sensor (analog)
#define COOLANT_PRESSURE_PIN    39   // ESP32 GPIO39 (ADC1_CH3)

// Warning thresholds
#define COOLANT_TEMP_WARNING    95   // °C
#define COOLANT_TEMP_CRITICAL   105  // °C
#define COOLANT_PRESSURE_MIN    0.5  // bar (minimum safe pressure)

#endif // ENABLE_COOLANT_MONITOR

// ============================================================================
// WIPER CONFIGURATION (if ENABLE_WIPERS)
// ============================================================================
#if ENABLE_WIPERS

#define WIPER_CONTROL_PIN       25   // ESP32 GPIO25 (relay control)
#define WIPER_SENSE_PIN         26   // ESP32 GPIO26 (optional position sense)

// Speed-sensitive delay thresholds (km/h)
#define WIPER_DELAY_STOP        3000  // ms (vehicle stopped)
#define WIPER_DELAY_SLOW        2000  // ms (< 20 km/h)
#define WIPER_DELAY_MEDIUM      1500  // ms (20-40 km/h)
#define WIPER_DELAY_FAST        1000  // ms (40-60 km/h)
#define WIPER_DELAY_VERY_FAST   500   // ms (60+ km/h)

#endif // ENABLE_WIPERS

// ============================================================================
// WEB DASHBOARD CONFIGURATION (if ENABLE_WEB_DASHBOARD)
// ============================================================================
#if ENABLE_WEB_DASHBOARD

#define WEB_SERVER_PORT         80
#define WEBSOCKET_PORT          81   // For real-time data streaming

// Update rate for web dashboard (ms)
#define WEB_UPDATE_RATE_MS      100

#endif // ENABLE_WEB_DASHBOARD

// ============================================================================
// FREERTOS TASK CONFIGURATION
// ============================================================================
#define TASK_DISPLAY_PRIORITY   1    // Display update task
#define TASK_NETWORK_PRIORITY   1    // WiFi/Bluetooth task
#define TASK_CAN_PRIORITY       2    // CAN/UART listening task (higher priority)
#define TASK_WIPER_PRIORITY     1    // Wiper control task

#define TASK_DISPLAY_STACK      4096  // Stack size in bytes
#define TASK_NETWORK_STACK      8192  // Larger stack for network operations
#define TASK_CAN_STACK          2048
#define TASK_WIPER_STACK        2048

// ============================================================================
// DEBUG CONFIGURATION
// ============================================================================
#define ENABLE_SERIAL_DEBUG     1    // Serial console output
#define SERIAL_BAUDRATE         115200

// Debug levels
#define DEBUG_LEVEL_NONE        0
#define DEBUG_LEVEL_ERROR       1
#define DEBUG_LEVEL_WARN        2
#define DEBUG_LEVEL_INFO        3
#define DEBUG_LEVEL_VERBOSE     4

#define DEBUG_LEVEL             DEBUG_LEVEL_INFO  // <-- SET THIS

// ============================================================================
// VALIDATION
// ============================================================================
#if USE_CAN_BRIDGE && USE_UART_BRIDGE
    #error "Cannot enable both CAN_BRIDGE and UART_BRIDGE - choose one"
#endif

#if !USE_CAN_BRIDGE && !USE_UART_BRIDGE
    #error "Must enable either CAN_BRIDGE or UART_BRIDGE for vehicle data"
#endif

#if ENABLE_OTA_UPDATES && !ENABLE_WIFI
    #error "OTA_UPDATES requires WIFI to be enabled"
#endif

#if ENABLE_WEB_DASHBOARD && !ENABLE_WIFI
    #error "WEB_DASHBOARD requires WIFI to be enabled"
#endif

// ============================================================================
// FEATURE COUNT (for info display)
// ============================================================================
#define FEATURE_COUNT ( \
    ENABLE_AC_DISPLAY + \
    ENABLE_OLED_GAUGES + \
    ENABLE_COOLANT_MONITOR + \
    ENABLE_WIPERS + \
    ENABLE_WIFI + \
    ENABLE_BLUETOOTH + \
    ENABLE_OTA_UPDATES + \
    ENABLE_WEB_DASHBOARD + \
    ENABLE_SD_LOGGING + \
    ENABLE_CLOUD_UPLOAD \
)
