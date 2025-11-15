# RX8 Arduino Examples

This directory contains working Arduino examples for the RX8 CAN bus decoder library and various modules.

---

## Available Examples

### 1. CAN_Decoder_Example

**Purpose**: Basic example showing how to decode all major RX8 CAN messages

**Hardware**:
- Arduino Uno, Nano, Leonardo, or Mega
- MCP2515 CAN module
- Connection to RX8 CAN bus

**Features**:
- Decodes RPM, Speed, Throttle (0x201)
- Decodes Temperature, Warning Lights (0x420)
- Decodes Wheel Speeds (0x4B1)
- Decodes Steering Angle (0x4BE)
- Decodes Accelerometer (0x075)
- Formatted serial output
- Demonstrates all decoder functions

**Usage**:
1. Open `CAN_Decoder_Example.ino` in Arduino IDE
2. Install `mcp_can` library
3. Copy `lib/RX8_CAN_Messages.h` to sketch folder
4. Wire MCP2515 to Arduino (see code comments)
5. Upload and open Serial Monitor (115200 baud)

**Expected Output**:
```
================================
RX8 CAN Bus Data
================================
--- PCM Status ---
Engine RPM:     3000 RPM
Vehicle Speed:  60 MPH
Throttle Pos:   50 %

--- Engine Status ---
Coolant Temp:   145 (raw) / 90°C / 194°F
Temp Status:    NORMAL
Oil Pressure:   OK

--- Warning Lights ---
No warnings active

--- Wheel Speeds ---
Front Left:     96 kph (60 mph)
Front Right:    97 kph (60 mph)
...
```

---

### 2. OLED_Display_Example

**Purpose**: Display RX8 data on three OLED displays

**Hardware**:
- Arduino Uno or Nano
- MCP2515 CAN module
- 3x SSD1306 OLED displays (128x64, I2C)
- Connection to RX8 CAN bus

**Display Layout**:
- **Display 1**: RPM (large) and Speed (large)
- **Display 2**: Coolant Temperature + bar graph, Throttle + bar graph
- **Display 3**: Warning lights and status indicators

**Features**:
- Real-time data at 10Hz update rate
- Large, easy-to-read text
- Bar graphs for temperature and throttle
- Warning light indicators
- Oil pressure and temp status

**Libraries Required**:
- `mcp_can`
- `Adafruit_GFX`
- `Adafruit_SSD1306`
- `RX8_CAN_Messages` (from `lib/`)

**I2C Addresses**:
- Display 1: 0x3C (default)
- Display 2: 0x3D
- Display 3: 0x3E

**Note**: Most OLED modules default to 0x3C. You may need to modify the I2C address on two displays using solder jumpers, or use an I2C multiplexer.

---

## How to Use These Examples

### Arduino IDE Method

1. **Install Required Libraries**:
   - Open Arduino IDE
   - Go to Sketch → Include Library → Manage Libraries
   - Search for and install: `mcp_can`, `Adafruit GFX`, `Adafruit SSD1306`

2. **Copy RX8_CAN_Messages Library**:
   - Copy `lib/RX8_CAN_Messages.h` to the example sketch folder
   - Or copy to your Arduino `libraries` folder

3. **Open Example**:
   - File → Open → Navigate to example folder
   - Open the `.ino` file

4. **Configure Hardware**:
   - Check pin definitions at top of sketch
   - Modify if your wiring is different

5. **Upload**:
   - Select correct board (Tools → Board)
   - Select correct port (Tools → Port)
   - Click Upload

---

### PlatformIO Method

1. **Create New Project**:
   ```bash
   pio init -b uno
   ```

2. **Add Libraries** to `platformio.ini`:
   ```ini
   [env:uno]
   platform = atmelavr
   board = uno
   framework = arduino
   lib_deps =
       SPI
       adafruit/Adafruit GFX Library
       adafruit/Adafruit SSD1306
       https://github.com/coryjfowler/MCP_CAN_lib.git
   ```

3. **Copy Files**:
   - Copy example `.ino` file to `src/main.cpp`
   - Copy `lib/RX8_CAN_Messages.h` to `lib/RX8_CAN_Messages/RX8_CAN_Messages.h`

4. **Build and Upload**:
   ```bash
   pio run -t upload
   pio device monitor
   ```

---

## Wiring Diagrams

### MCP2515 to Arduino

| MCP2515 Pin | Arduino Pin | Notes |
|-------------|-------------|-------|
| VCC | 5V | Power |
| GND | GND | Ground |
| CS | Pin 10 | Chip select (configurable) |
| SO (MISO) | Pin 12 | SPI |
| SI (MOSI) | Pin 11 | SPI |
| SCK | Pin 13 | SPI |
| INT | Pin 2 | Interrupt (configurable) |

### OLED Display (I2C) to Arduino

| OLED Pin | Arduino Pin | Notes |
|----------|-------------|-------|
| VCC | 5V | Power (some modules are 3.3V only!) |
| GND | GND | Ground |
| SDA | A4 (Uno) / 20 (Mega) | I2C data |
| SCL | A5 (Uno) / 21 (Mega) | I2C clock |

