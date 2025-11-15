# RX8 CAN Message Library

## Overview

This directory contains shared libraries for working with Mazda RX8 CAN bus messages.

### RX8_CAN_Messages.h

Comprehensive CAN message decoder/encoder library for all documented RX8 CAN messages.

**Features**:
- ✅ Decode all major CAN messages (0x201, 0x420, 0x4B0, 0x4B1, 0x4BE, 0x075, etc.)
- ✅ Encode messages for ECU/transmitting modules
- ✅ Structured data types for easy access
- ✅ Helper functions (unit conversions, validation)
- ✅ Debug output functionality
- ✅ Zero external dependencies (uses only Arduino.h)

---

## Installation

### Arduino IDE

1. Copy `lib/RX8_CAN_Messages.h` to your sketch folder
2. Include in your sketch:
   ```cpp
   #include "RX8_CAN_Messages.h"
   ```

### PlatformIO

1. Copy `lib/RX8_CAN_Messages.h` to `lib/` directory in your project
2. Include in your code:
   ```cpp
   #include <RX8_CAN_Messages.h>
   ```

---

## Quick Start

### Basic Usage (Receiving Module)

```cpp
#include <mcp_can.h>
#include "RX8_CAN_Messages.h"

#define CAN_CS 10
MCP_CAN CAN0(CAN_CS);
RX8_CAN_Decoder decoder;

void setup() {
  Serial.begin(115200);
  CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ);
  CAN0.setMode(MCP_NORMAL);
}

void loop() {
  long unsigned int rxId;
  unsigned char len = 0;
  unsigned char rxBuf[8];

  if(CAN0.readMsgBuf(&rxId, &len, rxBuf) == CAN_OK) {
    switch(rxId) {
      case 0x201:
        decoder.decode0x201(rxBuf);
        Serial.print("RPM: ");
        Serial.println(decoder.pcmStatus.engineRPM);
        break;

      case 0x420:
        decoder.decode0x420(rxBuf);
        Serial.print("Coolant Temp: ");
        Serial.println(decoder.warningLights.coolantTemperature);
        break;

      case 0x4B1:
        decoder.decode0x4B1(rxBuf);
        Serial.print("Speed: ");
        Serial.println(decoder.wheelSpeeds.averageFront / 100);
        break;
    }
  }
}
```

### Advanced Usage (Multiple Messages)

```cpp
#include <mcp_can.h>
#include "RX8_CAN_Messages.h"

MCP_CAN CAN0(10);
RX8_CAN_Decoder decoder;

void loop() {
  readAllCANMessages();

  // Access all decoded data
  int rpm = decoder.pcmStatus.engineRPM;
  int speed = decoder.pcmStatus.vehicleSpeed;
  int throttle = decoder.pcmStatus.throttlePosition;
  int coolantTemp = decoder.warningLights.coolantTemperature;
  float steeringAngle = decoder.steeringAngle.steeringAngle;

  // Check warnings
  if(decoder.hasActiveWarnings()) {
    Serial.println("WARNING ACTIVE!");
  }

  // Print all data
  decoder.printAll(Serial);
}

void readAllCANMessages() {
  long unsigned int rxId;
  unsigned char len = 0;
  unsigned char rxBuf[8];

  while(CAN0.readMsgBuf(&rxId, &len, rxBuf) == CAN_OK) {
    switch(rxId) {
      case 0x201: decoder.decode0x201(rxBuf); break;
      case 0x420: decoder.decode0x420(rxBuf); break;
      case 0x4B0: decoder.decode0x4B0(rxBuf); break;
      case 0x4B1: decoder.decode0x4B1(rxBuf); break;
      case 0x4BE: decoder.decode0x4BE(rxBuf); break;
      case 0x075: decoder.decode0x075(rxBuf); break;
      case 0x620: decoder.decode0x620(rxBuf); break;
      case 0x630: decoder.decode0x630(rxBuf); break;
      case 0x212: decoder.decode0x212(rxBuf); break;
    }
  }
}
```

### Encoding (Transmitting Module)

```cpp
#include "RX8_CAN_Messages.h"

void sendPCMStatus() {
  uint8_t msg[8];

  // Encode RPM=3000, Speed=60mph, Throttle=50%
  RX8_CAN_Encoder::encode0x201(msg, 3000, 60, 50);
  CAN0.sendMsgBuf(0x201, 0, 8, msg);
}

void sendWarningLights() {
  uint8_t msg[7];

  // Encode Temp=145, CEL=false, LowCoolant=false, Battery=false, Oil=false
  RX8_CAN_Encoder::encode0x420(msg, 145, false, false, false, false);
  CAN0.sendMsgBuf(0x420, 0, 7, msg);
}
```

