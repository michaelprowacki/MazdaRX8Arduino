# ECU Module - Internal Combustion Engine

## Overview

This is the **primary ECU/PCM replacement** for Mazda RX8 with internal combustion engines (gasoline). It replaces the factory ECU entirely while maintaining all vehicle systems.

**Use Case**: Engine swaps (rotary, LS, JZ, 2JZ, K-series, etc.) or standalone engine management

---

## Hardware

- **Microcontroller**: Arduino Leonardo (ATmega32U4)
- **CAN Controller**: MCP2515
- **CAN Speed**: 500 kbps
- **Power**: 12V vehicle power (regulated to 5V)

---

## Features

✅ Complete CAN bus emulation
✅ Dashboard functionality (RPM, speed, temperature)
✅ Warning light control
✅ ABS/DSC integration
✅ Immobilizer bypass
✅ Power steering enabled
✅ Throttle pedal processing
✅ Wheel speed monitoring
✅ **Speed-sensitive wipers** (optional, enable with `#define ENABLE_WIPERS`)

---

## Quick Start

1. **Install Arduino IDE** or PlatformIO
2. **Install MCP_CAN library**
3. **Open** `RX8_CANBUS.ino`
4. **Select Board**: Arduino Leonardo
5. **Upload** to Arduino

---

## Optional Features

### Speed-Sensitive Wipers (CONSOLIDATED from Wipers_Module)

This module now includes integrated speed-sensitive wiper control. Enable by uncommenting `#define ENABLE_WIPERS` at the top of `RX8_CANBUS.ino`.

**How it works:**
- Reads vehicle speed from wheel sensors (already calculated by ECU)
- Adjusts wiper delay automatically:
  - 0 mph: 3.0 second delay
  - <20 mph: 2.0 second delay (city)
  - <40 mph: 1.5 second delay (suburban)
  - <60 mph: 1.0 second delay (highway)
  - 60+ mph: 0.5 second delay (high speed)

**Wiring:**
- `WIPER_CONTROL_PIN` (default: pin 6) → Wiper relay control
- `WIPER_SENSE_PIN` (default: pin 4) → Optional position feedback

**Benefits of consolidation:**
- Single Arduino board instead of two
- Uses existing vehicle speed calculation
- No additional CAN bus overhead
- Simpler wiring and installation

---

## Documentation

See main **CLAUDE.md** for complete documentation including:
- Pin configuration
- CAN message details
- Safety warnings
- Modification guidelines

---

## Related Modules

- **EV_ECU_Module** - Electric vehicle version
- **AC_Display_Module** - Factory AC display control
- **Dash_Controller_Module** - Alternative dashboard implementation

---

*For complete documentation, see main repository README and CLAUDE.md*
