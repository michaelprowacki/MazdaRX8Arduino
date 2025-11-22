# ESP32 Migration Plan for AC Display Module

## Overview

This document outlines the migration plan to consolidate the **AC Display Module** (Arduino Mega 2560) and **ESP8266 Companion** into a **single ESP32 board**.

**Status**: Planning / Documentation
**Created**: 2025-11-15
**Consolidation Goal**: 2 boards → 1 board

---

## Current Architecture (2 Boards)

### Board 1: Arduino Mega 2560 - AC Display Controller
- **Purpose**: Main AC display logic, button scanning, display control
- **CPU**: ATmega2560 @ 16 MHz
- **Flash**: 256 KB
- **RAM**: 8 KB
- **I/O**: 54 digital, 16 analog
- **Libraries**:
  - Adafruit RTClib (I2C RTC)
  - Adafruit BusIO
  - Encoder (rotary encoders on interrupts)
  - Smoothed (analog smoothing)
  - 13 custom libraries (button, display, menu, etc.)

### Board 2: ESP8266 - WiFi/Bluetooth Companion
- **Purpose**: Wireless connectivity, OBD-II logging, web interface
- **CPU**: ESP8266 @ 80/160 MHz
- **Flash**: 4 MB
- **RAM**: 80 KB
- **Connectivity**: WiFi only
- **Libraries**:
  - ElegantOTA (OTA updates)
  - ESPAsyncWebServer
  - ESPAsyncTCP
  - TaskScheduler
  - ELMo (OBD-II ELM327 communication)

### Communication Between Boards
- Serial UART connection (Mega Serial3 ↔ ESP8266 Serial)
- Custom protocol for data exchange
- Additional wiring complexity

---

## Proposed Architecture (1 Board)

### ESP32 - Unified AC Display + WiFi
- **Board**: ESP32 DevKit v1 or ESP32-WROOM-32
- **CPU**: Dual-core Xtensa LX6 @ 240 MHz
- **Flash**: 4 MB
- **RAM**: 520 KB
- **I/O**: 34 digital, 18 analog (12-bit ADC)
- **Connectivity**: WiFi + Bluetooth Classic + BLE
- **Advantages**:
  - Native WiFi/Bluetooth (no companion needed)
  - More RAM (520 KB vs 8 KB)
  - Faster CPU (240 MHz vs 16 MHz)
  - Dual-core (one for display, one for WiFi)
  - Same interrupt capabilities
  - More cost-effective
  - Simpler wiring

---

## Migration Benefits

### Hardware Simplification
✅ **Single board** instead of two
✅ **No inter-board serial communication** required
✅ **Fewer power supplies** (5V only)
✅ **Smaller physical footprint**
✅ **Lower total cost** ($8 ESP32 vs $15 Mega + $5 ESP8266)

### Performance Improvements
✅ **Faster processing** (240 MHz vs 16 MHz)
✅ **More RAM** for buffering and features
✅ **Dual-core multitasking** (UI + networking)
✅ **Native Bluetooth** for mobile app integration
✅ **Better WiFi** (802.11 b/g/n)

### Software Benefits
✅ **Unified codebase** (no protocol between boards)
✅ **Shared data structures** (no serialization)
✅ **FreeRTOS multitasking** built-in
✅ **Better debugging** (single board)

---

## Compatibility Analysis

### Pin Compatibility

| Function | Mega 2560 | ESP32 | Notes |
|----------|-----------|-------|-------|
| **Encoders** | 2, 3, 18, 19 (INT) | 12, 13, 14, 27 | ESP32 all pins support INT |
| **Button Matrix** | 22-25 (rows), 29, 31 (cols) | 16, 17, 18, 19, 23, 25 | Any GPIO works |
| **SPI Display** | 51 (MOSI), 52 (SCK), 53 (SS) | 23 (MOSI), 18 (SCK), 5 (SS) | Use VSPI bus |
| **AC Amp Serial** | 16 (TX), 17 (RX) | 17 (TX2), 16 (RX2) | Use Serial2 |
| **Backlight PWM** | 9, 12 | 25, 26 | ESP32 has 16 PWM channels |
| **I2C RTC** | 20 (SDA), 21 (SCL) | 21 (SDA), 22 (SCL) | Compatible |
| **Analog** | A4, A8, A11 (10-bit) | 32, 33, 34 (12-bit) | Higher resolution! |

### Library Compatibility