---

## API Reference

### RX8_CAN_Decoder Class

#### Data Structures (Public Members)

```cpp
PCM_Status pcmStatus;              // 0x201 data
Warning_Lights warningLights;      // 0x420 data
Wheel_Speeds wheelSpeeds;          // 0x4B0/0x4B1 data
Steering_Angle steeringAngle;      // 0x4BE data
Accelerometer accelerometer;       // 0x075 data
ABS_Data absData;                  // 0x620 data
ABS_Config absConfig;              // 0x630 data
DSC_Status dscStatus;              // 0x212 data
```

#### Decoding Methods

```cpp
void decode0x201(uint8_t buf[8]);  // PCM Status (RPM, Speed, Throttle)
void decode0x420(uint8_t buf[7]);  // Warning Lights and Temperature
void decode0x4B0(uint8_t buf[8]);  // Wheel Speeds (ABS/DSC)
void decode0x4B1(uint8_t buf[8]);  // Wheel Speeds (Dashboard)
void decode0x4BE(uint8_t buf[8]);  // Steering Angle
void decode0x075(uint8_t buf[8]);  // Accelerometer (3-axis)
void decode0x620(uint8_t buf[7]);  // ABS System Data
void decode0x630(uint8_t buf[8]);  // ABS Configuration
void decode0x212(uint8_t buf[7]);  // DSC/ABS Status
```

#### Helper Methods

```cpp
int speedMPHtoKPH(int mph);        // Convert MPH to KPH
int speedKPHtoMPH(int kph);        // Convert KPH to MPH
int tempToCelsius(int rawTemp);    // Convert raw temp to Celsius
int tempToFahrenheit(int rawTemp); // Convert raw temp to Fahrenheit
bool isCoolantTempNormal();        // Check if coolant temp normal
bool hasActiveWarnings();          // Check for any active warnings
int getWheelSpeedMPH(int encoded); // Convert wheel speed to MPH
void printAll(Stream &s = Serial); // Print all decoded values
void reset();                      // Reset all values to defaults
```

### RX8_CAN_Encoder Class

#### Encoding Methods (Static)

```cpp
static void encode0x201(uint8_t buf[], int rpm, int speed, int throttle);
static void encode0x420(uint8_t buf[], int temp, bool cel, bool lowCoolant,
                        bool batteryCharge, bool oilPressure);
static void encodeWheelSpeeds(uint8_t buf[], int fl, int fr, int rl, int rr);
```

---

## Data Structures

### PCM_Status (0x201)

```cpp
struct PCM_Status {
    int engineRPM;           // Engine RPM (0-10000+)
    int vehicleSpeed;        // Vehicle speed in MPH
    int throttlePosition;    // Throttle position (0-100%)
    uint16_t rawRPM;         // Raw encoded value
    uint16_t rawSpeed;       // Raw encoded value
    uint8_t rawThrottle;     // Raw encoded value
};

// Access example:
int rpm = decoder.pcmStatus.engineRPM;
int speed = decoder.pcmStatus.vehicleSpeed;
int throttle = decoder.pcmStatus.throttlePosition;
```

### Warning_Lights (0x420)

```cpp
struct Warning_Lights {
    int coolantTemperature;  // Coolant temp (145 = normal)
    bool checkEngineMIL;     // Check Engine light
    bool checkEngineBL;      // Check Engine backlight
    bool lowCoolantMIL;      // Low coolant warning
    bool batteryChargeMIL;   // Battery charge warning
    bool oilPressureMIL;     // Oil pressure warning
    bool oilPressureOK;      // Oil pressure OK (1 = OK)
    bool catalystTempHigh;   // Catalyst over-temperature
    bool engineOverheat;     // Engine overheat warning
    uint8_t odometerByte;    // Odometer-related byte
    uint8_t byte5;           // Raw byte 5
    uint8_t byte6;           // Raw byte 6
};

// Access example:
if(decoder.warningLights.checkEngineMIL) {
    Serial.println("Check Engine Light is ON!");
}
```

### Wheel_Speeds (0x4B0/0x4B1)

