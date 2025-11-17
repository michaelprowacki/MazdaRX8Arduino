# Unified Two-Tier Architecture

**Status**: Phase 5 - Complete Architectural Unification
**Date**: 2025-11-16

---

## Vision

**Replace the modular approach with a unified two-tier architecture:**

### ❌ OLD: Separate Modules (9+ Arduino boards)
```
ECU_Module (Arduino Leonardo) - Engine control
EV_ECU_Module (Arduino Nano) - Motor control
AC_Display_Module (Arduino Mega) - AC display
Wipers_Module (ATtiny) - Wipers
ESP8266 - WiFi
... 9 separate boards, 9 separate codebases
```

### ✅ NEW: Unified Two-Tier System (2 boards)
```
┌─────────────────────────────────────────────────────────┐
│ TIER 1: AUTOMOTIVE MCU (Safety-Critical)               │
│ - STM32F407 / TI C2000 / NXP S32K                       │
│ - ONE unified firmware with ALL critical features:      │
│   ✓ ICE engine control OR EV motor control              │
│   ✓ CAN bus emulation (all messages)                    │
│   ✓ Immobilizer bypass                                  │
│   ✓ ABS/DSC/traction control                            │
│   ✓ OBD-II diagnostics                                  │
│   ✓ Safety monitoring (watchdog, failsafe)              │
└─────────────────────────────────────────────────────────┘
                            ↕ CAN Bus
┌─────────────────────────────────────────────────────────┐
│ TIER 2: ESP32 (Non-Critical)                            │
│ - ONE unified firmware with ALL UI features:            │
│   ✓ AC display control                                  │
│   ✓ Aftermarket OLED gauges                             │
│   ✓ Coolant monitor                                     │
│   ✓ Speed-sensitive wipers                              │
│   ✓ WiFi telemetry                                      │
│   ✓ Bluetooth connectivity                              │
│   ✓ Data logging                                        │
│   ✓ OTA updates                                         │
└─────────────────────────────────────────────────────────┘
```

---

## Architecture Principles

### 1. **Unification, Not Modularity**
- **No separate modules** - All features in one codebase per tier
- **Compile-time configuration** - Enable/disable features via config file
- **Shared code** - Single HAL, single CAN library, single state machine

### 2. **Safety-First Hardware**
- **Tier 1 (Critical)**: Automotive-grade MCU only
  - Temperature: -40°C to 125°C (not 0-85°C)
  - Hardware watchdog mandatory
  - Built-in CAN controllers (no MCP2515)
  - ISO 26262 capable (for certification path)
- **Tier 2 (Non-Critical)**: ESP32 or similar
  - WiFi/Bluetooth integrated
  - Sufficient RAM for UI tasks
  - Failure = graceful degradation (not crash)

### 3. **Configuration-Driven**
- **Vehicle profile**: ICE vs EV, manual vs auto, wheel size, etc.
- **Feature flags**: Enable only what you need
- **Hardware abstraction**: Same code runs on STM32/C2000/S32K

---

## Unified Firmware Structure

