# Power Distribution Module (PDM) Integration Guide

**Last Updated**: 2025-11-17
**Status**: Design & Planning Phase
**Complexity**: Advanced (Hardware + Software)
**Estimated Effort**: 20-40 hours

---

## Table of Contents

1. [What is a PDM?](#what-is-a-pdm)
2. [Why Add PDM to RX8 ECU?](#why-add-pdm-to-rx8-ecu)
3. [Implementation Options](#implementation-options)
4. [Recommended Approach](#recommended-approach)
5. [Hardware Design](#hardware-design)
6. [Software Integration](#software-integration)
7. [Safety & Testing](#safety--testing)

---

## What is a PDM?

A **Power Distribution Module** is an electronic replacement for traditional fuses, relays, and switches. Instead of mechanical relays, it uses solid-state switches (MOSFETs) to control electrical loads.

### Traditional Setup (Factory RX8)
```
Battery → Fuse Box → Mechanical Relays → Device (Fuel Pump, Fan, etc.)
                      ↓
                  Manual switches or PCM control
```

### Modern PDM Setup
```
Battery → PDM (Electronic Switching) → Device
           ↑
        ECU Control (CAN bus or digital signals)
```

### Key Features

| Feature | Traditional | PDM |
|---------|-------------|-----|
| **Switching** | Mechanical relays | Solid-state (MOSFETs) |
| **Control** | Manual or basic | Programmable logic |
| **Current Limiting** | Fuses (one-time) | Electronic (resetable) |
| **Fault Detection** | None | Real-time monitoring |
| **Wiring** | Complex | Simplified |
| **Weight** | Heavy (relays) | Light (electronics) |

---

## Why Add PDM to RX8 ECU?

### Use Cases

#### 1. **Engine Swap** ⭐ **Most Common**
When swapping engines (e.g., LS1, 2JZ, EV conversion), you need to control:
- Fuel pump (high current, ~15A)
- Radiator fans (2x fans, ~20A each)
- Coolant pump (EV conversions)
- Throttle body heater
- A/C compressor clutch
- Engine bay lighting

**Problem**: Factory RX8 wiring is for rotary engine. Swapped engine has different needs.

**Solution**: PDM provides flexible, reprogrammable power control.

---

#### 2. **Race Car Setup** 🏁
For track/race use, you want:
- Data logging (current draw monitoring)
- Kill switches (master power cut)
- Fault detection (prevent fires)
- Weight reduction (remove heavy relays)
- Simplified wiring harness

---

#### 3. **EV Conversion** ⚡
Electric RX8 conversions need to control:
- Battery management system (BMS)
- Motor controller power
- DC-DC converter
- Coolant pumps (battery + motor)
- Charging system
- High-voltage contactors (safety)

**Our EV_ECU Module** (already in repo) would benefit from PDM integration!

---

#### 4. **Advanced Features**
- **Progressive fan control**: Low speed → medium → high based on coolant temp
- **Soft-start**: Ramp up high-current devices (prevent voltage sag)
- **Load shedding**: Turn off non-critical loads if alternator fails
- **Remote diagnostics**: Monitor current draw, detect shorts, log faults

---

## Implementation Options

### Option 1: Commercial PDM (Easiest)

**Buy a ready-made PDM** from manufacturers like:
- **Haltech PDM**: ~$800-1,500 (8-16 channels)
- **MoTeC PDM**: ~$1,500-3,000 (professional grade)
- **Motec Infineon PDM60**: ~$1,200 (8 channels, 60A total)
- **AEM PDU**: ~$700 (6 channels)

**Pros**:
- ✅ Plug-and-play
- ✅ Professional-grade reliability
- ✅ Built-in safety features
- ✅ CAN bus support (500 kbps)
- ✅ Software configuration tools

**Cons**:
- ❌ Expensive ($700-3,000)
- ❌ Overkill for simple swaps
- ❌ May need specific CAN protocol
- ❌ Not DIY-friendly

**Integration**:
```cpp
// In RX8_CANBUS.ino, add PDM CAN messages:
void sendPDMControl() {
  byte pdm_msg[8];
  pdm_msg[0] = fuelPumpEnable ? 0x01 : 0x00;  // Channel 1: Fuel pump
  pdm_msg[1] = fan1Enable ? 0x01 : 0x00;      // Channel 2: Fan 1
  pdm_msg[2] = fan2Enable ? 0x01 : 0x00;      // Channel 3: Fan 2
  // ... etc.

  CAN0.sendMsgBuf(0x600, 0, 8, pdm_msg);  // PDM CAN ID (check PDM manual)
}
```

**Best For**: Professional builds, race cars, high-budget projects

---

### Option 2: DIY Arduino PDM (Budget-Friendly)

**Build your own** using:
- **Arduino Mega** (more I/O pins than Leonardo)
- **MOSFET modules** (30A each, ~$5-10)
- **Current sensors** (ACS712, ~$3 each)
- **Fused outputs** (backup protection)

**Pros**:
- ✅ Low cost (~$100-200 for 8 channels)
- ✅ Fully customizable
- ✅ Learning experience
- ✅ Easy CAN bus integration (same MCP2515)

**Cons**:
- ❌ DIY = more failure risk
- ❌ Requires electrical engineering knowledge
- ❌ Need proper testing and safety measures
- ❌ PCB design recommended (not breadboard)

**Best For**: DIY enthusiasts, budget builds, learning projects

---

### Option 3: Expand Existing ECU (Integrated Approach)

**Add PDM functionality** to our Leonardo ECU:
- Use **existing Arduino Leonardo**
- Add **MOSFET shield** or external relay board
- Control via **digital output pins**
- Monitor current via **analog pins**

**Pros**:
- ✅ Single integrated unit (ECU + PDM)
- ✅ No additional CAN messages needed
- ✅ Minimal wiring
- ✅ Lowest cost (~$50-100)

**Cons**:
- ❌ Limited I/O pins (Leonardo has ~20 digital pins)
- ❌ Shared resources (CAN, serial, etc.)
- ❌ Harder to troubleshoot (one device = single point of failure)
- ❌ Not modular

**Best For**: Simple swaps (3-5 devices), integrated solutions

---

### Option 4: Separate Arduino PDM Module (Recommended) ⭐

**Create a dedicated PDM** as a separate module:
- **Arduino Mega 2560** (54 digital I/O pins)
- **MCP2515 CAN module** (for ECU communication)
- **MOSFET array** (8-16 channels)
- **Current sensors** (per-channel monitoring)
- **Enclosure** (waterproof for engine bay)

**Pros**:
- ✅ Modular (ECU + PDM separate)
- ✅ Many I/O pins (Mega has 54)
- ✅ Easy troubleshooting (swap modules)
- ✅ Scalable (add more channels)
- ✅ CAN bus integration (coordinate with ECU)

**Cons**:
- ⚠️ Medium complexity (need CAN protocol)
- ⚠️ Two Arduinos (but isolated)
- ⚠️ Moderate cost (~$150-300)

**Best For**: Most engine swaps, modular builds, future expansion

---

## Recommended Approach

### For RX8 Project: **Option 4 (Separate Arduino Mega PDM)**

**Rationale**:
1. **Modular**: Keeps ECU and PDM separate (easier debugging)
2. **Scalable**: Mega has 54 I/O pins (plenty of room)
3. **CAN Integration**: Coordinate with existing RX8 ECU
4. **Cost-Effective**: ~$200 total (vs. $800+ commercial)
5. **Community**: Can be shared as another module in repo

---

## Hardware Design

### PDM Block Diagram

```
                          ┌─────────────────────────────┐
                          │   Arduino Mega 2560 PDM     │
                          │                             │
    12V Battery ──────────┤ VIN (12V input)             │
                          │                             │
    CAN Bus ──────────────┤ MCP2515 (CAN interface)     │
    (from ECU)            │   - CANH / CANL             │
                          │   - CS Pin 53               │
                          │   - INT Pin 2               │
                          │                             │
    Current Sensors ──────┤ Analog Inputs (A0-A15)      │
    (ACS712 30A)          │   - Per-channel monitoring  │
                          │                             │
    MOSFET Outputs ───────┤ Digital Outputs (22-53)     │
    (IRLB8721)            │   - 8-16 channels           │
                          │   - PWM capable (fans)      │
                          │                             │
    Status LEDs ──────────┤ Digital Outputs (3-13)      │
                          │   - Per-channel indicators  │
                          │                             │
    Fused Outputs ────────┤ → Fuel Pump (15A)           │
                          │ → Fan 1 (20A)               │
                          │ → Fan 2 (20A)               │
                          │ → Water Pump (10A)          │
                          │ → A/C Clutch (5A)           │
                          │ → Aux 1-3 (10A each)        │
                          │                             │
                          └─────────────────────────────┘
```

---

### Component Selection

#### 1. **Arduino Mega 2560**
- **Why**: 54 digital I/O pins (vs. 20 on Leonardo)
- **Cost**: ~$15-20 (clone) or $40 (genuine)
- **Flash**: 256 KB (vs. 32 KB Leonardo)
- **RAM**: 8 KB (vs. 2.5 KB Leonardo)

#### 2. **MOSFET Modules (High-Current Switching)**

**Recommended**: **IRLB8721** N-Channel MOSFETs
- **Rating**: 62A continuous, 100A peak
- **Rds(on)**: 5.5 mΩ (very low resistance = less heat)
- **Logic-level**: 5V gate drive (Arduino compatible)
- **Cost**: ~$1-2 each

**Alternative**: Pre-made MOSFET modules
- **RobotDyn 30A MOSFET**: ~$5-10 (includes screw terminals, heatsink)
- **Pololu 30A MOSFET**: ~$8 (professional quality)

**Wiring**:
```
Arduino Pin 22 ──> MOSFET Gate (control)
Battery 12V ────> MOSFET Drain (input)
Fuel Pump ──────> MOSFET Source (output, switched ground)
```

**Heat Dissipation**:
- Power = I² × Rds(on)
- Example: 20A @ 5.5 mΩ = 20² × 0.0055 = 2.2W
- **Need heatsink** for loads >10A

---

#### 3. **Current Sensors (Per-Channel Monitoring)**

**Recommended**: **ACS712 Hall-Effect Sensors**
- **Ratings**: 5A, 20A, or 30A versions
- **Output**: Analog voltage (2.5V = 0A, ±66 mV/A)
- **Cost**: ~$3-5 each
- **Accuracy**: ±1.5%

**Wiring**:
```
ACS712 VCC ──> 5V (Arduino)
ACS712 GND ──> GND
ACS712 OUT ──> A0 (analog input)
ACS712 IP+ ──> Fuel pump positive wire
ACS712 IP- ──> Fuel pump (to ground)
```

**Reading Current**:
```cpp
// Read current from ACS712 (30A version)
int sensorValue = analogRead(A0);
float voltage = sensorValue * (5.0 / 1023.0);
float current = (voltage - 2.5) / 0.066;  // 66 mV/A for 30A sensor
```

---

#### 4. **Inline Fuses (Backup Protection)**

Even with electronic current limiting, **always use fuses**:
- **Blade fuses**: ATO/ATC automotive style
- **Rating**: 125% of max expected current
- **Placement**: Between battery and PDM input

**Example**:
- Fuel pump (15A) → Use 20A fuse
- Fan (20A) → Use 25A fuse
- Main PDM input (100A total) → Use 125A fuse

---

#### 5. **Power Input (Battery Connection)**

**Main Power Bus**:
- **Wire gauge**: 10 AWG (30A) or 8 AWG (50A) or 6 AWG (100A)
- **Connector**: Anderson Powerpole or ring terminals
- **Fuse**: Main inline fuse (100-125A)
- **Switch**: Master power switch (optional, for safety)

**Ground**:
- **Heavy ground wire**: Same gauge as power wire
- **Direct to chassis**: Shortest path to battery negative
- **Star grounding**: All grounds to single point (reduce noise)

---

### PDM Circuit Schematic (Single Channel)

```
                    Main Fuse
    Battery 12V ────[100A]─────┬─────────────> To other channels
                               │
                               │ Inline Fuse
                               └───[20A]────┬──> To Load (Fan)
                                            │
                         ┌──────────────────┤
                         │  ACS712 Current  │
                         │     Sensor       │
                         └──────────────────┤
                                            │
                         ┌──────────────────┴───┐
                         │   IRLB8721 MOSFET    │
                         │   Drain ──> Source   │
                         │   Gate <── Pin 22    │──> Arduino Mega
                         └──────────────────────┘
                                            │
                                           GND
```

---

### Enclosure & Mounting

**Requirements**:
- **Waterproof**: IP65+ rated (engine bay moisture)
- **Heat dissipation**: Vented or with heatsink
- **Vibration resistant**: Automotive-grade mounting
- **Size**: 6" x 4" x 2" (approximate, for 8-channel PDM)

**Recommended Enclosures**:
- **Hammond 1591** series (polycarbonate, IP65)
- **Polycase WC** series (waterproof, clear lid)
- **Custom 3D printed** (PLA/PETG + silicone gasket)

**Mounting**:
- **Location**: Engine bay (near ECU) or under dash (safer)
- **Bolts**: M5 or M6 with lock washers
- **Cable glands**: Waterproof strain relief for all wires

---

## Software Integration

### CAN Bus Protocol (ECU ↔ PDM Communication)

#### CAN Message Structure

**From ECU to PDM** (Control Commands):
```
CAN ID: 0x600 (1536 decimal) - PDM Control

Byte 0: Channel 1-8 Enable (bit flags)
  Bit 0: Channel 1 (Fuel Pump)
  Bit 1: Channel 2 (Fan 1)
  Bit 2: Channel 3 (Fan 2)
  Bit 3: Channel 4 (Water Pump)
  Bit 4: Channel 5 (A/C Clutch)
  Bit 5: Channel 6 (Aux 1)
  Bit 6: Channel 7 (Aux 2)
  Bit 7: Channel 8 (Aux 3)

Byte 1: Channel 1 PWM (0-255) - For variable speed control
Byte 2: Channel 2 PWM
Byte 3: Channel 3 PWM
Byte 4: Channel 4 PWM
Byte 5-7: Reserved

Example:
Enable Fuel Pump (Ch1) + Fan 1 at 50% (Ch2):
[0x03, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00]
 ^^^^  ^^^^  ^^^^
  |     |     +-- Fan 1 PWM = 128 (50%)
  |     +-------- Fuel Pump PWM = 255 (100%)
  +-------------- Enable bits: 0b00000011 (Ch1 + Ch2)
```

**From PDM to ECU** (Status/Telemetry):
```
CAN ID: 0x601 (1537 decimal) - PDM Status

Byte 0: Channel 1-8 Active Status (bit flags)
Byte 1: Channel 1 Current (0-255 = 0-30A)
Byte 2: Channel 2 Current
Byte 3: Channel 3 Current
Byte 4: Channel 4 Current
Byte 5: Fault Flags (bit flags)
  Bit 0: Channel 1 overcurrent
  Bit 1: Channel 2 overcurrent
  Bit 2: Channel 3 overcurrent
  ... etc.
Byte 6: Total Current (High Byte)
Byte 7: Total Current (Low Byte)

Example:
Ch1 active (15A), Ch2 active (20A), no faults:
[0x03, 0x7F, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x23]
 ^^^^  ^^^^  ^^^^                      ^^^^  ^^^^
  |     |     |                          |     +-- Total: 0x0023 = 35A
  |     |     +-- Ch2: 0xAA = 170 (20A)  |
  |     +-------- Ch1: 0x7F = 127 (15A)  +-------- No faults
  +-------------- Active: 0b00000011 (Ch1 + Ch2)
```

---

### ECU Code (Sending PDM Commands)

**Add to `ECU_Module/RX8_CANBUS.ino`**:

```cpp
// ========== PDM CONTROL (ADD TO GLOBAL VARIABLES) ==========
// PDM channels (8 channels)
bool fuelPumpEnable = false;
bool fan1Enable = false;
bool fan2Enable = false;
bool waterPumpEnable = false;
bool acClutchEnable = false;
bool aux1Enable = false;
bool aux2Enable = false;
bool aux3Enable = false;

// PWM values for each channel (0-255)
byte fuelPumpPWM = 255;      // 100% (fuel pump always full power)
byte fan1PWM = 0;            // Variable (based on coolant temp)
byte fan2PWM = 0;
byte waterPumpPWM = 255;     // 100% (or variable for EV)
byte acClutchPWM = 255;      // 100% (on/off device)
byte aux1PWM = 0;
byte aux2PWM = 0;
byte aux3PWM = 0;

// PDM status (received from PDM)
byte pdmChannelStatus = 0;   // Active channels (bit flags)
byte pdmCurrentCh[8];        // Current per channel
byte pdmFaults = 0;          // Fault flags
int pdmTotalCurrent = 0;     // Total current draw

// ========== PDM FUNCTIONS ==========

/*
 * Update PDM control logic based on engine state
 * Called every 100ms in sendOnTenth()
 */
void updatePDMControl() {
  // 1. Fuel Pump Control
  if (engineRPM > 0) {
    fuelPumpEnable = true;
    fuelPumpPWM = 255;  // Full power when engine running
  } else {
    fuelPumpEnable = false;
  }

  // 2. Fan Control (Progressive based on coolant temp)
  if (engTemp > 95) {  // >95°C (203°F)
    fan1Enable = true;
    fan2Enable = true;
    fan1PWM = 255;  // Both fans 100%
    fan2PWM = 255;
  } else if (engTemp > 85) {  // 85-95°C (185-203°F)
    fan1Enable = true;
    fan2Enable = true;
    fan1PWM = 180;  // Fan 1: 70%
    fan2PWM = 180;  // Fan 2: 70%
  } else if (engTemp > 75) {  // 75-85°C (167-185°F)
    fan1Enable = true;
    fan2Enable = false;
    fan1PWM = 128;  // Fan 1: 50%
  } else {
    fan1Enable = false;
    fan2Enable = false;
  }

  // 3. Water Pump Control (if applicable - EV or aftermarket)
  if (engineRPM > 0) {
    waterPumpEnable = true;
    waterPumpPWM = 255;  // Full power (or make variable later)
  } else {
    waterPumpEnable = false;
  }

  // 4. A/C Clutch Control
  // (Requires input from A/C button - add later)
  acClutchEnable = false;  // Disabled for now

  // 5. Auxiliary Outputs (user-defined)
  aux1Enable = false;  // Could be: headlights, fog lights, etc.
  aux2Enable = false;
  aux3Enable = false;
}

/*
 * Send PDM control message via CAN bus
 * Called every 100ms in sendOnTenth()
 */
void sendPDMControl() {
  byte pdm_control[8];

  // Byte 0: Enable bits (8 channels)
  pdm_control[0] = 0;
  if (fuelPumpEnable) pdm_control[0] |= 0x01;
  if (fan1Enable) pdm_control[0] |= 0x02;
  if (fan2Enable) pdm_control[0] |= 0x04;
  if (waterPumpEnable) pdm_control[0] |= 0x08;
  if (acClutchEnable) pdm_control[0] |= 0x10;
  if (aux1Enable) pdm_control[0] |= 0x20;
  if (aux2Enable) pdm_control[0] |= 0x40;
  if (aux3Enable) pdm_control[0] |= 0x80;

  // Bytes 1-4: PWM values
  pdm_control[1] = fuelPumpPWM;
  pdm_control[2] = fan1PWM;
  pdm_control[3] = fan2PWM;
  pdm_control[4] = waterPumpPWM;

  // Bytes 5-7: Reserved (future use)
  pdm_control[5] = 0;
  pdm_control[6] = 0;
  pdm_control[7] = 0;

  // Send on CAN bus
  CAN0.sendMsgBuf(0x600, 0, 8, pdm_control);
}

/*
 * Receive PDM status message via CAN bus
 * Called in loop() when CAN message available
 */
void receivePDMStatus(byte buf[]) {
  pdmChannelStatus = buf[0];  // Active channels
  for (int i = 0; i < 4; i++) {
    pdmCurrentCh[i] = buf[i + 1];  // Current per channel
  }
  pdmFaults = buf[5];  // Fault flags
  pdmTotalCurrent = (buf[6] << 8) | buf[7];  // Total current (16-bit)

  // Check for faults
  if (pdmFaults != 0) {
    Serial.print("PDM FAULT: ");
    Serial.println(pdmFaults, BIN);
    checkEngineMIL = 1;  // Light up check engine light
  }

  // Optional: Print telemetry
  Serial.print("PDM Total Current: ");
  Serial.print(pdmTotalCurrent);
  Serial.println(" A");
}

// ========== INTEGRATE INTO EXISTING CODE ==========

// In sendOnTenth() function (around line 289):
void sendOnTenth() {
  // ... existing code ...

  updatePDMControl();   // ← ADD THIS
  sendPDMControl();     // ← ADD THIS

  updateOdometer();
  updateMIL();
  CAN0.sendMsgBuf(0x420, 0, 7, send420);

  // ... rest of function ...
}

// In loop() function (around line 357), add to CAN message handler:
if (ID == 0x601) {  // PDM Status message
  receivePDMStatus(buf);
}
```

---

### PDM Code (Separate Arduino Mega Module)

**Create new file**: `PDM_Module/PDM_Module.ino`

```cpp
/*
 * RX8 Power Distribution Module (PDM)
 * Hardware: Arduino Mega 2560 + MCP2515 CAN + MOSFET array
 * Author: [Your Name]
 * Date: 2025-11-17
 */

#include <Arduino.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>

// ========== PIN DEFINITIONS ==========
#define CAN_CS_PIN 53
#define CAN_INT_PIN 2

// MOSFET control pins (8 channels)
#define CH1_PIN 22  // Fuel Pump
#define CH2_PIN 24  // Fan 1
#define CH3_PIN 26  // Fan 2
#define CH4_PIN 28  // Water Pump
#define CH5_PIN 30  // A/C Clutch
#define CH6_PIN 32  // Aux 1
#define CH7_PIN 34  // Aux 2
#define CH8_PIN 36  // Aux 3

// Current sensor analog pins (8 channels)
#define CH1_CURRENT A0
#define CH2_CURRENT A1
#define CH3_CURRENT A2
#define CH4_CURRENT A3
#define CH5_CURRENT A4
#define CH6_CURRENT A5
#define CH7_CURRENT A6
#define CH8_CURRENT A7

// Status LEDs (8 channels)
#define CH1_LED 38
#define CH2_LED 40
#define CH3_LED 42
#define CH4_LED 44
#define CH5_LED 46
#define CH6_LED 48
#define CH7_LED 50
#define CH8_LED 52

// ========== CONFIGURATION ==========
#define NUM_CHANNELS 8
#define CURRENT_LIMIT_DEFAULT 30  // 30A per channel
#define SERIAL_BAUD 115200

// ========== GLOBAL VARIABLES ==========
MCP_CAN CAN0(CAN_CS_PIN);

// Channel configuration
struct Channel {
  byte pin;           // MOSFET control pin
  byte currentPin;    // Analog current sensor pin
  byte ledPin;        // Status LED pin
  bool enabled;       // Enable state (from ECU)
  byte pwm;           // PWM value (0-255)
  float current;      // Measured current (Amps)
  float currentLimit; // Current limit (Amps)
  bool faulted;       // Overcurrent fault flag
};

Channel channels[NUM_CHANNELS] = {
  {CH1_PIN, CH1_CURRENT, CH1_LED, false, 0, 0, 30, false},  // Fuel Pump
  {CH2_PIN, CH2_CURRENT, CH2_LED, false, 0, 0, 25, false},  // Fan 1
  {CH3_PIN, CH3_CURRENT, CH3_LED, false, 0, 0, 25, false},  // Fan 2
  {CH4_PIN, CH4_CURRENT, CH4_LED, false, 0, 0, 15, false},  // Water Pump
  {CH5_PIN, CH5_CURRENT, CH5_LED, false, 0, 0, 10, false},  // A/C Clutch
  {CH6_PIN, CH6_CURRENT, CH6_LED, false, 0, 0, 15, false},  // Aux 1
  {CH7_PIN, CH7_CURRENT, CH7_LED, false, 0, 0, 15, false},  // Aux 2
  {CH8_PIN, CH8_CURRENT, CH8_LED, false, 0, 0, 15, false}   // Aux 3
};

// Timing
unsigned long lastCANSend = 0;
unsigned long lastCurrentCheck = 0;

// ========== SETUP ==========
void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println(F("RX8 PDM Starting..."));

  // Initialize CAN bus
  if (CAN0.begin(CAN_500KBPS) == CAN_OK) {
    Serial.println(F("CAN bus initialized"));
  } else {
    Serial.println(F("CAN bus initialization FAILED"));
    while (1);  // Halt
  }

  pinMode(CAN_INT_PIN, INPUT);

  // Initialize channels
  for (int i = 0; i < NUM_CHANNELS; i++) {
    pinMode(channels[i].pin, OUTPUT);
    pinMode(channels[i].ledPin, OUTPUT);
    digitalWrite(channels[i].pin, LOW);    // All off initially
    digitalWrite(channels[i].ledPin, LOW);
  }

  Serial.println(F("PDM Ready"));
}

// ========== MAIN LOOP ==========
void loop() {
  // 1. Check for CAN messages from ECU
  if (CAN_MSGAVAIL == CAN0.checkReceive()) {
    processCANMessage();
  }

  // 2. Update channel outputs (apply enable/PWM states)
  updateChannels();

  // 3. Monitor current (every 100ms)
  if (millis() - lastCurrentCheck >= 100) {
    lastCurrentCheck = millis();
    monitorCurrent();
  }

  // 4. Send status to ECU (every 100ms)
  if (millis() - lastCANSend >= 100) {
    lastCANSend = millis();
    sendCANStatus();
  }
}

// ========== CAN MESSAGE PROCESSING ==========
void processCANMessage() {
  unsigned long id;
  byte len;
  byte buf[8];

  CAN0.readMsgBufID(&id, &len, buf);

  if (id == 0x600) {  // PDM Control from ECU
    // Byte 0: Enable bits
    for (int i = 0; i < NUM_CHANNELS; i++) {
      channels[i].enabled = (buf[0] & (1 << i)) != 0;
    }

    // Bytes 1-4: PWM values
    channels[0].pwm = buf[1];  // Ch1 PWM
    channels[1].pwm = buf[2];  // Ch2 PWM
    channels[2].pwm = buf[3];  // Ch3 PWM
    channels[3].pwm = buf[4];  // Ch4 PWM
    // Channels 5-8 use default PWM (255 = 100%)
  }
}

// ========== CHANNEL CONTROL ==========
void updateChannels() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    if (channels[i].enabled && !channels[i].faulted) {
      // Enable channel with PWM
      analogWrite(channels[i].pin, channels[i].pwm);
      digitalWrite(channels[i].ledPin, HIGH);  // LED on
    } else {
      // Disable channel
      digitalWrite(channels[i].pin, LOW);
      digitalWrite(channels[i].ledPin, channels[i].faulted ? HIGH : LOW);  // LED blinks if faulted
    }
  }
}

// ========== CURRENT MONITORING ==========
void monitorCurrent() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    // Read current from ACS712 sensor
    int sensorValue = analogRead(channels[i].currentPin);
    float voltage = sensorValue * (5.0 / 1023.0);
    channels[i].current = abs((voltage - 2.5) / 0.066);  // 66 mV/A for ACS712-30A

    // Check for overcurrent
    if (channels[i].current > channels[i].currentLimit) {
      channels[i].faulted = true;
      Serial.print(F("OVERCURRENT Ch"));
      Serial.print(i + 1);
      Serial.print(F(": "));
      Serial.print(channels[i].current);
      Serial.println(F(" A"));
    }
  }
}

// ========== CAN STATUS TRANSMISSION ==========
void sendCANStatus() {
  byte status[8];

  // Byte 0: Active channels (bit flags)
  status[0] = 0;
  for (int i = 0; i < NUM_CHANNELS; i++) {
    if (channels[i].enabled && !channels[i].faulted) {
      status[0] |= (1 << i);
    }
  }

  // Bytes 1-4: Current per channel (scaled to 0-255)
  status[1] = constrain(channels[0].current * 8.5, 0, 255);  // Ch1 current (30A = 255)
  status[2] = constrain(channels[1].current * 8.5, 0, 255);  // Ch2
  status[3] = constrain(channels[2].current * 8.5, 0, 255);  // Ch3
  status[4] = constrain(channels[3].current * 8.5, 0, 255);  // Ch4

  // Byte 5: Fault flags
  status[5] = 0;
  for (int i = 0; i < NUM_CHANNELS; i++) {
    if (channels[i].faulted) {
      status[5] |= (1 << i);
    }
  }

  // Bytes 6-7: Total current (16-bit, 0-1000A range)
  float totalCurrent = 0;
  for (int i = 0; i < NUM_CHANNELS; i++) {
    totalCurrent += channels[i].current;
  }
  int totalCurrentInt = (int)totalCurrent;
  status[6] = (totalCurrentInt >> 8) & 0xFF;  // High byte
  status[7] = totalCurrentInt & 0xFF;          // Low byte

  // Send on CAN bus
  CAN0.sendMsgBuf(0x601, 0, 8, status);
}
```

---

## Safety & Testing

### Safety Features (CRITICAL)

#### 1. **Overcurrent Protection** ⚡
```cpp
// In monitorCurrent():
if (channels[i].current > channels[i].currentLimit) {
  channels[i].faulted = true;
  digitalWrite(channels[i].pin, LOW);  // Turn off immediately
  // Optionally: require manual reset (button press)
}
```

#### 2. **Failsafe: CAN Bus Timeout**
```cpp
// In loop():
if (millis() - lastCANReceive > 500) {  // 500ms timeout
  // ECU not responding - shut down all channels
  for (int i = 0; i < NUM_CHANNELS; i++) {
    digitalWrite(channels[i].pin, LOW);
  }
  Serial.println(F("FAILSAFE: CAN timeout!"));
}
```

#### 3. **Inline Fuses (Hardware Backup)**
- **Always use fuses** on each output
- PDM software protection is **not enough**
- Fuses prevent wiring fires

#### 4. **Watchdog Timer**
```cpp
// In setup():
wdt_enable(WDTO_1S);  // 1-second watchdog

// In loop():
wdt_reset();  // Reset watchdog (must be called every loop)
```

---

### Testing Procedure

#### Phase 1: Bench Test (NO VEHICLE)

1. **Power Supply Test**
   - Use lab power supply (12V, 5A limit)
   - Connect to PDM input
   - Verify Arduino powers up
   - Check all LEDs light up during self-test

2. **Channel Control Test**
   - Enable each channel via serial commands
   - Verify MOSFET switches on (use multimeter)
   - Verify LED lights up
   - Measure voltage at output (should be ~12V)

3. **PWM Test**
   - Set channel to 50% PWM
   - Measure output voltage (should be ~6V average)
   - Connect small fan (12V, <1A)
   - Verify fan speed changes with PWM

4. **Current Sensing Test**
   - Connect known resistive load (e.g., 12V/5A bulb)
   - Measure current with multimeter
   - Compare to PDM reading
   - Calibrate if needed

5. **Overcurrent Test**
   - Set current limit to 5A
   - Connect 10A load
   - Verify PDM shuts off channel
   - Verify fault flag set

#### Phase 2: Vehicle Test (ENGINE OFF)

1. **CAN Bus Integration**
   - Connect PDM to OBD2 (CAN only, not 12V yet)
   - Start ECU code
   - Verify CAN messages received (Serial monitor)
   - Check for CAN errors

2. **Low-Power Device Test**
   - Connect small device (e.g., LED light bar, <2A)
   - Enable via ECU (set `aux1Enable = true`)
   - Verify device turns on
   - Check current reading

#### Phase 3: Vehicle Test (ENGINE ON) ⚠️ **CAUTION**

1. **Fuel Pump Test** (CRITICAL)
   - **Safety**: Have fire extinguisher ready
   - Connect fuel pump to PDM Ch1
   - Start engine with ECU
   - Verify fuel pump activates when ECU starts
   - Check current draw (should be 8-15A)
   - **Monitor for leaks!**

2. **Fan Test**
   - Connect radiator fan to Ch2
   - Override coolant temp to 95°C in code
   - Verify fan activates
   - Check PWM control (variable speed)
   - Monitor current (should be 15-25A)

3. **Full Load Test**
   - Enable all channels simultaneously
   - Monitor total current draw
   - Check for voltage sag (<11V = problem)
   - Verify no overheating (MOSFETs, wiring)

---

## Next Steps

### Immediate (Design Phase)

1. ✅ Read this guide (you're here!)
2. Choose implementation option (recommend Option 4)
3. Order hardware:
   - Arduino Mega 2560
   - MCP2515 CAN module
   - MOSFET modules (x8)
   - ACS712 current sensors (x8)
   - Fuses, wire, connectors
4. Design PCB (optional but recommended)

### Short-Term (Build Phase)

5. Prototype on breadboard (test 1-2 channels)
6. Write PDM code (start with 1 channel)
7. Test bench (power supply + resistive load)
8. Integrate with ECU code (CAN messages)

### Long-Term (Deployment)

9. Build final PCB or enclosure
10. Vehicle test (low-power devices first)
11. Engine test (fuel pump, fans)
12. Document and share with community!

---

## Cost Estimate

### Option 4 (Separate Arduino Mega PDM) - 8 Channels

| Component | Quantity | Unit Price | Total |
|-----------|----------|------------|-------|
| Arduino Mega 2560 (clone) | 1 | $18 | $18 |
| MCP2515 CAN module | 1 | $8 | $8 |
| MOSFET modules (30A) | 8 | $7 | $56 |
| ACS712 current sensors (30A) | 8 | $4 | $32 |
| Blade fuse holders | 8 | $2 | $16 |
| Fuses (assorted) | 20 | $0.50 | $10 |
| Enclosure (waterproof) | 1 | $20 | $20 |
| Wire (14 AWG, 50 ft) | 1 | $15 | $15 |
| Connectors (Anderson, ring) | 20 | $1 | $20 |
| PCB (optional, JLCPCB 5x) | 1 | $10 | $10 |
| **Total** | | | **~$205** |

**Compare to**:
- Commercial PDM: $800-1,500 (4x-7x cost)
- Pre-made relay board: $50-100 (but no current sensing, PWM, or CAN)

---

## Conclusion

Adding a PDM to your RX8 Arduino ECU is **highly recommended** for:
- ✅ Engine swaps (LS1, 2JZ, etc.)
- ✅ EV conversions (motor + battery control)
- ✅ Race cars (weight reduction, telemetry)
- ✅ Advanced features (progressive fans, soft-start, fault detection)

**Recommended Approach**: **Option 4** (Separate Arduino Mega PDM)
- Modular, scalable, cost-effective
- Full CAN bus integration with existing ECU
- ~$200 total cost (vs. $800+ commercial)

**Next Step**: Order hardware and start prototyping!

---

**Questions? See also**:
- `ECU_Module/RX8_CANBUS.ino` - ECU integration code
- `EV_ECU_Module/` - EV conversion (would benefit from PDM)
- `docs/AUTOMOTIVE_MCU_MIGRATION.md` - Future STM32 upgrade

---

**End of PDM Integration Guide**
