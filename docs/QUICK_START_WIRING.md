# Quick Start Wiring - Minimal Boost Control Setup

**Goal**: Get boost control working as first feature proof-of-concept

**Time**: 4-6 hours
**Cost**: ~$200
**Difficulty**: Intermediate

---

## What You'll Build

A minimal working system with:
- ✅ CAN bus communication (read wheel speeds, RPM, throttle)
- ✅ Closed-loop boost control
- ✅ Basic safety (overboost protection)
- ✅ Data logging to SD card

This proves the concept before investing in full harness.

---

## Parts List (Minimal)

| Item | Qty | Cost | Link/Part# |
|------|-----|------|------------|
| Arduino Mega 2560 | 1 | $40 | Amazon "Arduino Mega" |
| MCP2515 CAN module | 1 | $8 | Amazon "MCP2515 CAN bus" |
| SD card module | 1 | $5 | Amazon "SD card module Arduino" |
| MAC 3-port solenoid | 1 | $80 | Turbosmart TS-0101-3001 |
| AEM boost sensor | 1 | $60 | AEM 30-2130-50 (0-50 PSI) |
| MOSFET (IRLB8721) | 1 | $2 | Mouser/Digikey |
| 1N4007 diode | 1 | $0.50 | Mouser/Digikey |
| Breadboard jumpers | 20 | $5 | Amazon |
| Project box | 1 | $10 | Hammond 1591XXSBK |

**Total**: ~$200

---

## Wiring Diagram (Minimal Boost Control)

```
┌─────────────────────────────────────────────────────┐
│              ARDUINO MEGA 2560                      │
│                                                     │
│  Digital Pins:                                      │
│  ┌────────────────────────────────────────┐        │
│  │ 2  → MCP2515 INT                       │        │
│  │ 10 → MCP2515 CS                        │        │
│  │ 11 → MCP2515 SI (MOSI)                 │        │
│  │ 12 → MCP2515 SO (MISO)                 │        │
│  │ 13 → MCP2515 SCK                       │        │
│  │                                         │        │
│  │ 22 → MOSFET Gate (Boost Solenoid)      │        │
│  │ 23 → SD Card CS                        │        │
│  │ 50 → SD Card MISO                      │        │
│  │ 51 → SD Card MOSI                      │        │
│  │ 52 → SD Card SCK                       │        │
│  └────────────────────────────────────────┘        │
│                                                     │
│  Analog Pins:                                       │
│  ┌────────────────────────────────────────┐        │
│  │ A0 → Boost Sensor Signal               │        │
│  └────────────────────────────────────────┘        │
│                                                     │
│  Power:                                             │
│  ┌────────────────────────────────────────┐        │
│  │ 5V  → Boost sensor power               │        │
│  │ GND → All grounds (star topology)      │        │
│  │ Vin → 12V (from relay)                 │        │
│  └────────────────────────────────────────┘        │
└─────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────┐
│              MCP2515 CAN MODULE                     │
│  ┌────────────────────────────────────────┐        │
│  │ VCC  → Arduino 5V                      │        │
│  │ GND  → Arduino GND                     │        │
│  │ CS   → Arduino D10                     │        │
│  │ SO   → Arduino D12 (MISO)              │        │
│  │ SI   → Arduino D11 (MOSI)              │        │
│  │ SCK  → Arduino D13                     │        │
│  │ INT  → Arduino D2                      │        │
│  │ CANH → Vehicle CAN H (via OBD2)        │        │
│  │ CANL → Vehicle CAN L (via OBD2)        │        │
│  └────────────────────────────────────────┘        │
└─────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────┐
│         BOOST SENSOR (AEM 30-2130-50)               │
│  ┌────────────────────────────────────────┐        │
│  │ Pin 1 (Red)   → Arduino 5V             │        │
│  │ Pin 2 (White) → Arduino A0             │        │
│  │ Pin 3 (Black) → Arduino GND            │        │
│  └────────────────────────────────────────┘        │
│                                                     │
│  Mount sensor on intake manifold with vacuum line  │
│  Use 1/8" NPT fitting                               │
└─────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────┐
│      WASTEGATE SOLENOID (MAC 3-PORT)                │
│                                                     │
│  Pneumatic Connections:                             │
│  Port 1: To atmosphere (filter recommended)        │
│  Port 2: To wastegate actuator                     │
│  Port 3: To boost source (intake manifold)         │
│                                                     │
│  Electrical Connection:                             │
│  ┌────────────────────────────────────────┐        │
│  │                                         │        │
│  │         MOSFET DRIVER CIRCUIT           │        │
│  │                                         │        │
│  │  Arduino D22 ─── 1kΩ ─── Gate         │        │
│  │                          │              │        │
│  │                      IRLB8721           │        │
│  │                          │              │        │
│  │                  ┌───────┴──────┐      │        │
│  │                  │              │      │        │
│  │             Drain (D)       Source (S) │        │
│  │                  │              │      │        │
│  │            ┌─────┴─────┐       │      │        │
│  │            │           │       │      │        │
│  │         Solenoid    1N4007    GND     │        │
│  │          Coil      (Flyback)          │        │
│  │            │           │               │        │
│  │           12V      (Cathode            │        │
│  │         (Fused)     to 12V)            │        │
│  │                                         │        │
│  └────────────────────────────────────────┘        │
└─────────────────────────────────────────────────────┘
```