```
firmware/
│
├── automotive_ecu/                  # 🚗 TIER 1: Automotive MCU
│   ├── src/
│   │   ├── main.cpp                 # Main entry point
│   │   ├── core/
│   │   │   ├── can_controller.cpp   # CAN bus management (all messages)
│   │   │   ├── engine_control.cpp   # ICE engine control (throttle, RPM, temp)
│   │   │   ├── motor_control.cpp    # EV motor control (torque, regen)
│   │   │   ├── immobilizer.cpp      # Security & key handshake
│   │   │   ├── abs_dsc.cpp          # ABS/DSC/traction control
│   │   │   ├── diagnostics.cpp      # OBD-II/diagnostic messages
│   │   │   └── safety_monitor.cpp   # Watchdog, failsafe, error handling
│   │   ├── peripherals/
│   │   │   ├── wheel_speed.cpp      # Wheel speed sensor processing
│   │   │   ├── throttle_pedal.cpp   # Throttle position sensing
│   │   │   └── power_steering.cpp   # Power steering enable
│   │   └── communication/
│   │       └── uart_bridge.cpp      # Communication with ESP32
│   │
│   ├── hal/                         # Hardware Abstraction Layer
│   │   ├── hal_interface.h          # Common HAL interface
│   │   ├── stm32/
│   │   │   ├── hal_stm32.cpp        # STM32F4xx implementation
│   │   │   ├── can_stm32.cpp
│   │   │   └── gpio_stm32.cpp
│   │   ├── c2000/
│   │   │   ├── hal_c2000.cpp        # TI C2000 implementation
│   │   │   └── can_c2000.cpp
│   │   └── s32k/
│   │       ├── hal_s32k.cpp         # NXP S32K implementation
│   │       └── can_s32k.cpp
│   │
│   ├── config/
│   │   ├── vehicle_config.h         # Vehicle profile (ICE/EV, manual/auto)
│   │   ├── feature_flags.h          # Enable/disable features
│   │   └── pin_mapping.h            # Pin definitions per board
│   │
│   ├── lib/
│   │   └── RX8_CAN_Protocol/        # Shared CAN message library
│   │       ├── can_messages.h
│   │       └── can_encoder.cpp
│   │
│   ├── platformio.ini               # PlatformIO config for all MCU targets
│   └── README.md
│
├── ui_controller/                   # 📱 TIER 2: ESP32
│   ├── src/
│   │   ├── main.cpp                 # Main entry point
│   │   ├── displays/
│   │   │   ├── ac_display.cpp       # Factory AC display control
│   │   │   ├── oled_gauges.cpp      # Aftermarket OLED displays
│   │   │   ├── coolant_monitor.cpp  # Coolant temp/pressure display
│   │   │   └── display_manager.cpp  # Display multiplexing
│   │   ├── controls/
│   │   │   ├── wipers.cpp           # Speed-sensitive wipers
│   │   │   └── buttons.cpp          # Button/encoder input
│   │   ├── connectivity/
│   │   │   ├── wifi_telemetry.cpp   # WiFi data logging
│   │   │   ├── bluetooth.cpp        # Bluetooth connectivity
│   │   │   ├── ota_update.cpp       # Over-the-air updates
│   │   │   └── web_server.cpp       # Web dashboard
│   │   ├── communication/
│   │   │   ├── can_listener.cpp     # Read CAN data from automotive ECU
│   │   │   └── uart_bridge.cpp      # UART to automotive ECU
│   │   └── tasks/
│   │       ├── display_task.cpp     # FreeRTOS task for displays
│   │       ├── network_task.cpp     # FreeRTOS task for WiFi/BT
│   │       └── can_task.cpp         # FreeRTOS task for CAN
│   │
│   ├── config/
│   │   ├── features.h               # Enable/disable UI features
│   │   ├── wifi_config.h            # WiFi credentials
│   │   └── pin_mapping.h            # ESP32 pin definitions
│   │
│   ├── lib/
│   │   └── RX8_CAN_Protocol/        # Same shared CAN library
│   │
│   ├── platformio.ini               # PlatformIO config for ESP32
│   └── README.md
│
├── shared/                          # 📚 Shared between both tiers
│   ├── RX8_CAN_Protocol/            # CAN message definitions
│   │   ├── can_messages.h           # All CAN IDs and structures
│   │   ├── can_encoder.cpp          # Encoding functions
│   │   └── can_decoder.cpp          # Decoding functions
│   ├── vehicle_state.h              # Common vehicle state structure
│   └── protocol.h                   # UART communication protocol
│
└── tools/
    ├── configurator/                # GUI tool for generating configs
    ├── flash_automotive.sh          # Script to flash automotive MCU
    └── flash_esp32.sh               # Script to flash ESP32
```

---

## Configuration Examples

### Vehicle Config (automotive_ecu/config/vehicle_config.h)

```cpp
#pragma once

// ============================================================================
// VEHICLE TYPE
// ============================================================================
#define VEHICLE_TYPE_ICE        1    // Internal combustion engine
#define VEHICLE_TYPE_EV         2    // Electric vehicle

#define VEHICLE_TYPE            VEHICLE_TYPE_ICE  // <-- SET THIS

// ============================================================================
// TRANSMISSION
// ============================================================================
#define TRANSMISSION_MANUAL     1
#define TRANSMISSION_AUTO       2

#define TRANSMISSION_TYPE       TRANSMISSION_MANUAL

// ============================================================================
// WHEEL CONFIGURATION
// ============================================================================
#define WHEEL_SIZE_FRONT        225  // mm (tire width)
#define WHEEL_SIZE_REAR         225  // mm

// ============================================================================
// FEATURE FLAGS
// ============================================================================
#define ENABLE_ABS_DSC          1    // ABS/DSC emulation
#define ENABLE_IMMOBILIZER      1    // Immobilizer bypass
#define ENABLE_DIAGNOSTICS      1    // OBD-II support
#define ENABLE_POWER_STEERING   1    // Power steering enable

// ICE-specific (ignored if VEHICLE_TYPE == EV)
#define ENABLE_THROTTLE_PEDAL   1    // Throttle pedal processing

// EV-specific (ignored if VEHICLE_TYPE == ICE)
#define ENABLE_REGEN_BRAKING    0    // Regenerative braking
#define ENABLE_BATTERY_MONITOR  0    // Battery state monitoring

// ============================================================================
// HARDWARE PLATFORM
// ============================================================================
#define MCU_STM32F407           1
#define MCU_TI_C2000            2
#define MCU_NXP_S32K            3

#define MCU_PLATFORM            MCU_STM32F407  // <-- SET THIS
```

