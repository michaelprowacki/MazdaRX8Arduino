# Unified Firmware Architecture

**Phase 5: Complete Architectural Unification**

This directory contains the **unified firmware** that replaces the old modular approach.

## 🎯 Vision

**Before (Phases 1-4):** 9 separate modules, 9 Arduino boards, 9 codebases
**Now (Phase 5):** 2 unified firmwares, 2 boards, ALL features integrated

```
┌─────────────────────────────────────┐
│  Tier 1: Automotive MCU             │  ← ONE firmware, ALL critical features
│  - STM32F407 / C2000 / S32K          │     • ICE or EV control
│  - Engine/motor control             │     • CAN bus emulation
│  - CAN bus emulation                │     • Immobilizer
│  - Immobilizer bypass               │     • ABS/DSC
│  - ABS/DSC/traction                 │     • OBD-II
│  - OBD-II diagnostics               │     • Safety monitoring
│  - Safety monitoring                │
└─────────────────────────────────────┘
               ↕ UART or CAN
┌─────────────────────────────────────┐
│  Tier 2: ESP32                      │  ← ONE firmware, ALL UI features
│  - ALL displays unified:            │     • AC display
│    • AC display                     │     • OLED gauges
│    • OLED gauges                    │     • Coolant monitor
│    • Coolant monitor                │     • Wipers
│  - Wipers control                   │     • WiFi/Bluetooth
│  - WiFi telemetry                   │     • OTA updates
│  - Bluetooth connectivity           │     • Web dashboard
│  - OTA updates                      │
│  - Web dashboard                    │
└─────────────────────────────────────┘
```

---

## 📁 Structure

```
firmware/
├── automotive_ecu/          # Tier 1: Safety-critical automotive MCU
│   ├── src/                 # All critical features in one codebase
│   ├── hal/                 # Hardware abstraction (STM32/C2000/S32K)
│   ├── config/              # vehicle_config.h - configure your build
│   └── platformio.ini       # Multi-platform build configuration
│
├── ui_controller/           # Tier 2: Non-critical ESP32
│   ├── src/                 # All UI features in one codebase
│   ├── config/              # features.h - enable/disable features
│   └── platformio.ini       # ESP32 build configuration
│
└── shared/                  # Shared between both tiers
    ├── RX8_CAN_Protocol/    # CAN message definitions
    └── vehicle_state.h      # Communication protocol
```

---

## 🚀 Quick Start

### 1. Choose Your Hardware

**Tier 1 (Critical):**
| MCU | Cost | Skill Level | Best For |
|-----|------|-------------|----------|
| **STM32F407** | $15 | Beginner | ICE engine swaps |
| **TI C2000** | $30 | Intermediate | EV conversions, motor control |
| **NXP S32K148** | $50 | Advanced | Production, ISO 26262 |

**Tier 2 (Non-Critical):**
- **ESP32-WROOM-32** ($9) - Standard, dual-core
- **ESP32-S3** ($12) - Newer, better peripherals

### 2. Configure Automotive ECU

```bash
cd firmware/automotive_ecu
nano config/vehicle_config.h
```

**Set your vehicle type:**
```cpp
#define VEHICLE_TYPE            VEHICLE_TYPE_ICE  // or VEHICLE_TYPE_EV
#define TRANSMISSION_TYPE       TRANSMISSION_MANUAL  // or AUTO
#define MCU_PLATFORM            MCU_STM32F407  // or MCU_TI_C2000, MCU_NXP_S32K148
```

**Enable features you need:**
```cpp
#define ENABLE_IMMOBILIZER      1    // Immobilizer bypass
#define ENABLE_ABS_DSC          1    // ABS/DSC emulation
#define ENABLE_THROTTLE_PEDAL   1    // Throttle pedal (ICE only)
#define ENABLE_MOTOR_CONTROL    1    // Motor control (EV only)
```

### 3. Configure ESP32 UI

```bash
cd firmware/ui_controller
nano config/features.h
```