**Multiple I2C Displays**: If using multiple displays, each needs a unique I2C address. Check your OLED module for address configuration jumpers.

---

## Modifying Examples

### Change CAN Bus Speed

If your setup uses different CAN speed:

```cpp
// Change from:
CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ);

// To (for example, 125 kbps):
CAN0.begin(MCP_ANY, CAN_125KBPS, MCP_16MHZ);
```

### Change MCP2515 Crystal Frequency

If your MCP2515 has 8MHz crystal instead of 16MHz:

```cpp
// Change from:
CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ);

// To:
CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ);
```

### Add More CAN Messages

To decode additional messages, add to the switch statement:

```cpp
switch(rxId) {
  case 0x201:
    decoder.decode0x201(rxBuf);
    break;

  // Add new message here:
  case 0x4BE:
    decoder.decode0x4BE(rxBuf);
    float steeringAngle = decoder.steeringAngle.steeringAngle;
    Serial.print("Steering: ");
    Serial.println(steeringAngle);
    break;
}
```

---

## Troubleshooting

### "CAN Init Failed" Error

**Check**:
- Wiring (especially CS and INT pins)
- MCP2515 crystal frequency (8MHz or 16MHz)
- Power supply (stable 5V)
- SPI connections (MOSI, MISO, SCK)

**Debug**:
```cpp
Serial.println("Checking MCP2515...");
if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
  Serial.println("MCP2515 OK");
} else {
  Serial.println("MCP2515 FAILED");
  // Try different crystal frequency
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("MCP2515 OK with 8MHz crystal");
  }
}
```

### No CAN Messages Received

**Check**:
- Connection to vehicle CAN bus (CAN_H and CAN_L)
- CAN bus speed (RX8 uses 500 kbps)
- Vehicle is running (some messages only sent when engine on)
- INT pin connection
- 120Ω termination resistors (at both ends of bus only)

**Debug**:
```cpp
// Check if CAN interrupt is working
if (!digitalRead(CAN_INT)) {
  Serial.println("CAN message available");
} else {
  Serial.println("No CAN message (check wiring)");
}
```

### OLED Display Not Working

**Check**:
- I2C address (scan for devices)
- Power (3.3V vs 5V - check your module specs)
- SDA/SCL connections
- Pull-up resistors (usually built into Arduino)

**I2C Scanner**:
```cpp
#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(115200);

  Serial.println("Scanning I2C bus...");
  for(byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if(Wire.endTransmission() == 0) {
      Serial.print("Device found at 0x");
      Serial.println(addr, HEX);
    }
  }
}

void loop() {}
```

### Values Seem Wrong

**Check**:
- Using correct decoder for message ID
- Buffer length matches (8 bytes for most messages)
- CAN bus speed is correct (500 kbps for RX8)
- ECU/transmitter is encoding correctly

**Verify**:
```cpp
// Print raw CAN data
Serial.print("ID: 0x");
Serial.print(rxId, HEX);
Serial.print(" [");
Serial.print(len);
Serial.print("] ");
for(int i = 0; i < len; i++) {
  if(rxBuf[i] < 0x10) Serial.print("0");
  Serial.print(rxBuf[i], HEX);
  Serial.print(" ");
}
Serial.println();

// Then decode
decoder.decode0x201(rxBuf);
decoder.printAll(Serial);  // Shows both raw and decoded
```

---

## Creating Your Own Examples

### Template

```cpp
#include <mcp_can.h>
#include <SPI.h>
#include "RX8_CAN_Messages.h"

#define CAN_CS 10
#define CAN_INT 2

MCP_CAN CAN0(CAN_CS);
RX8_CAN_Decoder decoder;

void setup() {
  Serial.begin(115200);

  if(CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
    Serial.println("CAN OK");
  }
  CAN0.setMode(MCP_NORMAL);
  pinMode(CAN_INT, INPUT);
}

void loop() {
  long unsigned int rxId;
  unsigned char len = 0;
  unsigned char rxBuf[8];

  if(!digitalRead(CAN_INT)) {
    if(CAN0.readMsgBuf(&rxId, &len, rxBuf) == CAN_OK) {
      // Your code here
    }
  }
}
```

### Ideas for New Examples

- **Data Logger**: Log CAN data to SD card
- **Warning Alarm**: Sound buzzer when warning lights active
- **Shift Light**: LED strip that lights up based on RPM
- **Lap Timer**: GPS + CAN data for track timing
- **Telemetry**: WiFi streaming to PC/phone
- **Custom Gauge Cluster**: E-ink display or TFT screen

---

## Contributing Examples

Have a useful example? Share it!

1. Create example sketch in new folder
2. Test thoroughly on hardware
3. Add comments explaining the code
4. Add to this README
5. Submit pull request

---

## Additional Resources

- **Library Documentation**: See `lib/README.md`
- **CAN Protocol Reference**: See `Documentation/CAN_PID_Reference.md`
- **Integration Guide**: See `Documentation/INTEGRATION_GUIDE.md`
- **Main README**: See root `README.md`

---

*Last Updated: 2025-11-15*
*Repository: https://github.com/michaelprowacki/MazdaRX8Arduino*
