# Automotive MCU Migration Plan

## Overview

**Date**: 2025-11-15
**Purpose**: Migrate from hobbyist Arduino boards to automotive-grade MCUs for safety-critical engine control
**Priority**: HIGH - Safety improvement

---

## Current Architecture Problems

### Critical Safety Issues with Arduino Leonardo

**Current Setup:**
- **Arduino Leonardo** (ATmega32U4) controlling ENGINE
- **Problem**: This is a HOBBYIST board, not automotive-grade!

**Why This Is Dangerous:**

| Issue | Arduino Leonardo | Risk Level |
|-------|------------------|------------|
| **Temperature Range** | 0°C to 85°C | 🔴 **CRITICAL** - Engine bay reaches 120°C+ |
| **EMI Protection** | Minimal | 🔴 **CRITICAL** - Vehicle EMI can cause crashes |
| **Watchdog** | Software only | 🔴 **CRITICAL** - Code freeze = no failsafe |
| **Power Supply** | 5V regulated | 🟡 **MEDIUM** - No brownout protection |
| **Flash Endurance** | 10K cycles | 🟡 **MEDIUM** - Limited for frequent updates |
| **Safety Certification** | None | 🔴 **CRITICAL** - No ISO 26262, ASIL |
| **CAN Controller** | External (MCP2515) | 🟡 **MEDIUM** - Extra failure point |
| **Real-Time OS** | None | 🟡 **MEDIUM** - No deterministic timing |

**VERDICT**: Arduino is **NOT SUITABLE** for safety-critical engine control!

---

## Proposed Architecture

### Two-Tier System

```
┌─────────────────────────────────────────────────────┐
│ TIER 1: CRITICAL (Automotive MCU)                   │
├─────────────────────────────────────────────────────┤
│ - Engine Control (throttle, RPM, safety checks)    │
│ - CAN Bus ECU emulation (0x201, 0x420, etc.)       │
│ - Immobilizer bypass                                 │
│ - ABS/DSC emulation (if controlling brakes)        │
│                                                      │
│ Hardware: STM32F4, C2000, S32K, AURIX, Hercules    │
│ Temp Range: -40°C to 125°C (automotive grade)      │
│ Features: Hardware watchdog, CAN-FD, RTOS, EMI     │
└─────────────────────────────────────────────────────┘
                            │
                            │ CAN Bus
                            ▼
┌─────────────────────────────────────────────────────┐
│ TIER 2: NON-CRITICAL (ESP32/Arduino)                │
├─────────────────────────────────────────────────────┤
│ - AC Display control                                 │
│ - Aftermarket gauges                                 │
│ - Speed-sensitive wipers                            │
│ - WiFi/Bluetooth telemetry                          │
│ - Data logging                                       │
│                                                      │
│ Hardware: ESP32, Arduino Mega, etc.                 │
│ Temp Range: 0°C to 85°C (consumer grade) - OK     │
│ Features: WiFi, displays, user interface            │
└─────────────────────────────────────────────────────┘
```

**Key Principle**: If it can KILL someone, use automotive MCU. If it's convenience, use cheaper boards.

---

## Hardware Recommendations

### Option 1: STM32F4 Series (BEST for DIY)

**Recommended Board**: STM32F407VET6 DevKit (~$15)

**Specifications:**
- **CPU**: ARM Cortex-M4 @ 168 MHz
- **Flash**: 512 KB (plenty for ECU code)
- **RAM**: 192 KB (vs 2.5 KB on Leonardo!)
- **Temperature**: -40°C to 85°C (industrial)
  - Upgrade to automotive-grade STM32F407 (-40°C to 125°C)