**Enable the displays/features you have:**
```cpp
#define ENABLE_AC_DISPLAY       1    // Factory AC display
#define ENABLE_OLED_GAUGES      1    // Aftermarket OLED displays
#define ENABLE_COOLANT_MONITOR  1    // Coolant temp/pressure monitor
#define ENABLE_WIPERS           1    // Speed-sensitive wipers
#define ENABLE_WIFI             1    // WiFi telemetry
#define ENABLE_WEB_DASHBOARD    1    // Web-based dashboard
```

**Set WiFi credentials:**
```cpp
#define WIFI_SSID               "Your_WiFi_SSID"
#define WIFI_PASSWORD           "your_password"
```

### 4. Build & Flash

**Automotive ECU:**
```bash
cd firmware/automotive_ecu
pio run -e stm32f407     # or -e c2000, -e s32k148
pio run -e stm32f407 -t upload
```

**ESP32 UI:**
```bash
cd firmware/ui_controller
pio run -e esp32dev
pio run -e esp32dev -t upload
```

### 5. Monitor Serial Output

```bash
# Automotive ECU
pio device monitor -b 115200

# ESP32
pio device monitor -b 115200
```

You should see the configuration and enabled features printed at startup.

---

## 🔧 Hardware Connections

### Tier 1: Automotive MCU

**Critical Connections:**
- **CAN Bus** → Vehicle CAN (High-speed, 500 kbps)
- **Throttle Pedal** → Analog input (1.7V - 4.0V)
- **Wheel Speed Sensors** → CAN (ID 0x4B1)
- **Power** → 12V vehicle power + voltage regulator

**Optional Connections:**
- **UART to ESP32** → TX/RX pins (115200 baud)
- **Immobilizer** → CAN (responds to ID 0x047)

### Tier 2: ESP32

**Display Connections:**
- **AC Display** → SPI (MOSI, SCK, CS)
- **OLED Gauges** → I2C (SDA, SCL)
- **Coolant Monitor** → I2C + Analog sensors

**Other Connections:**
- **Wipers** → GPIO (relay control)
- **UART from Automotive ECU** → RX/TX pins
- **WiFi** → Built-in (no external hardware)

---

## 📊 Features Comparison

| Feature | Old Modules | Unified Firmware |
|---------|-------------|------------------|
| **Hardware** | 9 boards ($150+) | 2 boards ($24) |
| **Codebases** | 9 separate projects | 2 unified projects |
| **Configuration** | Edit 9 different files | Edit 2 config headers |
| **Code Reuse** | 40% (lots of duplication) | 95% (shared libraries) |
| **Build Time** | Build 9 projects | Build 2 projects |
| **Safety** | Arduino (0-85°C) | Automotive MCU (-40 to 125°C) |
| **OTA Updates** | No | Yes (ESP32) |
| **Web Dashboard** | No | Yes (built-in) |
| **Maintainability** | Hard (9 projects) | Easy (2 projects) |

---

## 🛠️ Development Workflow

### Adding a New Feature

**Example: Add oil pressure monitoring**

1. **Update automotive ECU** (`automotive_ecu/config/vehicle_config.h`):
   ```cpp
   #define ENABLE_OIL_PRESSURE  1
   ```

2. **Add to vehicle state** (`shared/vehicle_state.h`):
   ```cpp
   uint16_t oil_pressure;  // PSI * 10
   ```

3. **Implement in ECU** (`automotive_ecu/src/peripherals/oil_pressure.cpp`):
   ```cpp
   uint16_t readOilPressure() {
       // Read analog sensor
       return adc_value * conversion_factor;
   }
   ```

4. **Display on ESP32** (`ui_controller/src/displays/oled_gauges.cpp`):
   ```cpp
   void OLEDGauges::update(VehicleState state) {
       display.printf("Oil: %.1f PSI", state.oil_pressure / 10.0f);
   }
   ```

5. **Rebuild both firmwares** - done!

### Testing