---

## Step-by-Step Build

### Step 1: Breadboard Prototype (1 hour)

**Goal**: Test circuit on breadboard before soldering

1. Insert Arduino Mega into breadboard (or use separate breadboard)
2. Connect MCP2515 module:
   - VCC → Arduino 5V
   - GND → Arduino GND
   - CS → D10, SO → D12, SI → D11, SCK → D13, INT → D2
3. Connect SD card module:
   - VCC → 5V, GND → GND
   - CS → D23, MISO → D50, MOSI → D51, SCK → D52
4. Connect boost sensor:
   - Red (5V) → Arduino 5V
   - White (Signal) → A0
   - Black (GND) → GND
5. Build MOSFET driver on breadboard:
   - D22 → 1kΩ resistor → Gate
   - Source → GND
   - Drain → Solenoid (connect later)
   - 1N4007 across solenoid (cathode to +12V)

### Step 2: Upload Test Code (30 min)

Use the test sketch:

```cpp
// File: test_boost_control.ino
#include <mcp_can.h>

MCP_CAN CAN0(10);  // CS pin 10

int boostPin = A0;
int solenoidPin = 22;

void setup() {
  Serial.begin(115200);
  pinMode(solenoidPin, OUTPUT);

  // Initialize CAN bus
  if(CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("CAN Init OK!");
  } else {
    Serial.println("CAN Init Failed!");
  }

  CAN0.setMode(MCP_NORMAL);
}

void loop() {
  // Read boost sensor
  int sensorValue = analogRead(boostPin);
  float voltage = sensorValue * (5.0 / 1023.0);
  float boost_psi = (voltage - 0.5) * 10.0;  // AEM sensor: 0.5V = 0 PSI, linear

  Serial.print("Boost: ");
  Serial.print(boost_psi);
  Serial.println(" PSI");

  // Simple boost control (target 10 PSI for testing)
  float target = 10.0;
  if(boost_psi > target) {
    digitalWrite(solenoidPin, HIGH);  // Vent to atmosphere
  } else {
    digitalWrite(solenoidPin, LOW);   // Build boost
  }

  delay(100);
}
```

**Upload and test**:
- Open Serial Monitor (115200 baud)
- Should see "CAN Init OK!" and boost readings
- Blow into boost sensor (gently!) to see reading change

### Step 3: Vehicle CAN Bus Connection (30 min)

**Option A: OBD2 Port** (Easy, temporary)

1. Buy OBD2 breakout cable ($10 on Amazon)
2. Identify CAN pins:
   - Pin 6: CAN H
   - Pin 14: CAN L
   - Pin 4/5: GND
3. Connect to MCP2515:
   - Pin 6 → CANH
   - Pin 14 → CANL
   - Pin 4 → GND

**Option B: Direct Tap** (Permanent)

