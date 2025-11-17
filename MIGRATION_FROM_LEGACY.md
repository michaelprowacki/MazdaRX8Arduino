# Migration Guide: From Legacy Modules to Unified Architecture

**Moving from Phase 1-4 modular approach to Phase 5 unified firmware**

---

## Overview

| Aspect | Legacy (Phases 1-4) | Unified (Phase 5) |
|--------|---------------------|-------------------|
| **Architecture** | 9 separate modules | 2 unified firmwares |
| **Hardware** | 9 Arduino boards | 2 boards (automotive MCU + ESP32) |
| **Codebases** | 9 separate projects | 2 unified projects |
| **Configuration** | Edit multiple .ino files | Edit 2 config headers |
| **Safety** | Arduino (hobby-grade) | Automotive MCU (production-grade) |

---

## Migration Paths

### Path 1: ICE Engine Swap (Recommended for Most Users)

**You had:**
- `core/ECU_Module/` on Arduino Leonardo
- Optional: `archived/Wipers_Module/` on ATtiny

**You need:**
1. **Automotive MCU** (STM32F407, $15) for critical functions
2. **ESP32** (optional, $9) for wipers and displays

**Steps:**

1. **Configure automotive ECU:**
   ```bash
   cd firmware/automotive_ecu
   nano config/vehicle_config.h
   ```

   Set:
   ```cpp
   #define VEHICLE_TYPE        VEHICLE_TYPE_ICE
   #define MCU_PLATFORM        MCU_STM32F407
   #define ENABLE_IMMOBILIZER  1
   #define ENABLE_ABS_DSC      1
   ```

2. **Build and flash:**
   ```bash
   pio run -e stm32f407 -t upload
   ```

3. **Wire up:**
   - Connect CAN bus (same as Arduino Leonardo)
   - Connect throttle pedal (same pins via HAL)
   - Connect power (12V + regulator)

4. **Test:**
   - Bench test with CAN analyzer
   - Vehicle test (immobilizer, CAN messages)

5. **(Optional) Add ESP32 for wipers:**
   ```bash
   cd ../ui_controller
   nano config/features.h
   ```

   Set:
   ```cpp
   #define ENABLE_WIPERS  1
   #define ENABLE_AC_DISPLAY       0  // Unless you have it
   #define ENABLE_OLED_GAUGES      0
   #define ENABLE_COOLANT_MONITOR  0
   ```

   Connect wiper relay to ESP32 GPIO25.

**Result:** Same functionality as before, but on automotive-grade hardware with watchdog protection.

---

### Path 2: EV Conversion

**You had:**
- `core/EV_ECU_Module/` on Arduino Nano

**You need:**
1. **TI C2000** ($30) - Best for motor control
2. **ESP32** (optional, $9) for displays

**Steps:**

1. **Configure automotive ECU:**
   ```cpp
   #define VEHICLE_TYPE        VEHICLE_TYPE_EV
   #define MCU_PLATFORM        MCU_TI_C2000
   #define MOTOR_CONTROLLER    MOTOR_CTRL_OPEN_INV
   #define ENABLE_MOTOR_CONTROL    1
   #define ENABLE_REGEN_BRAKING    1
   ```

2. **Build for C2000:**
   ```bash
   pio run -e c2000 -t upload
   ```

3. **Connect to motor controller:**
   - CAN bus to OpenInverter/controller
   - Map throttle input
   - Configure regen braking

**Result:** Better motor control with automotive-grade MCU, built-in CAN, better PWM.

---

### Path 3: Full Display Integration

**You had:**
- `displays/AC_Display_Module/` on Arduino Mega
- `displays/Aftermarket_Display_Module/`
- `displays/Coolant_Monitor_Module/`
- Possibly `ESP8266_Companion/`

**You need:**
1. **ESP32** ($9) - Replaces Mega + ESP8266

**Steps:**