| Library | Mega Support | ESP32 Support | Migration Notes |
|---------|--------------|---------------|-----------------|
| **Adafruit RTClib** | ✅ Yes | ✅ Yes | No changes needed |
| **Adafruit BusIO** | ✅ Yes | ✅ Yes | No changes needed |
| **Encoder** | ✅ Yes | ✅ Yes | ESP32-Encoder alternative available |
| **Smoothed** | ✅ Yes | ✅ Yes | Platform independent |
| **ElegantOTA** | ❌ No | ✅ Yes | Native ESP32 support |
| **ESPAsyncWebServer** | ❌ No | ✅ Yes | Native ESP32 support |
| **TaskScheduler** | ✅ Yes | ✅ Yes | Platform independent |
| **Custom Libraries** | ✅ Yes | ⚠️ Port | May need pin adjustments |

**Verdict**: 95% compatible - only pin definitions need updating

---

## Migration Steps

### Phase 1: Hardware Preparation
1. ✅ Obtain ESP32 development board (DevKit v1 recommended)
2. ✅ Create pin mapping document (Mega → ESP32)
3. ✅ Test basic peripherals:
   - I2C RTC communication
   - SPI display communication
   - Rotary encoder interrupts
   - Button matrix scanning
   - Serial communication with AC amplifier

### Phase 2: Code Migration
1. ✅ Create new PlatformIO project for ESP32
2. ✅ Copy custom libraries to new project
3. ✅ Update `pins.h` with ESP32 pin mappings
4. ✅ Update `platformio.ini` with ESP32 configuration
5. ✅ Integrate WiFi code directly into main application
6. ✅ Test compilation and fix any compatibility issues

### Phase 3: WiFi/OTA Integration
1. ✅ Merge ESP8266 Companion code into main application
2. ✅ Create separate FreeRTOS task for WiFi/web server
3. ✅ Implement web interface for configuration
4. ✅ Add OTA update capability
5. ✅ Add Bluetooth support for mobile app (future)

### Phase 4: Testing & Validation
1. ✅ Bench test all display functions
2. ✅ Test button inputs and encoders
3. ✅ Test AC amplifier communication
4. ✅ Test WiFi connectivity and web interface
5. ✅ Test OTA updates
6. ✅ Vehicle integration testing

---

## Recommended ESP32 Pin Mapping

```cpp
// ========== ESP32 Pin Definitions ==========

// Rotary Encoders (interrupt-capable)
#define ENC1_A 12
#define ENC1_B 13
#define ENC2_A 14
#define ENC2_B 27

// Button Matrix
#define BTN_ROW1 16
#define BTN_ROW2 17
#define BTN_ROW3 18
#define BTN_ROW4 19
#define BTN_COL1 23
#define BTN_COL2 25

// SPI Display (VSPI)
#define SPI_MOSI 23
#define SPI_CLK  18
#define SPI_CS   5

// AC Amplifier Serial (UART2)
#define AC_TX 17
#define AC_RX 16

// I2C RTC
#define I2C_SDA 21
#define I2C_SCL 22

// Backlight PWM
#define BACKLIGHT1 26
#define BACKLIGHT2 25

// Analog Inputs (12-bit ADC)
#define BATTERY_SENSE 32
#define DIMMER_SENSE  33
#define TEMP_SENSE    34
```

---

## PlatformIO Configuration for ESP32

```ini
; ESP32 Unified AC Display Controller
; Consolidates Mega 2560 + ESP8266 into single board

[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

; Serial monitor
monitor_speed = 115200
monitor_filters = default, time

; Build flags
build_flags =
    -D DEBUG
    -I include
    -Wall
    -Wextra
    -D ARDUINO_ARCH_ESP32

; Library dependencies (merged from both boards)
lib_deps =
    adafruit/Adafruit RTClib@^2.1.1
    adafruit/Adafruit BusIO@^1.14.5
    madhephaestus/ESP32Encoder@^0.10.1
    dirkschlager/Smoothed@^1.2
    ayushsharma82/ElegantOTA@^3.1.0
    me-no-dev/AsyncTCP@^1.1.1
    me-no-dev/ESPAsyncWebServer@^1.2.3
    arkhipenko/TaskScheduler@^3.7.0

; Custom libraries
lib_extra_dirs = lib/

; Partition scheme (for OTA)
board_build.partitions = default.csv

; Flash size
board_build.flash_size = 4MB

; OTA upload
upload_protocol = espota
upload_port = rx8-display.local  ; mDNS hostname
```

---

## Code Architecture for ESP32

### FreeRTOS Task Structure

```cpp
// Core 0: Display and UI (main loop runs here by default)
void setup() {
    // Initialize hardware
    initDisplay();
    initButtons();
    initEncoders();
    initRTC();
    initACAmplifier();

    // Create WiFi task on Core 1
    xTaskCreatePinnedToCore(
        wifiTask,      // Task function
        "WiFi",        // Name
        10000,         // Stack size
        NULL,          // Parameters
        1,             // Priority
        NULL,          // Task handle
        1              // Core 1
    );
}

void loop() {
    // Core 0: Handle display updates (time-critical)
    updateDisplay();
    scanButtons();
    processMenu();
    controlACAmplifier();
}

void wifiTask(void *parameter) {
    // Core 1: Handle WiFi/web server (non-critical)
    setupWiFi();
    setupWebServer();
    setupOTA();

    while(1) {
        handleWebServer();
        handleOTA();
        sendTelemetry();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
```