**Bench Testing:**
1. Flash automotive ECU
2. Connect to serial monitor
3. Verify features initialize correctly
4. Check CAN message transmission

**Integration Testing:**
1. Flash both firmwares
2. Connect automotive ECU ↔ ESP32 via UART
3. Verify vehicle state transmission
4. Check displays update correctly

**Vehicle Testing:**
1. Connect to vehicle CAN bus
2. Monitor CAN traffic (bus analyzer)
3. Verify immobilizer unlock
4. Test all features incrementally

---

## 📚 Documentation

- **[UNIFIED_ARCHITECTURE.md](../UNIFIED_ARCHITECTURE.md)** - Complete architectural overview
- **[automotive_ecu/README.md](automotive_ecu/README.md)** - Tier 1 detailed docs
- **[ui_controller/README.md](ui_controller/README.md)** - Tier 2 detailed docs
- **[../MIGRATION_FROM_LEGACY.md](../MIGRATION_FROM_LEGACY.md)** - How to migrate from old modules

---

## 🔐 Safety Notes

### Critical Automotive MCU Features

**Hardware Watchdog:**
```cpp
// Automotive ECU kicks watchdog every loop iteration
SafetyMonitor::kick();  // MUST be called < 1000ms
```

**Failsafe Mode:**
```cpp
// If no CAN RX for 500ms, enter failsafe:
// - Close throttle
// - Zero RPM
// - Turn on all warning lights
```

**Temperature Range:**
- Arduino Leonardo: 0°C to 85°C ❌
- STM32F407: -40°C to 85°C ⚠️
- TI C2000: -40°C to 125°C ✅ (automotive-grade)
- NXP S32K: -40°C to 125°C ✅ (automotive-grade)

### Isolation

- Automotive ECU and ESP32 run **independently**
- ESP32 failure does NOT affect engine control
- ESP32 only reads vehicle state (no critical control)

---

## 🎓 Learn More

**Automotive MCU Resources:**
- [STM32 Getting Started](https://www.st.com/en/microcontrollers-microprocessors/stm32f4-series.html)
- [TI C2000 Motor Control](https://www.ti.com/microcontrollers-mcus-processors/c2000-real-time-microcontrollers/overview.html)
- [NXP S32K Automotive](https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/s32k-automotive-mcus:S32K)

**ISO 26262 (Functional Safety):**
- [ISO 26262 Overview](https://www.iso.org/standard/68383.html)
- [Automotive ASIL Levels](https://en.wikipedia.org/wiki/Automotive_Safety_Integrity_Level)

---

## ❓ FAQ

**Q: Can I still use Arduino Leonardo?**
A: Yes, for bench testing. But **NOT recommended** for vehicle use (temperature range, no watchdog, safety concerns).

**Q: Do I need both tiers?**
A: No! Tier 1 (automotive ECU) is sufficient for basic ECU replacement. Tier 2 (ESP32) adds displays, WiFi, wipers.

**Q: Can I use just ESP32 for everything?**
A: **NO!** ESP32 is NOT automotive-grade. Use it only for non-critical UI features.

**Q: How do I migrate from old modules?**
A: See `MIGRATION_FROM_LEGACY.md` for step-by-step guide.

**Q: What if I only want ICE support?**
A: Set `VEHICLE_TYPE = VEHICLE_TYPE_ICE` in config. EV code won't be compiled.

**Q: Can I disable features I don't need?**
A: Yes! Set `ENABLE_XXX = 0` in config headers. Unused code won't be compiled.

---

## 🤝 Contributing

This unified architecture makes contributing easier:
- **One place** for ECU code (automotive_ecu/)
- **One place** for UI code (ui_controller/)
- **Shared libraries** benefit everyone
- **Configuration-driven** - easy to test different setups

See [CONTRIBUTING.md](../CONTRIBUTING.md) for guidelines.

---

## 📜 License

See [LICENSE](../LICENSE) for details.

---

*Last Updated: 2025-11-16*
*Phase 5: Unified Architecture*
*From 9 modules → 2 unified firmwares*