1. Locate CAN bus wiring (see RX8 FSM)
2. Solder tap connectors (don't cut wires!)
3. Use twisted pair to MCP2515
4. Add 120Ω termination resistor between CANH/CANL

**Test**:
- Start vehicle
- Should see CAN messages in Serial Monitor
- Verify RPM changes with throttle

### Step 4: Install Boost Sensor (1 hour)

1. Find vacuum port on intake manifold
2. Install 1/8" NPT to barb fitting
3. Run vacuum line to sensor (keep short, <12")
4. Mount sensor in engine bay (away from heat)
5. Connect 3-wire cable to Arduino (run through firewall)

**Test**:
- Start engine
- Should see ~0 PSI at idle (slight vacuum OK)
- Rev engine: should see slight positive pressure

### Step 5: Install Wastegate Solenoid (1 hour)

**Solenoid Placement**:
- Mount near wastegate actuator
- Keep vacuum lines short
- Away from exhaust heat

**Plumbing** (3-port MAC solenoid):
```
Port 1 (Atm) ──── Small filter (prevent dust)
Port 2 (WG)  ──── To wastegate actuator
Port 3 (Boost) ── To intake manifold (boost source)
```

**Electrical**:
1. Run 2-wire cable from solenoid to Arduino
2. Connect to MOSFET driver circuit
3. Fuse 12V supply (5A fuse)

**Test** (engine off):
- Apply 12V to solenoid (should click)
- Check for leaks (spray soapy water)

### Step 6: Bench Calibration (30 min)

**Boost Sensor Calibration**:

1. At sea level (atmospheric pressure):
   - Read sensor voltage
   - Should be ~2.5V (14.7 PSI absolute)
   - Adjust code if needed:
   ```cpp
   float boost_psi = (voltage - 2.5) * 10.0 + 14.7;
   float boost_gauge = boost_psi - 14.7;  // Gauge pressure
   ```

2. With vacuum pump (if available):
   - Apply -5 PSI vacuum
   - Sensor should read ~2.0V
   - Apply +5 PSI pressure
   - Sensor should read ~3.0V

### Step 7: Vehicle Testing (1-2 hours)

**⚠️ SAFETY FIRST**:
- Have helper monitor laptop
- Start conservatively (low boost target)
- Test in safe location (empty parking lot)
- Be ready to shut down

**Test Procedure**:

1. **Idle Test** (5 min):
   - Engine running, no load
   - Verify sensor reads 0 PSI
   - Verify no CAN errors

2. **Rev Test** (5 min):
   - Rev engine in neutral
   - Watch boost readings
   - Should see slight pressure

3. **Drive Test - Low Boost** (15 min):
   - Set target to 5 PSI (conservative)
   - Drive gently, light throttle
   - Monitor actual vs target boost
   - Adjust PID gains if needed

4. **Drive Test - Medium Boost** (15 min):
   - Increase target to 8-10 PSI
   - More aggressive driving
   - Watch for overboost
   - Verify solenoid response

5. **Data Logging** (ongoing):
   - Log all tests to SD card
   - Review for tuning

---

## Troubleshooting

### CAN Bus Not Working

**Symptom**: "CAN Init Failed!" message

**Checks**:
- [ ] Verify MCP2515 wiring (especially SI/SO)
- [ ] Check SPI connections (continuity test)
- [ ] Verify 8MHz crystal on MCP2515 (use 16MHz if different)
- [ ] Add 120Ω termination resistor
- [ ] Check vehicle CAN H/L not swapped

### Boost Sensor Reading Incorrect

**Symptom**: Shows 50 PSI at idle or negative at WOT

**Checks**:
- [ ] Verify sensor voltage at A0 (multimeter)
- [ ] Check sensor power (should be 5V)
- [ ] Verify sensor ground
- [ ] Check for vacuum leaks
- [ ] Recalibrate offset in code

### Solenoid Not Clicking

**Symptom**: No solenoid response, boost uncontrolled

**Checks**:
- [ ] MOSFET wired correctly (Gate/Drain/Source)
- [ ] 12V supply present (fuse OK)
- [ ] D22 outputting PWM (test with LED)
- [ ] MOSFET not damaged (test with multimeter)
- [ ] Flyback diode correct polarity

### Boost Overshooting Target

**Symptom**: Boost goes to 15 PSI when target is 10 PSI

**Solution**: Tune PID gains in code:
```cpp
// In boost_control.cpp
// Reduce P gain (make less aggressive)
pid_kp = 0.5;  // Was 1.0
pid_ki = 0.1;  // Was 0.2
pid_kd = 0.05; // Was 0.1
```

---

## Next Steps After Proof-of-Concept

Once boost control is working:

1. ✅ Add launch control (easy - just a switch)
2. ✅ Add traction control (reads wheel speeds from CAN)
3. ✅ Add data logging (SD card already installed)
4. ✅ Build proper wiring harness (see HARDWARE_WIRING_GUIDE.md)
5. ✅ Add remaining features incrementally

---

## Cost Comparison

| Approach | Cost | Time | Reliability |
|----------|------|------|-------------|
| **This DIY** | $200 | 6 hrs | Good (if done right) |
| **Haltech Base** | $1,200 | 2 hrs | Excellent |
| **Haltech Elite** | $2,500 | 2 hrs | Excellent |
| **Standalone shop install** | $2,000+ | 8 hrs | Excellent |

**Savings**: $1,000-2,300 by doing it yourself!

---

**Last Updated**: 2025-11-16
**Tested On**: Arduino Mega 2560 + MCP2515
**Vehicle**: Mazda RX8 (2004-2011)

**Next**: See HARDWARE_WIRING_GUIDE.md for complete system wiring