```cpp
struct Wheel_Speeds {
    int frontLeft;           // Front left (kph * 100)
    int frontRight;          // Front right (kph * 100)
    int rearLeft;            // Rear left (kph * 100)
    int rearRight;           // Rear right (kph * 100)
    int averageFront;        // Average of front wheels
    int averageRear;         // Average of rear wheels
    int averageAll;          // Average of all wheels
    bool wheelSpeedMismatch; // Front wheels differ >5 kph
};

// Access example:
int speedKPH = decoder.wheelSpeeds.averageFront / 100;
int speedMPH = decoder.getWheelSpeedMPH(decoder.wheelSpeeds.averageFront);
```

### Steering_Angle (0x4BE)

```cpp
struct Steering_Angle {
    float steeringAngle;     // Steering angle in degrees (-780 to +780)
    int16_t rawAngle;        // Raw 16-bit signed value
    bool centerCalibrated;   // True if steering centered (±5°)
};

// Access example:
float angle = decoder.steeringAngle.steeringAngle;
Serial.print("Steering: ");
Serial.print(angle);
Serial.println("°");
```

### Accelerometer (0x075)

```cpp
struct Accelerometer {
    float lateralG;          // Lateral G-force (left/right)
    float longitudinalG;     // Longitudinal G-force (forward/back)
    float verticalG;         // Vertical G-force (up/down)
    int16_t rawLateral;      // Raw values
    int16_t rawLongitudinal;
    int16_t rawVertical;
};

// Access example:
float corneringG = decoder.accelerometer.lateralG;
float brakingG = decoder.accelerometer.longitudinalG;
```

---

## Examples

### Example 1: Simple Display Module

```cpp
#include <mcp_can.h>
#include <Adafruit_SSD1306.h>
#include "RX8_CAN_Messages.h"

MCP_CAN CAN0(10);
RX8_CAN_Decoder decoder;
Adafruit_SSD1306 display(128, 64, &Wire, -1);

void setup() {
  CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ);
  CAN0.setMode(MCP_NORMAL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
}

void loop() {
  updateCANData();
  updateDisplay();
  delay(100);
}

void updateCANData() {
  long unsigned int rxId;
  unsigned char len, rxBuf[8];

  while(CAN0.readMsgBuf(&rxId, &len, rxBuf) == CAN_OK) {
    if(rxId == 0x201) decoder.decode0x201(rxBuf);
    if(rxId == 0x420) decoder.decode0x420(rxBuf);
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.print("RPM:");
  display.println(decoder.pcmStatus.engineRPM);

  display.setCursor(0, 20);
  display.print("MPH:");
  display.println(decoder.pcmStatus.vehicleSpeed);

  display.setCursor(0, 40);
  display.print("TEMP:");
  display.println(decoder.warningLights.coolantTemperature);

  display.display();
}
```

### Example 2: Data Logger

```cpp
#include <SD.h>
#include <mcp_can.h>
#include "RX8_CAN_Messages.h"

MCP_CAN CAN0(10);
RX8_CAN_Decoder decoder;
File dataFile;

void setup() {
  Serial.begin(115200);
  SD.begin(4);
  CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ);
  CAN0.setMode(MCP_NORMAL);

  dataFile = SD.open("rx8_log.csv", FILE_WRITE);
  dataFile.println("Time,RPM,Speed,Throttle,Temp,SteeringAngle");
}

void loop() {
  updateCANData();
  logData();
  delay(100);  // Log at 10Hz
}

void updateCANData() {
  long unsigned int rxId;
  unsigned char len, rxBuf[8];

  while(CAN0.readMsgBuf(&rxId, &len, rxBuf) == CAN_OK) {
    switch(rxId) {
      case 0x201: decoder.decode0x201(rxBuf); break;
      case 0x420: decoder.decode0x420(rxBuf); break;
      case 0x4BE: decoder.decode0x4BE(rxBuf); break;
    }
  }
}

void logData() {
  dataFile.print(millis());
  dataFile.print(",");
  dataFile.print(decoder.pcmStatus.engineRPM);
  dataFile.print(",");
  dataFile.print(decoder.pcmStatus.vehicleSpeed);
  dataFile.print(",");
  dataFile.print(decoder.pcmStatus.throttlePosition);
  dataFile.print(",");
  dataFile.print(decoder.warningLights.coolantTemperature);
  dataFile.print(",");
  dataFile.println(decoder.steeringAngle.steeringAngle);
  dataFile.flush();
}
```