1. **Enable all displays in one firmware:**
   ```bash
   cd firmware/ui_controller
   nano config/features.h
   ```

   Set:
   ```cpp
   #define ENABLE_AC_DISPLAY       1
   #define ENABLE_OLED_GAUGES      1
   #define ENABLE_COOLANT_MONITOR  1
   #define ENABLE_WIPERS           1
   #define ENABLE_WIFI             1
   #define ENABLE_WEB_DASHBOARD    1
   ```

2. **Configure pins:**
   - Map AC display pins to ESP32 (see config/features.h)
   - Connect I2C for OLED displays
   - Connect analog sensors for coolant

3. **Build and flash:**
   ```bash
   pio run -e esp32dev -t upload
   ```

**Result:**
- **Before:** Arduino Mega ($15) + ESP8266 ($5) = $20, 2 boards
- **After:** ESP32 ($9), 1 board, WiFi built-in, dual-core multitasking

---

## Feature Mapping

### Legacy → Unified

| Legacy Module | New Location | Notes |
|---------------|--------------|-------|
| `core/ECU_Module/` | `firmware/automotive_ecu/` | Now on automotive MCU |
| `core/EV_ECU_Module/` | `firmware/automotive_ecu/` | Set `VEHICLE_TYPE = EV` |
| `displays/AC_Display_Module/` | `firmware/ui_controller/` | Set `ENABLE_AC_DISPLAY = 1` |
| `displays/Aftermarket_Display_Module/` | `firmware/ui_controller/` | Set `ENABLE_OLED_GAUGES = 1` |
| `displays/Coolant_Monitor_Module/` | `firmware/ui_controller/` | Set `ENABLE_COOLANT_MONITOR = 1` |
| `archived/Wipers_Module/` | `firmware/ui_controller/` | Set `ENABLE_WIPERS = 1` |
| `ESP8266_Companion/` | `firmware/ui_controller/` | Native WiFi on ESP32 |
| `specialized/Sim_Racing_Module/` | Not migrated | Standalone use case |
| `specialized/Dash_Controller_Module/` | Reference only | Alternative implementation |

### Code Migration

**OLD (modular):**
```cpp
// ECU_Module/RX8_CANBUS.ino
void setup() {
    CAN0.begin(CAN_500KBPS);
    // ... 300 lines of setup
}
```

**NEW (unified):**
```cpp
// firmware/automotive_ecu/src/main.cpp
#include "config/vehicle_config.h"

void setup() {
    HAL_Init();  // Hardware abstraction
    CANController::init(CAN_SPEED);

    #if ENABLE_IMMOBILIZER
        Immobilizer::init();
    #endif

    // All features configured via header
}
```

---

## Hardware Transitions

### Tier 1: Automotive ECU

| Old Hardware | New Hardware | Why Change? |
|--------------|--------------|-------------|
| Arduino Leonardo | STM32F407 | Better temp range, watchdog, built-in CAN |
| Arduino Nano (EV) | TI C2000 | Better motor control, automotive-grade |
| MCP2515 CAN module | Built-in CAN | More reliable, fewer components |
| ATtiny (wipers) | Integrated into ECU | Reduced complexity |