- **CAN**: 2x CAN 2.0B controllers (built-in!)
- **Watchdog**: Independent hardware watchdog
- **ADC**: 3x 12-bit ADC (better than Arduino's 10-bit)
- **Timers**: 14 timers (precise PWM for throttle)
- **RTOS**: FreeRTOS support
- **Debugger**: SWD debugging (professional)

**Pros:**
- ✅ Cheap (~$15)
- ✅ Large community (STM32CubeIDE, Arduino Core)
- ✅ Easy migration from Arduino code
- ✅ Built-in CAN (no MCP2515 needed!)
- ✅ Plenty of resources online

**Cons:**
- ⚠️ DevKit is industrial, not automotive (-40°C to 85°C)
- ⚠️ Need to buy automotive-grade chip separately for full temp range
- ⚠️ No safety certification (DIY only)

---

### Option 2: TI C2000 (BEST for Motor Control)

**Recommended Board**: TMS320F28379D LaunchPad (~$30)

**Specifications:**
- **CPU**: Dual C28x cores @ 200 MHz
- **Flash**: 1 MB
- **RAM**: 204 KB
- **Temperature**: -40°C to 125°C (automotive grade)
- **CAN**: 2x CAN-FD controllers
- **PWM**: 24 channels with nanosecond precision (PERFECT for motor control)
- **ADC**: 16-bit ADC (vs 10-bit on Arduino!)
- **Safety**: Hardware-based safety features
- **FPU**: 32-bit & 64-bit floating point

**Pros:**
- ✅ Designed FOR automotive motor control
- ✅ Automotive temperature range
- ✅ Best-in-class PWM (crucial for throttle/motor)
- ✅ TI Code Composer Studio (free IDE)
- ✅ Safety-focused architecture

**Cons:**
- ⚠️ Harder to program (not Arduino-compatible)
- ⚠️ Steeper learning curve
- ⚠️ More expensive ($30 vs $15)

---

### Option 3: NXP S32K (TRUE Automotive MCU)

**Recommended Board**: S32K144EVB (~$50)

**Specifications:**
- **CPU**: ARM Cortex-M4F @ 80 MHz
- **Flash**: 512 KB
- **RAM**: 64 KB
- **Temperature**: -40°C to 150°C (AUTOMOTIVE GRADE)
- **CAN**: 3x CAN-FD controllers
- **Safety**: ISO 26262 ASIL-B capable
- **Watchdog**: Multiple independent watchdogs
- **Power**: Low-power modes, brownout protection
- **EMI**: Automotive EMI/EMC compliant

**Pros:**
- ✅ **TRUE automotive MCU** (used in real cars!)
- ✅ ISO 26262 safety certification path
- ✅ Extreme temperature range (-40°C to 150°C)
- ✅ Automotive EMI protection
- ✅ NXP S32 Design Studio (free IDE)
- ✅ CAN-FD support (future-proof)

**Cons:**
- ⚠️ Expensive ($50+ for dev board)
- ⚠️ Complex toolchain
- ⚠️ Not Arduino-compatible
- ⚠️ Requires automotive engineering knowledge

---

### Option 4: Infineon AURIX (PROFESSIONAL Grade)

**Board**: TriCore TC275 DevKit (~$150)

**Specifications:**
- **CPU**: TriCore @ 200 MHz
- **Temperature**: -40°C to 125°C
- **Safety**: ISO 26262 ASIL-D (highest safety level)
- **CAN**: MultiCAN (up to 4 nodes)
- **Features**: Lockstep cores, safety monitor, ECC memory

**Pros:**
- ✅ Highest safety level (ASIL-D)
- ✅ Used in real production vehicles
- ✅ Triple-core architecture (safety core)
- ✅ Best reliability

**Cons:**
- 🔴 Very expensive ($150+)
- 🔴 Professional-level complexity
- 🔴 Overkill for DIY project
- 🔴 Steep learning curve

---

### Option 5: TI Hercules (Safety-Critical)

**Board**: TMS570LS0432 LaunchPad (~$30)

**Specifications:**
- **CPU**: ARM Cortex-R4F @ 80 MHz
- **Temperature**: -40°C to 125°C
- **Safety**: ISO 26262 ASIL-D capable
- **Lockstep**: Dual lockstep CPUs (error detection)
- **CAN**: 3x CAN controllers
- **Memory**: ECC on all RAM/Flash

**Pros:**
- ✅ Safety-focused (lockstep cores detect errors)
- ✅ Automotive temperature range
- ✅ Reasonably priced ($30)
- ✅ TI Code Composer Studio support

**Cons:**
- ⚠️ Complex safety features (may be overkill)
- ⚠️ Harder to program than STM32
- ⚠️ Less community support

---

## Recommended Migration Path

### Phase 1: Proof of Concept (STM32F4)

**Board**: STM32F407VET6 DevKit ($15)
**Reason**: Easy migration, Arduino-compatible, cheap
**Timeline**: 1-2 weeks

**Steps:**
1. Port ECU code to STM32 using STM32duino
2. Test built-in CAN (remove MCP2515)
3. Validate throttle control (12-bit ADC)
4. Bench test all functions
5. Compare with Arduino version

---

### Phase 2: Automotive Hardening (STM32 Automotive or C2000)

**Board**: Automotive-grade STM32F407 chip OR TI C2000 LaunchPad
**Reason**: Full temperature range, better safety
**Timeline**: 2-3 weeks

**Steps:**
1. Migrate to automotive-grade chip/board
2. Implement hardware watchdog
3. Add brownout protection
4. Implement fail-safe modes
5. Add EMI filtering
6. Extended temperature testing

---

### Phase 3: Production Ready (NXP S32K) - OPTIONAL

**Board**: S32K144EVB ($50)
**Reason**: ISO 26262 compliance path, true automotive MCU
**Timeline**: 1-2 months

**Steps:**
1. Migrate to S32K platform
2. Implement safety monitors
3. Add diagnostic trouble codes (DTCs)
4. Implement ISO 26262 safety measures
5. Professional EMI/EMC testing
6. Vehicle validation testing

---

## Critical vs Non-Critical Function Classification

### TIER 1: CRITICAL (Automotive MCU Required)

**Engine Control:**
- ✅ Throttle pedal processing
- ✅ Throttle safety limits (prevent runaway)
- ✅ Engine RPM monitoring
- ✅ Wheel speed mismatch detection
- ✅ Emergency shutdown logic

**CAN Bus Emulation:**
- ✅ 0x201 - PCM Status (RPM, Speed, Throttle)
- ✅ 0x420 - Warning Lights (Engine Temp, Oil)
- ✅ 0x203/215/231/240 - PCM Supplements
- ✅ 0x041 - Immobilizer (prevents theft)
- ✅ 0x620/630/650 - ABS Data (if controlling brakes)

**Why Critical:**
- Throttle malfunction → Uncontrolled acceleration → FATAL
- RPM/Speed wrong → Dashboard misleads driver → ACCIDENT
- ABS disabled → No braking assistance → CRASH
- Immobilizer fail → Vehicle stolen

---

### TIER 2: NON-CRITICAL (ESP32/Arduino OK)

**User Interface:**
- ⚪ AC Display control
- ⚪ Aftermarket gauge displays
- ⚪ OLED/LCD screens
- ⚪ Button inputs (except emergency stop)

**Convenience Features:**
- ⚪ Speed-sensitive wipers
- ⚪ Auto headlights
- ⚪ Interior lighting
- ⚪ Seat heaters

**Telemetry/Logging:**
- ⚪ WiFi data logging
- ⚪ Bluetooth OBD-II
- ⚪ Web interface
- ⚪ Mobile app connectivity

**Why Non-Critical:**
- Failure doesn't affect vehicle safety
- Driver can still control vehicle
- No risk of injury
- Can be rebooted without danger

---

## Migration Code Example

### Arduino Leonardo (Current):
```cpp
#include <Arduino.h>
#include <mcp_can.h>

MCP_CAN CAN0(17);  // External MCP2515

void setup() {
  CAN0.begin(CAN_500KBPS);  // External controller
}
```

### STM32F4 (Proposed):
```cpp
#include <Arduino.h>
#include <STM32_CAN.h>  // Built-in CAN!

STM32_CAN CAN(CAN1, ALT);  // Use built-in CAN1 peripheral

void setup() {
  CAN.begin();
  CAN.setBaudRate(500000);  // 500 kbps
}
```

**Benefits:**
- ✅ No external MCP2515 (one less failure point)
- ✅ Built-in CAN controller (more reliable)
- ✅ Hardware filtering (reduces CPU load)
- ✅ Dual CAN buses (can monitor + control)

---

## Cost Analysis

| Solution | Dev Board | Production Chip | CAN Transceiver | Total |
|----------|-----------|-----------------|-----------------|-------|
| **Arduino Leonardo** | $20 | $5 (ATmega32U4) | $2 (MCP2515) | **$27** |
| **STM32F407 (Industrial)** | $15 | $6 | Included | **$21** |
| **STM32F407 (Automotive)** | $30 | $10 | Included | **$40** |
| **TI C2000** | $30 | $8 | Included | **$38** |
| **NXP S32K** | $50 | $12 | Included | **$62** |
| **Infineon AURIX** | $150 | $20 | Included | **$170** |

**Recommendation**: Start with STM32F407 industrial ($21) for testing, then upgrade to automotive-grade chip ($40) for production.

---

## Safety Improvements

| Feature | Arduino Leonardo | STM32F407 | NXP S32K |
|---------|------------------|-----------|----------|
| **Watchdog** | Software only | Hardware | Multiple independent |
| **Temp Range** | 0-85°C 🔴 | -40-85°C 🟡 | -40-150°C ✅ |
| **CAN Controller** | External (MCP2515) | Built-in 2x | Built-in 3x CAN-FD |
| **EMI Protection** | Minimal 🔴 | Better 🟡 | Automotive ✅ |
| **Debugging** | Serial print | SWD debugger | JTAG + trace |
| **Safety Cert** | None 🔴 | None 🟡 | ISO 26262 ✅ |
| **Flash Endurance** | 10K cycles | 10K cycles | 100K cycles |
| **Brown-out** | No 🔴 | Yes ✅ | Yes + monitor ✅ |
| **Error Detection** | No 🔴 | Basic | Lockstep cores ✅ |

---

## Recommended Timeline

### Conservative Path (STM32):
```
Week 1-2: Port code to STM32F407 devkit
Week 3: Bench testing with CAN analyzer
Week 4: Vehicle testing (supervised)
Week 5-6: Reliability testing
Week 7: Upgrade to automotive-grade chip
Week 8: Final validation

Total: 2 months
```

### Aggressive Path (Keep Arduino for now):
```
Immediate: Continue using Arduino Leonardo
Parallel: Develop STM32 version
Testing: Side-by-side comparison
Switch: When STM32 version validated

Total: Use Arduino until STM32 ready
```

---

## Conclusion

**Current Status**: 🔴 **UNSAFE** - Consumer-grade Arduino controlling critical engine functions

**Recommended Action**: ✅ **MIGRATE TO AUTOMOTIVE MCU**

**Best Option for Most Users**: **STM32F407** (industrial → automotive upgrade path)
- Start with $15 devkit
- Port code using Arduino framework
- Validate thoroughly
- Upgrade to automotive chip for production

**Best Option for Professionals**: **NXP S32K144**
- True automotive MCU
- ISO 26262 compliant
- Worth the $50 investment for safety

**For Non-Critical Functions**: **ESP32** (as planned in Phase 1)
- WiFi/Bluetooth built-in
- Great for displays, telemetry, UI
- Cheap ($8)
- Non-safety-critical OK

---

## Next Steps

1. **Immediate**: Read this document, choose MCU platform
2. **Week 1**: Order dev board (STM32F407 recommended)
3. **Week 2**: Port existing code to STM32
4. **Week 3**: Bench test with CAN analyzer
5. **Week 4**: Vehicle test (supervised)
6. **Week 5**: Refine and harden
7. **Week 6**: Production deployment OR upgrade to automotive chip

---

*Document Version: 1.0*
*Last Updated: 2025-11-15*
*Priority: HIGH - Safety Critical*