### Example 3: Warning Light Monitor

```cpp
#include <mcp_can.h>
#include "RX8_CAN_Messages.h"

MCP_CAN CAN0(10);
RX8_CAN_Decoder decoder;

#define LED_CEL 7
#define LED_COOLANT 8
#define LED_OIL 9

void setup() {
  Serial.begin(115200);
  CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ);
  CAN0.setMode(MCP_NORMAL);

  pinMode(LED_CEL, OUTPUT);
  pinMode(LED_COOLANT, OUTPUT);
  pinMode(LED_OIL, OUTPUT);
}

void loop() {
  updateCANData();
  updateWarningLEDs();

  if(decoder.hasActiveWarnings()) {
    Serial.println("WARNINGS ACTIVE:");
    decoder.printAll(Serial);
  }

  delay(100);
}

void updateCANData() {
  long unsigned int rxId;
  unsigned char len, rxBuf[8];

  if(CAN0.readMsgBuf(&rxId, &len, rxBuf) == CAN_OK) {
    if(rxId == 0x420) {
      decoder.decode0x420(rxBuf);
    }
  }
}

void updateWarningLEDs() {
  digitalWrite(LED_CEL, decoder.warningLights.checkEngineMIL);
  digitalWrite(LED_COOLANT, decoder.warningLights.lowCoolantMIL);
  digitalWrite(LED_OIL, decoder.warningLights.oilPressureMIL);
}
```

---

## Message Reference

### Commonly Used Messages

| CAN ID | Name | Update Rate | Data |
|--------|------|-------------|------|
| 0x201 | PCM Status | 100ms | RPM, Speed, Throttle |
| 0x420 | Warning Lights | 100ms | Temp, MIL, Warnings |
| 0x4B1 | Wheel Speeds | 10ms | FL, FR, RL, RR speeds |
| 0x4BE | Steering Angle | 20ms | Steering wheel angle |
| 0x075 | Accelerometer | 20ms | 3-axis G-forces |

### All Supported Messages

See `RX8_CAN_Messages.h` header for complete list of CAN message IDs and structures.

For detailed protocol documentation, see:
- `Documentation/CAN_PID_Reference.md` - Complete protocol reference
- `Documentation/rx8_can_database.dbc` - DBC file for analysis tools

---

## Notes

### Accuracy

- RPM decoding is accurate (tested against factory tachometer)
- Speed requires calibration (varies by wheel/tire size)
- Temperature values are approximate (145 = ~90°C)
- Accelerometer values may need calibration

### Compatibility

- Tested on Series 1 RX8 (2004-2008)
- Series 2 RX8 (2009+) uses different protocol (not supported)
- Some values may vary between RX8 models (AT/MT, region, year)

### Performance

- Decoding is fast (~10-20 microseconds per message)
- Safe to call in real-time CAN receive loop
- No dynamic memory allocation
- Minimal stack usage

---

## Troubleshooting

### Problem: All values are zero

**Check:**
- CAN bus initialized correctly (500 kbps)
- Messages are being received (`readMsgBuf` returns OK)
- Correct CAN ID being decoded

**Debug:**
```cpp
// Add before decoding
Serial.print("Received ID: 0x");
Serial.print(rxId, HEX);
Serial.print(" Data: ");
for(int i = 0; i < len; i++) {
  Serial.print(rxBuf[i], HEX);
  Serial.print(" ");
}
Serial.println();
```

### Problem: Values seem incorrect

**Check:**
- Using correct decoder function for CAN ID
- Buffer length matches expected (8 bytes for most messages)
- Raw values vs decoded values (use `printAll()` to see both)

**Verify:**
```cpp
decoder.decode0x201(rxBuf);
decoder.printAll(Serial);  // Shows raw and decoded values
```

---

## Contributing

Found a bug or have improvements?

1. Test your changes thoroughly
2. Document the change
3. Update examples if needed
4. Submit pull request to repository

---

## License

MIT License - See repository LICENSE file

## Attribution

Based on reverse engineering work by:
- rnd-ash/rx8-reverse-engineering
- topolittle/RX8-CAN-BUS
- RX8Club community
- Extensive vehicle testing and validation

---

*Last Updated: 2025-11-15*
*Repository: https://github.com/michaelprowacki/MazdaRX8Arduino*
