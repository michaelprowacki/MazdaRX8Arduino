# Custom Libraries Directory

## Overview

This directory contains custom libraries for the AC Display Controller. These libraries implement the specific hardware interfaces and functionality for the RX8 AC display system.

## Required Libraries

The following custom libraries are referenced by the main code but are not included in this integration to keep the repository lightweight. They can be obtained from the original repository:

**Source Repository**: https://github.com/michaelprowacki/S1-RX8-AC-Display-controller

### Library List

1. **acAmp/** - AC Amplifier Control
   - Communicates with factory AC amplifier
   - Sends commands via serial interface
   - Manages AC system state

2. **backlightLed/** - Backlight LED Management
   - Controls dashboard backlight
   - PWM dimming support
   - Automatic brightness adjustment

3. **baseMenu/** - Menu System Foundation
   - Base class for all menus
   - Common menu navigation
   - Display update handling

4. **buttonPanel/** - Button Input Handler
   - Button matrix scanning
   - Rotary encoder reading
   - Debouncing and long-press detection

5. **clock/** - Real-Time Clock
   - RTC interface (I2C)
   - Time formatting
   - 12/24 hour support

6. **command_parser/** - Command Interpreter
   - Serial command parsing
   - ESP8266 communication protocol
   - Debug command handling

7. **confMenu/** - Configuration Menu
   - Settings interface
   - Persistent configuration
   - Time format, brightness, etc.

8. **display/** - Display Controller
   - 7-segment display driver
   - LED matrix control
   - SPI communication

9. **dsp/** - Digital Signal Processor
   - Audio DSP control
   - Volume management
   - Write protection handling

10. **espComm/** - ESP8266 Communication
    - Serial protocol for ESP8266
    - Data logging bridge
    - Bluetooth/WiFi interface

11. **logger/** - System Logger
    - Debug output formatting
    - Logging levels
    - Timestamp support

12. **mainMenu/** - Main Menu Interface
    - Primary user interface
    - Page registration
    - Navigation handling

13. **subVolMenu/** - Subwoofer Volume Menu
    - Subwoofer level control
    - DSP integration
    - Volume display

## Installation

### Option 1: Clone Original Repository

```bash
# Clone the original repository
git clone https://github.com/michaelprowacki/S1-RX8-AC-Display-controller.git temp_repo

# Copy library files
cp -r temp_repo/rx8_display/lib/* AC_Display_Module/lib/

# Remove temporary clone
rm -rf temp_repo
```

### Option 2: Git Submodule

```bash
# Add as submodule
git submodule add https://github.com/michaelprowacki/S1-RX8-AC-Display-controller.git external/ac_display

# Link libraries
ln -s ../external/ac_display/rx8_display/lib/* lib/
```

### Option 3: Manual Download

1. Visit https://github.com/michaelprowacki/S1-RX8-AC-Display-controller
2. Download repository as ZIP
3. Extract `rx8_display/lib/` contents
4. Copy to `AC_Display_Module/lib/`

## Library Dependencies

Each library may have dependencies on:
- Arduino core libraries
- Standard C/C++ libraries
- Other custom libraries in this directory
- Third-party libraries (install via PlatformIO)

## Compiling Without Libraries

The current `main.cpp` has library references commented out to allow compilation without the full library set. This is useful for:
- Understanding code structure
- Testing basic functionality
- Developing custom implementations

To use full functionality, uncomment the library includes in:
- `src/main.hpp`
- `src/main.cpp`

## Creating Your Own Implementations

You can implement simplified versions of these libraries for testing or custom hardware. Each library should follow this structure:

```
lib/libraryName/
├── libraryName.hpp    # Header file with class declaration
├── libraryName.cpp    # Implementation file
└── examples/          # Optional usage examples
    └── basic.ino
```

Example minimal library:

```cpp
// lib/display/display.hpp
#pragma once
#include <Arduino.h>

class display {
public:
    void init();
    void setTime(timeObj& t);
    void sendSevenSeg();
    void sendIcons();
private:
    // Hardware interface
};
```

## Testing

To test individual libraries:

1. Create a simple test sketch
2. Include only the library to test
3. Call initialization and basic functions
4. Verify expected behavior

## Documentation

For detailed information about each library's API:
- Check header files (.hpp) for class interfaces
- Review source files (.cpp) for implementation details
- See original repository wiki (if available)
- Refer to `docs/integration.md` for usage examples

## License

These libraries are part of the S1-RX8-AC-Display-controller project.

**Original License**: MIT (typically)
**Author**: NES-FM (original), Michael Prowacki (fork maintainer)

---

*Note: This is a placeholder directory. Actual library files must be obtained from the source repository.*
