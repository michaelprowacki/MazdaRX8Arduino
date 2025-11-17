# Build Procedures - RX8 Unified ECU

**Complete guide to building and flashing the unified firmware**

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Automotive ECU Build (Tier 1)](#automotive-ecu-build)
3. [ESP32 UI Controller Build (Tier 2)](#esp32-ui-controller-build)
4. [Configuration](#configuration)
5. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Required Software

**PlatformIO (Recommended)**:
```bash
# Install PlatformIO CLI
pip install -U platformio

# Or install VS Code extension
# https://platformio.org/install/ide?install=vscode
```

**Arduino IDE (Alternative)**:
- Download from https://www.arduino.cc/en/software
- Version 2.0+ recommended

### Required Libraries

**For Automotive ECU**:
- STM32duino (for STM32F407)
- TI C2000 SDK (for C2000)
- S32K SDK (for NXP S32K148)

**For ESP32 UI Controller**:
- Adafruit GFX Library
- Adafruit SSD1306
- PubSubClient (MQTT)
- WebServer

### Hardware Requirements

**Development Boards**:
- **STM32F407**: Nucleo-F407ZG ($25) or Black Pill ($10)
- **TI C2000**: LAUNCHXL-F28379D ($30)
- **NXP S32K148**: S32K148EVB ($100)
- **ESP32**: ESP32-WROOM-32 DevKit ($9)

**Tools**:
- USB cables (micro-USB or USB-C depending on board)
- CAN bus analyzer (optional but recommended): ~$30
- Multimeter for voltage checking
- Oscilloscope (optional, for PWM verification)

---

## Automotive ECU Build

### Option 1: PlatformIO (Recommended)

#### 1. Configure Platform

Edit `firmware/automotive_ecu/config/vehicle_config.h`:

```cpp
// Select your MCU platform
#define MCU_STM32F407    1  // STM32F407 (entry-level)
#define MCU_TI_C2000     2  // TI C2000 (motor control)
#define MCU_NXP_S32K148  3  // NXP S32K148 (production automotive)

#define MCU_PLATFORM     MCU_STM32F407  // <-- Change this

// Select vehicle type
#define VEHICLE_TYPE_ICE 1  // Internal combustion engine
#define VEHICLE_TYPE_EV  2  // Electric vehicle

#define VEHICLE_TYPE     VEHICLE_TYPE_ICE  // <-- Change this
```

#### 2. Select PlatformIO Environment

Open `firmware/automotive_ecu/platformio.ini` and select environment:

```bash
# Build for STM32F407
cd firmware/automotive_ecu
pio run -e stm32f407

# Build for TI C2000
pio run -e c2000

# Build for NXP S32K148
pio run -e s32k148
```

#### 3. Upload to Board

```bash
# Connect board via USB
# Upload (PlatformIO will auto-detect port)
pio run -e stm32f407 -t upload

# Monitor serial output
pio device monitor -b 115200
```

### Option 2: Arduino IDE

#### 1. Install Board Support

**For STM32F407**:
1. File → Preferences → Additional Board Manager URLs
2. Add: `https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json`
3. Tools → Board → Boards Manager → Search "STM32" → Install

**For ESP32**:
1. Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
2. Tools → Board → Boards Manager → Search "ESP32" → Install

#### 2. Select Board

- Tools → Board → STM32 Boards → Nucleo-144 → Nucleo F407ZG

#### 3. Install Libraries

Sketch → Include Library → Manage Libraries:
- Search "STM32duino CAN" → Install
- Search "IWatchdog" → Install

#### 4. Open and Upload

1. File → Open → `firmware/automotive_ecu/src/main.cpp`
2. Sketch → Verify/Compile
3. Sketch → Upload

---

## ESP32 UI Controller Build

### Option 1: PlatformIO

#### 1. Configure Features

Edit `firmware/ui_controller/config/features.h`:

```cpp
// Enable features you want
#define ENABLE_AC_DISPLAY       1  // Factory AC display
#define ENABLE_OLED_GAUGES      1  // Aftermarket OLED
#define ENABLE_WIPERS           1  // Speed-sensitive wipers
#define ENABLE_WIFI             1  // WiFi telemetry
#define ENABLE_WEB_DASHBOARD    1  // Web interface
```

#### 2. Configure WiFi

Edit `firmware/ui_controller/config/wifi_config.h`:

```cpp
#define WIFI_SSID     "YourWiFiName"
#define WIFI_PASSWORD "YourWiFiPassword"
#define WIFI_HOSTNAME "rx8-ecu"
```

#### 3. Build and Upload

```bash
cd firmware/ui_controller

# Build
pio run -e esp32dev

# Upload
pio run -e esp32dev -t upload

# Monitor
pio device monitor -b 115200
```

### Option 2: Arduino IDE

#### 1. Install ESP32 Board Support

(See above: Arduino IDE → ESP32 installation)

#### 2. Install Required Libraries

- Adafruit GFX Library
- Adafruit SSD1306
- PubSubClient
- ESPAsyncWebServer

#### 3. Select Board and Upload

- Tools → Board → ESP32 Arduino → ESP32 Dev Module
- Tools → Port → (select your ESP32)
- Sketch → Upload

---

## Configuration

### Vehicle-Specific Settings

#### Transmission Type

```cpp
// vehicle_config.h
#define TRANSMISSION_MANUAL 1
#define TRANSMISSION_AUTO   2
#define TRANSMISSION_TYPE   TRANSMISSION_MANUAL
```

#### ABS Variant

Some RX8s need different ABS byte values:

```cpp
// Try values 2, 3, or 4 if ABS light stays on
#define ABS_VARIANT  4  // Common: 2, 3, or 4
```

#### Throttle Pedal Calibration

RX8 throttle pedal voltage ranges vary slightly:

```cpp
// Default: 1.7V - 4.0V
// Adjust if needed based on your pedal
#define THROTTLE_VOLTAGE_MIN  170  // 1.7V in 0.01V units
#define THROTTLE_VOLTAGE_MAX  400  // 4.0V in 0.01V units
```

### Feature Enable/Disable

#### Automotive ECU

```cpp
// vehicle_config.h
#define ENABLE_IMMOBILIZER      1  // Immobilizer bypass
#define ENABLE_ABS_DSC          1  // ABS/DSC emulation
#define ENABLE_THROTTLE_PEDAL   1  // Throttle pedal (ICE only)
#define ENABLE_SAFETY_MONITOR   1  // Watchdog (recommended!)
```

#### ESP32 UI

```cpp
// features.h
#define ENABLE_AC_DISPLAY       1
#define ENABLE_OLED_GAUGES      1
#define ENABLE_COOLANT_MONITOR  0  // Disable if not installed
#define ENABLE_WIPERS           1
#define ENABLE_WIFI             1
#define ENABLE_BLUETOOTH        0  // Disable to save memory
#define ENABLE_WEB_DASHBOARD    1
```

---

## Build Verification

### Successful Build Output

**PlatformIO**:
```
[SUCCESS] Took 12.34 seconds
RAM:   [====      ]  42.1% (used 54872 bytes from 130560 bytes)
Flash: [=====     ]  51.2% (used 536428 bytes from 1048576 bytes)
```

**Arduino IDE**:
```
Sketch uses 536428 bytes (51%) of program storage space.
Global variables use 54872 bytes (42%) of dynamic memory.
```

### Common Build Errors

#### Error: "HAL_GPIO_SetMode not defined"

**Cause**: Wrong MCU_PLATFORM selected
**Fix**: Check `MCU_PLATFORM` matches your board in `vehicle_config.h`

#### Error: "WiFi.h: No such file"

**Cause**: ESP32 board support not installed
**Fix**: Install ESP32 board support in Arduino IDE or PlatformIO

#### Error: "multiple definition of `setup'"

**Cause**: Multiple .ino files in same folder
**Fix**: Only keep main.cpp in src/ folder

---

## Serial Debugging

### Enable Debug Output

```cpp
// vehicle_config.h or features.h
#define ENABLE_SERIAL_DEBUG  1  // Basic debug
#define ENABLE_SERIAL_DEBUG  2  // Verbose debug
#define ENABLE_SERIAL_DEBUG  3  // Very verbose (performance impact)
```

### Monitor Serial Output

**PlatformIO**:
```bash
pio device monitor -b 115200
```

**Arduino IDE**:
- Tools → Serial Monitor → 115200 baud

### Expected Startup Output

**Automotive ECU**:
```
╔══════════════════════════════════════╗
║  STM32F407 HAL Initialization        ║
╚══════════════════════════════════════╝
[HAL] CPU: ARM Cortex-M4F @ 168 MHz
[SAFETY] Watchdog initialized
[CAN] Controller initialized @ 500 kbps
[ENGINE] ICE control initialized
[IMMOB] Immobilizer bypass initialized
[ABS] ABS/DSC emulation initialized

=== AUTOMOTIVE ECU READY ===
```

**ESP32 UI**:
```
╔══════════════════════════════════════╗
║  RX8 UNIFIED UI CONTROLLER           ║
╚══════════════════════════════════════╝
[WIFI] Connecting to YourWiFi...Connected!
[WIFI] IP: 192.168.1.100
[UART] Bridge to automotive ECU initialized
[AC DISPLAY] Initialized
[OLED] OLED gauges initialized
[WEB] Web dashboard started on port 80
[WEB] Access at: http://192.168.1.100

=== UI CONTROLLER READY ===
```

---

## Build for Production

### Release Build (Optimized)

**PlatformIO**:
```ini
# platformio.ini
[env:stm32f407_release]
build_flags =
    -O3                    # Maximum optimization
    -DNDEBUG              # Disable asserts
    -DENABLE_SERIAL_DEBUG=0  # Disable debug output
```

```bash
pio run -e stm32f407_release
```

### Flash Protection

**STM32F407**:
```cpp
// Enable read protection (prevents firmware extraction)
// WARNING: Can brick board if done incorrectly!
// Consult STM32 documentation before enabling
```

---

## Next Steps

After successful build:
1. ✅ **Bench Testing** → See `TEST_PROCEDURES.md`
2. ✅ **CAN Bus Testing** → See `TEST_PROCEDURES.md`
3. ✅ **Vehicle Testing** → See `VEHICLE_TESTING.md`

---

**Last Updated**: 2025-11-16
**Part of Phase 5 unified architecture**