**Purchase Links:**
- [STM32F407 Nucleo Board](https://www.st.com/en/evaluation-tools/nucleo-f407zg.html) - $15
- [TI C2000 LaunchXL](https://www.ti.com/tool/LAUNCHXL-F28379D) - $30
- [NXP S32K148EVB](https://www.nxp.com/design/development-boards/automotive-development-platforms/s32k-mcu-platforms/s32k148-q176-evaluation-board-for-automotive-general-purpose:S32K148EVB) - $50

### Tier 2: ESP32

| Old Hardware | New Hardware | Why Change? |
|--------------|--------------|-------------|
| Arduino Mega 2560 | ESP32 | WiFi built-in, dual-core, more RAM |
| ESP8266 | ESP32 | Better performance, Bluetooth |

**Purchase Links:**
- [ESP32 DevKit](https://www.espressif.com/en/products/devkits/esp32-devkitc) - $9
- [ESP32-S3](https://www.espressif.com/en/products/socs/esp32-s3) - $12

---

## Pin Mapping

### Automotive ECU (STM32F407 Example)

| Function | Arduino Leonardo | STM32F407 |
|----------|------------------|-----------|
| CAN TX | Via MCP2515 SPI | PB9 (CAN1_TX) |
| CAN RX | Via MCP2515 SPI | PB8 (CAN1_RX) |
| Throttle ADC | A1 | PA0 (ADC1_IN0) |
| Throttle PWM Out | Pin 5 | PA8 (TIM1_CH1) |
| Status LED | Pin 7, 8 | PC13 (onboard LED) |

*Full pin mappings in `firmware/automotive_ecu/hal/<platform>/pin_mapping.h`*

### ESP32

| Function | Arduino Mega | ESP32 |
|----------|--------------|-------|
| AC Display SPI | 51, 52, 53 | 23, 18, 5 (VSPI) |
| I2C (OLED/RTC) | 20, 21 | 21, 22 (default) |
| UART (to ECU) | Serial3 | Serial2 (GPIO 16/17) |
| Wiper Control | N/A | GPIO25 |

*Full pin mappings in `firmware/ui_controller/config/features.h`*

---

## Testing Checklist

### ✅ Automotive ECU

- [ ] Compiles without errors
- [ ] Serial output shows configuration
- [ ] CAN messages transmit at 100ms interval
- [ ] Immobilizer handshake works
- [ ] Throttle pedal reads correctly
- [ ] Wheel speed calculation works
- [ ] Warning lights control works
- [ ] Watchdog kicks every loop
- [ ] Failsafe activates on CAN timeout
- [ ] Vehicle testing (immobilizer, dashboard, ABS)

### ✅ ESP32 UI Controller

- [ ] Compiles without errors
- [ ] WiFi connects successfully
- [ ] Web dashboard accessible
- [ ] Receives vehicle state from ECU
- [ ] AC display updates
- [ ] OLED gauges update
- [ ] Coolant monitor works
- [ ] Wipers respond to speed
- [ ] OTA updates work
- [ ] No memory leaks (FreeRTOS tasks stable)

---

## Troubleshooting

### Issue: "automotive ECU won't compile"

**Solution:**
1. Check PlatformIO is installed
2. Verify correct environment in platformio.ini
3. Check HAL for your platform exists
4. Try `pio lib install` to fetch dependencies

### Issue: "ESP32 won't connect to automotive ECU"

**Solution:**
1. Check UART wiring (TX→RX, RX→TX, GND)
2. Verify baud rate matches (115200)
3. Check `USE_UART_BRIDGE = 1` in config
4. Monitor serial output on both sides

### Issue: "CAN messages not transmitting"

**Solution:**
1. Check CAN bus termination (120Ω resistors)
2. Verify CAN transceiver wiring
3. Check CAN speed matches vehicle (500 kbps)
4. Use CAN analyzer to monitor bus

### Issue: "Immobilizer not unlocking"

**Solution:**
1. Monitor CAN traffic for ID 0x047
2. Verify response on ID 0x041
3. Check handshake bytes match
4. Ensure automotive ECU has priority on CAN bus

---

## Rollback Plan

If you need to rollback to legacy modules:

1. **Keep legacy code:**
   - Legacy modules preserved in `core/`, `displays/`, `archived/`
   - Git history intact

2. **Reflash Arduino:**
   ```bash
   cd core/ECU_Module
   # Use Arduino IDE or pio to flash RX8_CANBUS.ino
   ```

3. **Reconnect hardware:**
   - Same wiring as before
   - No changes to vehicle-side connections

---

## Support

**Documentation:**
- `UNIFIED_ARCHITECTURE.md` - Complete architecture overview
- `firmware/README.md` - Quick start for unified firmware
- `firmware/automotive_ecu/README.md` - Tier 1 details
- `firmware/ui_controller/README.md` - Tier 2 details

**Community:**
- GitHub Issues: https://github.com/michaelprowacki/MazdaRX8Arduino/issues
- Discussions: https://github.com/michaelprowacki/MazdaRX8Arduino/discussions

---

*Last Updated: 2025-11-16*
*Phase 5: Unified Architecture*
*Migration guide from legacy modular system*
