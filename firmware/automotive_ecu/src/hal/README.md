# Hardware Abstraction Layer (HAL)

**Platform-independent hardware access for automotive ECU**

---

## Overview

The HAL provides a consistent API across different microcontroller platforms, allowing the same ECU code to run on:

- **STM32F407** (ARM Cortex-M4, entry-level automotive)
- **TI C2000 F28379D** (motor control specialist)
- **NXP S32K148** (production automotive, ISO 26262)
- **Infineon AURIX TC375** (high-end automotive)
- **TI Hercules RM46** (safety-critical automotive)

---

## Directory Structure

```
hal/
├── hal_interface.h          # Platform-independent API (ALL platforms implement this)
│
├── stm32f407/               # STM32F407 implementation
│   ├── hal_stm32f407.cpp    # STM32-specific implementation
│   └── pin_mapping.h        # Pin assignments for Nucleo-F407ZG
│
├── ti_c2000/                # TI C2000 implementation (future)
│   ├── hal_c2000.cpp
│   └── pin_mapping.h
│
├── nxp_s32k/                # NXP S32K148 implementation (future)
│   ├── hal_s32k.cpp
│   └── pin_mapping.h
│
└── README.md                # This file
```

---

## Supported Functions

### GPIO (Digital I/O)
- `HAL_GPIO_SetMode()` - Configure pin mode (input, output, analog, PWM)
- `HAL_GPIO_Write()` - Write digital output
- `HAL_GPIO_Read()` - Read digital input
- `HAL_GPIO_Toggle()` - Toggle output state

### ADC (Analog Input)
- `HAL_ADC_Read()` - Read analog value (12-bit typical)
- `HAL_ADC_ReadMillivolts()` - Read voltage in mV

### PWM (Pulse Width Modulation)
- `HAL_PWM_Write()` - Set PWM duty cycle (8-bit)
- `HAL_PWM_Write16()` - Set PWM duty cycle (16-bit)
- `HAL_PWM_SetFrequency()` - Configure PWM frequency

### CAN (Controller Area Network)
- `HAL_CAN_Init()` - Initialize CAN controller
- `HAL_CAN_Transmit()` - Send CAN message
- `HAL_CAN_Receive()` - Receive CAN message (non-blocking)
- `HAL_CAN_Available()` - Check if message waiting
- `HAL_CAN_SetFilter()` - Configure message filters

### UART (Serial Communication)
- `HAL_UART_Init()` - Initialize UART port
- `HAL_UART_Write()` - Send data
- `HAL_UART_Read()` - Receive data (non-blocking)
- `HAL_UART_Available()` - Check bytes available

### Watchdog Timer
- `HAL_Watchdog_Init()` - Initialize hardware watchdog
- `HAL_Watchdog_Kick()` - Reset watchdog timer (MUST be called regularly)

### System Functions
- `HAL_Init()` - Initialize platform (clocks, peripherals)
- `HAL_GetTick()` - Get milliseconds since boot
- `HAL_Delay()` - Delay for specified time
- `HAL_GetDeviceID()` - Get unique device ID

---

## Platform Comparison

| Feature | STM32F407 | TI C2000 | NXP S32K148 | AURIX TC375 | Hercules RM46 |
|---------|-----------|----------|-------------|-------------|---------------|
| **CPU** | ARM M4F @ 168MHz | C28x @ 200MHz | ARM M4F @ 112MHz | TriCore @ 300MHz | ARM R4F @ 220MHz |
| **Flash** | 1 MB | 1 MB | 2 MB | 4 MB | 3 MB |
| **RAM** | 192 KB | 204 KB | 256 KB | 500 KB | 256 KB |
| **CAN** | 2x | 2x | 3x | 4x | 3x |
| **ADC** | 3x 12-bit | 4x 16-bit | 2x 12-bit | 8x 12-bit | 3x 12-bit |
| **PWM** | 14 timers | 16 ePWM | 8 FTM | 44 GTM | 24 N2HET |
| **Temp Range** | -40 to 85°C | -40 to 125°C | -40 to 150°C | -40 to 150°C | -40 to 125°C |
| **Safety** | None | None | ISO 26262 ASIL-B | ASIL-D | ASIL-D |
| **Cost** | $15 | $30 | $50 | $100+ | $80 |
| **Best For** | DIY, prototyping | Motor control | Production ECU | High-end automotive | Safety-critical |

---

## Adding a New Platform

To add support for a new microcontroller:

1. **Create platform directory**:
   ```bash
   mkdir hal/your_platform/
   ```

