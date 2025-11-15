# ESP8266 Companion Module for AC Display

## Overview

This module extends the AC Display Controller with WiFi and Bluetooth capabilities using an ESP8266 microcontroller.

**Source**: https://github.com/michaelprowacki/S1-RX8-AC-Display-ESP8266-Companion

**Status**: Placeholder - Full source code should be obtained from the source repository

---

## Purpose

The ESP8266 Companion provides:
- **Persistent Data Storage**: Save configuration and settings
- **Bluetooth Communication**: Mobile app connectivity
- **ESP-Now Protocol**: Wireless module-to-module communication
- **Data Logging**: Log vehicle parameters for analysis
- **OBD-II Integration**: Additional diagnostic capabilities

---

## Hardware

**Microcontroller**: ESP8266 (ESP-12E or similar)

**Connection to AC Display Module**:
- **Serial3 on Arduino Mega** (pins 14 TX, 15 RX)
- **Power**: 3.3V from Mega or separate regulator
- **Ground**: Common ground with Mega

---

## Features

### 1. WiFi Connectivity
- Access point mode for configuration
- Station mode for network connectivity
- Web interface for settings
- OTA (Over-The-Air) firmware updates

### 2. Bluetooth Communication
- SPP (Serial Port Profile) for apps
- Transparent serial bridge
- Real-time parameter streaming

### 3. Data Logging
- SD card support (optional)
- Flash memory storage
- Circular buffer for recent data
- Export formats: CSV, JSON

### 4. ESP-Now Protocol
- Low-latency wireless communication
- Module-to-module data sharing
- Range: ~100m (depending on environment)

---

## Obtaining the Source Code

### Method 1: Git Clone (Recommended)

```bash
cd AC_Display_Module/ESP8266_Companion
git clone https://github.com/michaelprowacki/S1-RX8-AC-Display-ESP8266-Companion.git .
```

### Method 2: Git Submodule

```bash
cd MazdaRX8Arduino
git submodule add https://github.com/michaelprowacki/S1-RX8-AC-Display-ESP8266-Companion.git \
  AC_Display_Module/ESP8266_Companion
git submodule update --init --recursive
```

### Method 3: Manual Download

1. Visit: https://github.com/michaelprowacki/S1-RX8-AC-Display-ESP8266-Companion
2. Download ZIP
3. Extract to `AC_Display_Module/ESP8266_Companion/`

---

## Building and Uploading

### PlatformIO (Recommended)

```bash
cd AC_Display_Module/ESP8266_Companion
pio run                    # Build firmware
pio run -t upload          # Upload to ESP8266
pio device monitor         # Serial monitor
```

### Arduino IDE

1. Install ESP8266 board support
2. Install required libraries
3. Open main.cpp (or .ino file)
4. Select Board: ESP8266 Generic Module
5. Upload

---

## Wiring Diagram

```
Arduino Mega 2560                ESP8266
(AC Display Module)
-------------------             ----------
    TX3 (Pin 14) ------>-------- RX
    RX3 (Pin 15) -------<------- TX
    3.3V ----------------------- VCC (or use level shifter)
    GND ------------------------ GND


Notes:
- ESP8266 operates at 3.3V logic level
- Arduino Mega TX is 5V - use level shifter or voltage divider
- Or power ESP8266 from separate 3.3V regulator
```

**Level Shifter Circuit** (if needed):
```
Mega TX3 (5V) ----[1kΩ]---+--- ESP RX (3.3V)
                           |
                        [2kΩ]
                           |
                          GND

ESP TX (3.3V) -------------- Mega RX3 (5V tolerant)
```

---

## Communication Protocol

### Serial Configuration
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1

### Message Format

```
Command Format: $CMD:VALUE\n

Examples:
$RPM:3500\n          // Engine RPM
$SPD:65\n            // Vehicle speed (mph)
$TMP:145\n           // Engine temperature
$BAT:13.8\n          // Battery voltage
$MENU:2\n            // Current menu page
$ACK\n               // Acknowledgment
$REQ:RPM\n           // Request parameter
```

### From AC Display → ESP8266
- Vehicle parameters (RPM, speed, temp)
- Menu states
- Button presses
- Configuration changes