### Feature Flags (ui_controller/config/features.h)

```cpp
#pragma once

// ============================================================================
// DISPLAY FEATURES
// ============================================================================
#define ENABLE_AC_DISPLAY       1    // Factory AC display control
#define ENABLE_OLED_GAUGES      1    // Aftermarket OLED displays
#define ENABLE_COOLANT_MONITOR  1    // Coolant temp/pressure monitor

// ============================================================================
// CONTROL FEATURES
// ============================================================================
#define ENABLE_WIPERS           1    // Speed-sensitive wipers

// ============================================================================
// CONNECTIVITY FEATURES
// ============================================================================
#define ENABLE_WIFI             1    // WiFi telemetry
#define ENABLE_BLUETOOTH        1    // Bluetooth connectivity
#define ENABLE_OTA_UPDATES      1    // Over-the-air firmware updates
#define ENABLE_WEB_DASHBOARD    1    // Web-based dashboard

// ============================================================================
// DATA LOGGING
// ============================================================================
#define ENABLE_SD_LOGGING       0    // SD card data logging
#define ENABLE_CLOUD_UPLOAD     0    // Upload to cloud (MQTT/HTTP)
```

---

## Example: Unified Main Loop (Automotive ECU)

```cpp
// firmware/automotive_ecu/src/main.cpp

#include "config/vehicle_config.h"
#include "core/can_controller.h"
#include "core/safety_monitor.h"

#if VEHICLE_TYPE == VEHICLE_TYPE_ICE
  #include "core/engine_control.h"
#elif VEHICLE_TYPE == VEHICLE_TYPE_EV
  #include "core/motor_control.h"
#endif

#if ENABLE_IMMOBILIZER
  #include "core/immobilizer.h"
#endif

#if ENABLE_ABS_DSC
  #include "core/abs_dsc.h"
#endif

void setup() {
    // Initialize HAL (hardware abstraction layer)
    HAL_Init();

    // Initialize safety monitor (watchdog)
    SafetyMonitor::init();

    // Initialize CAN controller
    CANController::init();

    #if VEHICLE_TYPE == VEHICLE_TYPE_ICE
        EngineControl::init();
    #elif VEHICLE_TYPE == VEHICLE_TYPE_EV
        MotorControl::init();
    #endif

    #if ENABLE_IMMOBILIZER
        Immobilizer::init();
    #endif

    #if ENABLE_ABS_DSC
        ABS_DSC::init();
    #endif
}

void loop() {
    // Kick watchdog
    SafetyMonitor::kick();

    // Process CAN bus
    CANController::process();

    // Process vehicle control
    #if VEHICLE_TYPE == VEHICLE_TYPE_ICE
        EngineControl::update();
    #elif VEHICLE_TYPE == VEHICLE_TYPE_EV
        MotorControl::update();
    #endif

    // Process safety systems
    #if ENABLE_IMMOBILIZER
        Immobilizer::update();
    #endif

    #if ENABLE_ABS_DSC
        ABS_DSC::update();
    #endif
}
```

---

## Hardware Migration Path

### Recommended Automotive MCU Choices

| MCU | Cost | Pros | Best For |
|-----|------|------|----------|
| **STM32F407** | $15 | - Easy to get started<br>- Great tooling (STM32CubeIDE)<br>- Built-in dual CAN<br>- 168 MHz ARM Cortex-M4 | **Beginners, ICE swaps** |
| **TI C2000 (F28379D)** | $30 | - Best motor control (PWM)<br>- Dual-core<br>- Real-time control<br>- Automotive temp range | **EV conversions, motor control** |
| **NXP S32K148** | $50 | - True automotive MCU<br>- ISO 26262 ASIL-B capable<br>- CAN-FD support<br>- AUTOSAR-ready | **Production, certification** |