2. **Implement HAL interface**:
   ```cpp
   // hal/your_platform/hal_your_platform.cpp
   #ifdef MCU_PLATFORM_YOUR_PLATFORM

   #include "../hal_interface.h"

   // Implement all functions from hal_interface.h
   void HAL_GPIO_SetMode(uint8_t pin, HAL_GPIO_Mode mode) {
       // Your implementation
   }

   // ... implement all other functions

   #endif
   ```

3. **Create pin mapping**:
   ```cpp
   // hal/your_platform/pin_mapping.h
   #define CAN1_TX_PIN     ...
   #define CAN1_RX_PIN     ...
   // ... define all pins
   ```

4. **Update vehicle_config.h**:
   ```cpp
   #define MCU_YOUR_PLATFORM  5
   #define MCU_PLATFORM       MCU_YOUR_PLATFORM
   ```

5. **Update platformio.ini**:
   ```ini
   [env:your_platform]
   platform = ...
   board = ...
   lib_deps = ...
   ```

6. **Test thoroughly**:
   - Bench test (CAN loopback, ADC, PWM)
   - Vehicle test (immobilizer, dashboard, ABS)

---

## Platform-Specific Notes

### STM32F407

- **Voltage**: 3.3V logic (NOT 5V tolerant)
- **CAN**: Requires external transceiver (TJA1050 or MCP2551)
- **Watchdog**: Independent watchdog (IWDG), 4s timeout
- **Libraries**: STM32duino, STM32_CAN
- **Documentation**: [STM32F407 Datasheet](https://www.st.com/resource/en/datasheet/stm32f407vg.pdf)

### TI C2000 (Future)

- **Voltage**: 3.3V core, 5V I/O tolerant
- **CAN**: Built-in CAN-FD capable
- **Watchdog**: Configurable watchdog timer
- **PWM**: Advanced ePWM for motor control
- **Documentation**: [C2000 MCU Guide](https://www.ti.com/lit/ug/spru566a/spru566a.pdf)

### NXP S32K148 (Future)

- **Voltage**: 5V tolerant I/O
- **CAN**: 3x CAN-FD controllers
- **Safety**: ISO 26262 ASIL-B capable
- **Temperature**: -40 to 150°C (automotive grade)
- **Documentation**: [S32K Reference Manual](https://www.nxp.com/docs/en/reference-manual/S32K1XXRM.pdf)

---

## Testing HAL

### Bench Testing

```cpp
void test_hal() {
    // Test GPIO
    HAL_GPIO_SetMode(LED_PIN, HAL_GPIO_MODE_OUTPUT);
    HAL_GPIO_Write(LED_PIN, true);
    HAL_Delay(1000);
    HAL_GPIO_Write(LED_PIN, false);

    // Test ADC
    uint16_t adc_value = HAL_ADC_Read(THROTTLE_ADC_PIN);
    Serial.printf("ADC: %d\n", adc_value);

    // Test PWM
    HAL_GPIO_SetMode(THROTTLE_PWM_PIN, HAL_GPIO_MODE_PWM);
    HAL_PWM_Write(THROTTLE_PWM_PIN, 128);  // 50% duty

    // Test CAN
    HAL_CAN_Init(500000);  // 500 kbps
    HAL_CAN_Message msg = {0x201, 8, {0}, false};
    HAL_CAN_Transmit(&msg);

    // Test Watchdog
    HAL_Watchdog_Kick();  // Should prevent reset
}
```

### Vehicle Testing

1. **CAN Bus**: Use CAN analyzer to verify messages
2. **Immobilizer**: Test keyless entry handshake
3. **Dashboard**: Verify gauges update correctly
4. **ABS**: Check ABS light status
5. **Throttle**: Test pedal response (bench first!)

---

## Safety Considerations

⚠️ **CRITICAL**: All HAL implementations MUST:

1. **Implement watchdog correctly** - Failure = vehicle reset while driving
2. **Validate pin assignments** - Wrong pin = potential damage
3. **Check voltage levels** - 3.3V MCU + 5V signal = dead MCU
4. **Test failsafe mode** - CAN timeout must not crash vehicle
5. **Verify temperature range** - Engine bay can exceed 120°C

---

## Contributing

When implementing HAL for a new platform:

1. Follow the interface defined in `hal_interface.h` exactly
2. Document platform-specific limitations
3. Provide complete pin mapping
4. Test all functions thoroughly
5. Update this README with platform details

---

*Last Updated: 2025-11-16*
*Part of Phase 5 unified automotive ECU architecture*