### From ESP8266 → AC Display
- Commands (change menu, set value)
- Configuration updates
- Acknowledgments
- Status requests

---

## Configuration

### WiFi Setup

1. On first boot, ESP8266 creates AP: `RX8_AC_Display`
2. Connect to AP with password: `rx8display`
3. Navigate to: http://192.168.4.1
4. Configure WiFi credentials
5. ESP8266 reboots and connects to network

### Bluetooth Setup

1. ESP8266 advertises as: `RX8-AC-Display`
2. Pair with PIN: `1234` (default)
3. Connect using serial terminal app
4. Send commands or receive data stream

---

## Python Upload Script

The repository includes `platformio_upload.py` for automated uploading:

```bash
python platformio_upload.py --port /dev/ttyUSB0
```

Script features:
- Auto-detects ESP8266
- Configures upload speed
- Monitors upload progress
- Verifies successful upload

---

## Integration with AC Display Module

### Update main.cpp

```cpp
// In AC_Display_Module/src/main.cpp

void loop() {
    // ... existing code ...

    // Send data to ESP8266 every 500ms
    static unsigned long lastEspUpdate = 0;
    if (millis() - lastEspUpdate >= 500) {
        Serial3.print("$RPM:");
        Serial3.println(engineRPM);

        Serial3.print("$SPD:");
        Serial3.println(vehicleSpeed);

        Serial3.print("$BAT:");
        Serial3.println(getBatVolt(), 2);

        lastEspUpdate = millis();
    }

    // Read commands from ESP8266
    if (Serial3.available()) {
        String cmd = Serial3.readStringUntil('\n');
        processEspCommand(cmd);
    }
}

void processEspCommand(String cmd) {
    if (cmd.startsWith("$MENU:")) {
        int page = cmd.substring(6).toInt();
        // Change to menu page
    }
    // ... handle other commands ...
}
```

---

## Mobile App Integration

### Recommended Apps

1. **Serial Bluetooth Terminal** (Android/iOS)
   - Real-time parameter display
   - Command sending
   - Data logging

2. **Custom App Development**
   - Use ESP8266 as web server
   - HTTP API for parameter access
   - WebSocket for real-time updates

### Example HTTP API

```
GET /api/rpm           → {"rpm": 3500}
GET /api/speed         → {"speed": 65}
GET /api/temp          → {"temperature": 145}
GET /api/all           → {...all parameters...}
POST /api/menu?page=2  → Change menu page
```

---

## Data Logging Format

### CSV Example
```csv
timestamp,rpm,speed,temp,voltage
1699564800,3500,65,145,13.8
1699564801,3550,66,145,13.7
```

### JSON Example
```json
{
  "timestamp": 1699564800,
  "parameters": {
    "rpm": 3500,
    "speed": 65,
    "temperature": 145,
    "voltage": 13.8
  }
}
```

---

## Troubleshooting

### ESP8266 Not Responding
- Check power supply (3.3V, sufficient current)
- Verify TX/RX connections (crossover)
- Check baud rate (115200)
- Test with AT commands

### WiFi Connection Issues
- Reset WiFi credentials (hold button on boot)
- Check SSID/password
- Verify network is 2.4GHz (ESP8266 doesn't support 5GHz)

### Upload Failures
- Check USB cable
- Install CH340/CP2102 drivers
- Put ESP8266 in flash mode (GPIO0 to GND on boot)
- Try slower upload speed

---

## Future Enhancements

Potential features to add:
- [ ] MQTT support for IoT platforms
- [ ] SD card data logging
- [ ] GPS module integration
- [ ] CAN bus pass-through mode
- [ ] Cloud data sync
- [ ] Firmware version checking
- [ ] Remote configuration

---

## Credits

**Original Project**: NES-FM/S1-RX8-AC-Display-ESP8266-Companion
**Fork Maintainer**: Michael Prowacki
**License**: MIT

---

## Support

For ESP8266 Companion specific issues:
- Original: https://github.com/NES-FM/S1-RX8-AC-Display-ESP8266-Companion
- Fork: https://github.com/michaelprowacki/S1-RX8-AC-Display-ESP8266-Companion

For integration with AC Display Module:
- See main project documentation
- Check `AC_Display_Module/docs/integration.md`

---

*This is a placeholder README. Full source code must be obtained from the repository.*