### ESP32 for Tier 2

- **ESP32-WROOM-32** ($9) - Dual-core 240 MHz, WiFi + Bluetooth
- **ESP32-S3** ($12) - Improved peripherals, USB-OTG

---

## Communication Between Tiers

### Option 1: CAN Bus (Recommended)
```
Automotive MCU ←→ CAN Bus ←→ ESP32 (with CAN transceiver)
```
- **Pros**: Standard protocol, galvanic isolation
- **Cons**: Requires CAN transceiver for ESP32 (~$2)

### Option 2: UART Bridge
```
Automotive MCU ←→ UART (TX/RX) ←→ ESP32
```
- **Pros**: Simple, no extra hardware
- **Cons**: No isolation, protocol overhead

### Protocol Example (UART)
```cpp
// Automotive MCU → ESP32
struct VehicleState {
    uint16_t rpm;
    uint16_t speed;  // km/h * 10
    int16_t  coolant_temp;  // °C * 10
    uint16_t battery_voltage;  // V * 100
    uint8_t  throttle;  // 0-100%
    uint8_t  warning_flags;
} __attribute__((packed));

// ESP32 → Automotive MCU
struct UICommands {
    uint8_t wiper_request;  // 0=off, 1=auto
    uint8_t reserved[7];
} __attribute__((packed));
```

---

## Benefits of Unified Architecture

### ✅ Cost Savings
- **Hardware**: 9 boards → 2 boards = **$150+ → $25**
- **Development boards**:
  - STM32F407 Nucleo: $15
  - ESP32 DevKit: $9
  - Total: **$24**

### ✅ Code Quality
- **Single source of truth** for CAN protocol
- **Shared libraries** eliminate duplication
- **Easier testing** (one codebase per tier)
- **Better maintainability** (fix once, applies everywhere)

### ✅ Safety
- **Automotive-grade hardware** for critical functions
- **Hardware watchdog** mandatory
- **Temperature resilience** (-40°C to 125°C)
- **Graceful degradation** (ESP32 failure doesn't crash engine)

### ✅ Features
- **All features available** in one build
- **Configure via header file** (no multiple projects)
- **OTA updates** for ESP32 (no need to remove from car)
- **WiFi telemetry** built-in

---

## Migration from Old Modular System

### Phase 1: Automotive ECU (Critical Path)
1. Choose MCU platform (STM32F407 recommended)
2. Port ECU_Module code to new HAL
3. Integrate immobilizer, ABS/DSC
4. Test on bench (CAN bus analyzer)
5. Test in vehicle (critical!)

### Phase 2: ESP32 UI Controller
1. Merge AC_Display, OLED, Coolant modules
2. Add WiFi telemetry
3. Test displays individually
4. Test CAN/UART bridge
5. Install in vehicle

### Phase 3: Deprecate Old Modules
1. Move old modules to `archived/`
2. Update documentation
3. Provide migration guides

---

## Development Workflow

### 1. Configure Your Build
```bash
# Edit vehicle config
nano firmware/automotive_ecu/config/vehicle_config.h

# Set vehicle type (ICE or EV)
# Set MCU platform (STM32/C2000/S32K)
# Enable features you need
```

### 2. Build Automotive ECU
```bash
cd firmware/automotive_ecu
pio run -e stm32f407    # or -e c2000, -e s32k
```

### 3. Build ESP32 UI
```bash
cd firmware/ui_controller
pio run -e esp32
```

### 4. Flash
```bash
# Automotive MCU (ST-Link/J-Link)
pio run -e stm32f407 -t upload

# ESP32 (USB)
pio run -e esp32 -t upload
```

---

## Next Steps

1. ✅ Create unified firmware structure
2. ✅ Implement HAL for STM32/C2000/S32K
3. ✅ Port ECU code to new architecture
4. ✅ Merge all display modules into ESP32 firmware
5. ✅ Test on development boards
6. ✅ Vehicle testing
7. ✅ Documentation and guides

---

## Questions?

See:
- `firmware/automotive_ecu/README.md` - Automotive MCU guide
- `firmware/ui_controller/README.md` - ESP32 UI guide
- `MIGRATION_FROM_LEGACY.md` - How to migrate from old modules

---

*Last Updated: 2025-11-16*
*Status: Phase 5 - Architectural Unification*
*Goal: One codebase per tier, not separate modules*
