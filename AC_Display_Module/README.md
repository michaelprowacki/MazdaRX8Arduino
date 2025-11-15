# AC Display Module for Mazda RX8

## Overview

This module is integrated from the [S1-RX8-AC-Display-controller](https://github.com/michaelprowacki/S1-RX8-AC-Display-controller) project. It provides AC (Air Conditioning) display control functionality for the Mazda S1 RX8.

**Purpose**: Control and interface with the factory AC display unit, enabling custom functionality and integration with aftermarket systems.

**Hardware**: Arduino Mega 2560 (can be adapted for other Arduino boards)

---

## Features

- **AC System Control**: Full control of AC amplifier and display functions
- **Button Panel Interface**: Manages all AC control buttons (Auto, Mode, A/C, Demist, etc.)
- **Display Management**: Controls 7-segment displays and status icons
- **Real-Time Clock**: Time display with 12/24 hour format support
- **Menu System**: Configuration menus, subwoofer volume control, and custom pages
- **Backlight Control**: Adaptive backlighting for dashboard integration
- **Temperature Monitoring**: Battery voltage and motor temperature display
- **ESP8266 Companion Support**: Optional Bluetooth/ESP-Now bridging for data logging
- **DSP Integration**: Digital Signal Processor control for audio systems

---

## Hardware Requirements

### Primary Components
- **Microcontroller**: Arduino Mega 2560 (ATmega2560)
- **Real-Time Clock**: RTC module (via Adafruit RTClib)
- **Rotary Encoders**: Fan speed and temperature control (2x encoders)
- **Display**: Custom 7-segment display with icon matrix
- **AC Amplifier**: Factory RX8 AC amplifier unit

### Pin Configuration

See `include/pins.h` for complete pin mapping:

#### Encoder Inputs
- Fan Encoder: Pins 2, 18
- Temperature Encoder: Pins 3, 19

#### LED Outputs
- Hazard Backlight: Pin 9
- Foot Backlight: Pin 12
- Various status LEDs: Pins 22-34

#### SPI Communication
- MOSI: Pin 51
- SCK: Pin 52
- SS: Pin 53

#### Analog Inputs
- Ignition Voltage: A4
- Backlight Positive: A8
- Backlight Negative: A11

#### AC Amplifier Serial
- TX: Pin 16
- RX: Pin 17

---

## Dependencies

### Arduino Libraries (Install via Library Manager)
- **Adafruit RTClib** (v2.1.1+) - Real-time clock support
- **Adafruit BusIO** (v1.14.5+) - I2C/SPI communication
- **Encoder** by Paul Stoffregen (v1.4.2+) - Rotary encoder reading
- **Smoothed** (v1.2+) - Data smoothing for analog inputs

### Custom Libraries (Included in `lib/`)
- `acAmp` - AC amplifier control
- `backlightLed` - Backlight LED management
- `baseMenu` - Menu system foundation
- `buttonPanel` - Button input handling
- `clock` - Time management
- `command_parser` - Command interpretation
- `confMenu` - Configuration menu
- `display` - Display control
- `dsp` - DSP control
- `espComm` - ESP8266 communication
- `logger` - System logging
- `mainMenu` - Main menu interface
- `subVolMenu` - Subwoofer volume menu

---

## Module Structure

```
AC_Display_Module/
├── README.md              # This file
├── docs/                  # Additional documentation
│   └── integration.md     # Integration guide
├── include/               # Header files
│   ├── pins.h            # Pin definitions
│   └── data_types.h      # Common data structures
├── lib/                   # Custom libraries
│   ├── acAmp/
│   ├── backlightLed/
│   ├── baseMenu/
│   ├── buttonPanel/
│   ├── clock/
│   ├── command_parser/
│   ├── confMenu/
│   ├── display/
│   ├── dsp/
│   ├── espComm/
│   ├── logger/
│   ├── mainMenu/
│   └── subVolMenu/
└── src/                   # Main source files
    ├── main.cpp          # Main application code
    └── main.hpp          # Main header file
```

---

## Data Structures

### Button Enumeration
```cpp
enum btn_enum {
    no_button = 0,
    Auto = 1,           // Auto switch
    Mode = 2,           // Mode switch
    AC = 3,             // A/C switch
    frontDemist = 4,    // Front defrost
    rearDemist = 5,     // Rear defrost
    AirSource = 6,      // Recirculation
    Off = 8             // Off switch
};
```

### Button State
```cpp
struct buttonState {
    int fanRotation;           // Fan encoder rotation
    int tempRotation;          // Temp encoder rotation
    btn_enum shortPushButton;  // Short press button
    btn_enum longPushButton;   // Long press button
};
```

### AC Display State
```cpp
struct acShow {
    uint8_t fanSpeed;
    uint8_t tempDigits[3];
    bool displayAmbient;
    bool stateAuto;
    bool stateAc;
    // ... additional states
};
```

---

## Integration with Main RX8 CAN Bus Project

This module can work alongside the main `RX8_CANBUS.ino` code:

1. **Separate Hardware**: Runs on its own Arduino Mega 2560
2. **CAN Bus Communication**: Can share CAN bus data if needed
3. **Power Integration**: Uses same vehicle power and ground
4. **Serial Communication**: Can communicate with main ECU replacement via Serial

### Integration Options

#### Option 1: Standalone Operation
- AC Display Module runs independently
- Controls AC display only
- No communication with main ECU replacement

#### Option 2: Serial Communication
- Connect Serial ports between Arduino Leonardo and Mega
- Share data: temperature, vehicle speed, warnings
- Coordinated display updates

#### Option 3: CAN Bus Integration
- Both modules on same CAN bus
- AC module can read wheel speed, RPM, etc.
- Shared warning light status

---

## Configuration

### Menu System

#### Main Menu
- Default AC control page
- Battery voltage display page
- Motor temperature display page
- Custom function pages can be added

#### Configuration Menu
- 12/24 hour time format
- Display brightness
- Additional settings

#### Subwoofer Volume Menu
- DSP control for rear subwoofer
- Volume adjustment via rotary encoder

### Button Actions

#### Short Press
- **Auto**: Toggle auto mode
- **Mode**: Change vent mode
- **A/C**: Toggle A/C compressor
- **Front Demist**: Enable front defrost
- **Rear Demist**: Enable rear defrost
- **Air Source**: Toggle recirculation
- **Off**: Turn off AC system

#### Long Press
- **Auto**: Toggle ambient temperature display
- **Mode**: Enter configuration menu
- **A/C**: Next menu page
- **Front Demist**: Previous menu page
- **Rear Demist**: Next menu page (alt)
- **Air Source**: Previous menu page (alt)
- **Off**: Enter subwoofer volume menu

---

## Serial Communication Protocol

The module uses Serial (115200 baud) for debugging and can use Serial3 for ESP8266 communication.

### Debug Output
- Button press events
- Menu state changes
- AC amplifier commands
- Battery voltage readings

---

## Supported OBD-II PIDs

The module can integrate with OBD-II data when ESP8266 companion is used:

- Engine load, coolant temperature
- Engine RPM, vehicle speed
- Throttle position, MAF sensor
- Fuel level, odometer
- Control module voltage
- Catalyst temperature

See `supported_pids.txt` for complete list.

---

## Usage Example

```cpp
#include "main.hpp"

// Initialize all subsystems
void setup() {
    logger_init();
    Wire.begin();

    buttons.init();
    time.init();
    ac.init();
    disp.init();

    // Register menu pages
    mainMenu.registerPage(new mainMenuPage());
    activeMenu = &mainMenu;
}

// Main loop
void loop() {
    ac.tick();              // Update AC state
    buttons.tick();         // Read button inputs
    time.tick();            // Update time

    // Process button actions
    if (mainMenu.isActive()) {
        ac.shortButtonPress(buttons.lastTickButtonState.shortPushButton);
        ac.changeRotary(buttons.lastTickButtonState.fanRotation,
                       buttons.lastTickButtonState.tempRotation);
    }

    // Update display
    if (time.t.minuteChange || activeMenu->displayChanged()) {
        disp.setTime(time.t);
        disp.writeToCharDisp(activeMenu->draw());
        disp.sendIcons();
        disp.sendSevenSeg();
    }
}
```

---

## Building and Uploading

### PlatformIO (Recommended)
```bash
cd AC_Display_Module
pio run              # Build
pio run -t upload    # Upload to board
pio device monitor   # Open serial monitor
```

### Arduino IDE
1. Copy `lib/*` folders to Arduino libraries directory
2. Open `src/main.cpp` (rename to .ino if needed)
3. Select Board: Arduino Mega 2560
4. Select correct COM port
5. Upload

---

## Troubleshooting

### Display Not Working
- Check SPI connections (MOSI, SCK, SS)
- Verify pin definitions in `pins.h`
- Check display power supply

### Buttons Not Responding
- Verify button matrix wiring
- Check pull-up/pull-down resistors
- Monitor serial output for button events

### AC Amplifier Not Responding
- Check Serial1 connections (pins 16, 17)
- Verify baud rate matches amplifier
- Monitor serial commands in debug output

### RTC Not Keeping Time
- Check RTC battery
- Verify I2C connections (SDA, SCL)
- Initialize RTC with correct time

---

## Future Enhancements

- [ ] Integration with main CAN bus controller
- [ ] Additional menu customization options
- [ ] Bluetooth audio streaming via ESP8266
- [ ] Data logging to SD card
- [ ] Custom display animations
- [ ] Touchscreen interface support

---

## Credits

**Original Project**: NES-FM/S1-RX8-AC-Display-controller
**Fork Maintainer**: Michael Prowacki
**License**: MIT

This module is designed to complement the main RX8_CANBUS ECU replacement project.

---

## Support

For issues specific to this module:
- Check original repository: https://github.com/NES-FM/S1-RX8-AC-Display-controller
- Fork repository: https://github.com/michaelprowacki/S1-RX8-AC-Display-controller

For integration with main RX8 ECU project, refer to main project documentation.