### Benefits of Dual-Core Architecture
- **Core 0**: Time-critical display updates (no WiFi delays)
- **Core 1**: Network operations (doesn't block UI)
- Smoother display refresh
- Responsive button input
- No WiFi-induced lag

---

## Migration Risks & Mitigation

### Risk 1: Custom Library Incompatibility
**Probability**: Low
**Impact**: Medium
**Mitigation**:
- Test each custom library independently
- Most are hardware-agnostic (menu system, state machines)
- Only `display` and `buttonPanel` need pin updates

### Risk 2: Timing Changes
**Probability**: Medium
**Impact**: Low
**Mitigation**:
- ESP32 runs faster (240 MHz), may need timing adjustments
- Use `delayMicroseconds()` for critical timing
- Test SPI clock speed with display

### Risk 3: Voltage Level Differences
**Probability**: High
**Impact**: Medium
**Mitigation**:
- ESP32 is **3.3V logic** (Mega is 5V)
- Most components are 3.3V/5V compatible
- Verify display and AC amplifier tolerance
- Use level shifters if needed

### Risk 4: Power Consumption
**Probability**: Low
**Impact**: Low
**Mitigation**:
- ESP32 WiFi consumes more power (~240mA vs ~100mA)
- Enable WiFi sleep mode when not needed
- Monitor vehicle battery voltage

---

## Cost Analysis

### Current Setup (2 Boards)
- Arduino Mega 2560: ~$15
- ESP8266 NodeMCU: ~$5
- Additional wiring/connectors: ~$3
- **Total: ~$23**

### Proposed Setup (1 Board)
- ESP32 DevKit v1: ~$8
- Simplified wiring: ~$1
- **Total: ~$9**

**Savings: $14 (61% reduction)**

---

## Development Timeline

| Phase | Estimated Time | Complexity |
|-------|----------------|------------|
| Hardware prep | 1-2 days | Low |
| Code migration | 3-5 days | Medium |
| WiFi integration | 2-3 days | Medium |
| Testing | 3-5 days | High |
| **Total** | **2-3 weeks** | **Medium** |

---

## Testing Checklist

### Hardware Tests
- [ ] I2C RTC communication
- [ ] SPI display refresh
- [ ] Rotary encoder interrupts
- [ ] Button matrix scanning
- [ ] AC amplifier serial communication
- [ ] Backlight PWM dimming
- [ ] Battery voltage reading
- [ ] 3.3V voltage levels verified

### Software Tests
- [ ] Menu navigation
- [ ] Display updates
- [ ] Clock display
- [ ] AC control functions
- [ ] WiFi connection
- [ ] Web interface
- [ ] OTA updates
- [ ] Data logging

### Integration Tests
- [ ] AC amplifier control in vehicle
- [ ] Button response time
- [ ] Display brightness in daylight
- [ ] WiFi range from vehicle
- [ ] Power consumption measurement
- [ ] Long-term stability test (24+ hours)

---

## Future Enhancements (ESP32 Only)

### Bluetooth Integration
- Mobile app for AC control
- BLE beacon for proximity detection
- Bluetooth OBD-II adapter support

### Advanced WiFi Features
- Access point mode for direct connection
- Web-based configuration portal
- Remote diagnostics via web interface
- Data logging to cloud services

### Display Improvements
- Color TFT display (ESP32 can drive it)
- Touch screen support
- Graphics and animations
- Custom themes

---

## Conclusion

**Recommendation**: ✅ **Proceed with ESP32 migration**

The ESP32 consolidation provides:
- Significant hardware simplification (2 boards → 1)
- Better performance (faster CPU, more RAM)
- Cost savings (61% reduction)
- Native WiFi/Bluetooth
- Future expandability

**Migration Risk**: 🟡 **LOW to MEDIUM**
- Most code is platform-independent
- Well-documented ESP32 Arduino framework
- Active community support
- Straightforward pin remapping

---

## References

- [ESP32 Arduino Core Documentation](https://docs.espressif.com/projects/arduino-esp32/)
- [ESP32 Pinout Reference](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)
- [ESP32Encoder Library](https://github.com/madhephaestus/ESP32Encoder)
- [AsyncWebServer for ESP32](https://github.com/me-no-dev/ESPAsyncWebServer)
- [FreeRTOS on ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html)

---

*Document Version: 1.0*
*Last Updated: 2025-11-15*
*Status: Planning Phase*
