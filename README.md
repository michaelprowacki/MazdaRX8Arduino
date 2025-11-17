# Mazda RX8 Arduino Electronics

Arduino-based ECU replacement and electronics control for the Mazda Series 1 RX8 (2004-2008).

## What This Does

Replace the factory ECU while maintaining dashboard, ABS, power steering, and immobilizer functionality through CAN bus emulation.

## Primary Modules

**ECU_Module** - ICE engine swap ECU replacement
**EV_ECU_Module** - Electric vehicle conversion ECU
**PDM_Module** - Power distribution (8 channels, 30A each, $230 DIY vs $1200-5500 commercial)
**AC_Display_Module** - Factory AC display controller
**Aftermarket_Display_Module** - OLED displays (15+ parameters via CAN)
**Coolant_Monitor_Module** - Accurate temp/pressure monitoring
**Wipers_Module** - Speed-sensitive wiper control
**Sim_Racing_Module** - Drive real cluster from Forza/Dirt Rally

## Quick Start

1. Choose your module (see above)
2. Read the module README for hardware requirements
3. Upload firmware to Arduino
4. Bench test before vehicle installation
5. See Documentation/INTEGRATION_GUIDE.md for multi-module setups

## Hardware Requirements

**Minimum (ECU replacement)**:
- Arduino Leonardo (ECU_Module) or Nano (EV_ECU_Module)
- MCP2515 CAN controller
- 12V vehicle power with 5V regulator

**For PDM**:
- Arduino Mega 2560
- 8x IRLB8721 MOSFETs
- 8x ACS712-30A current sensors
- Total cost: $230

## CAN Bus Protocol

High-speed CAN at 500 kbps. Key messages:

| ID (Hex) | Purpose | Rate |
|----------|---------|------|
| 0x201 | PCM Status (RPM, Speed, Throttle) | 100ms |
| 0x420 | Warning Lights, Engine Temp | 100ms |
| 0x4B0/0x4B1 | Wheel Speeds | 10ms |
| 0x047/0x041 | Immobilizer Handshake | On request |
| 0x600-0x603 | PDM Control/Status | 10-100ms |

Complete protocol: Documentation/CAN_PID_Reference.md
DBC file: Documentation/rx8_can_database.dbc

## Code Library

Shared CAN message library in `lib/RX8_CAN_Messages.h`:

```cpp
#include "RX8_CAN_Messages.h"
RX8_CAN_Decoder decoder;

decoder.decode0x201(canBuffer);
int rpm = decoder.pcmStatus.engineRPM;
int speed = decoder.pcmStatus.vehicleSpeed;
```

Eliminates duplicate code across modules. See lib/README.md for API documentation.

## Documentation

**docs/ODOMETER_IMPLEMENTATION_GUIDE.md** - Odometer increment implementation (4,140 increments/mile)
**docs/PDM_INTEGRATION_GUIDE.md** - Power distribution module design guide
**docs/RX8_BCM_CANBUS_RESEARCH.md** - Body control module CAN research
**Documentation/CAN_PID_Reference.md** - Complete CAN protocol reference
**Documentation/INTEGRATION_GUIDE.md** - Multi-module integration scenarios
**CLAUDE.md** - Comprehensive development guide for AI assistants

## Safety Warning

This code controls safety-critical vehicle systems (throttle, ABS, warning lights).

- Bench test all modifications before vehicle installation
- Do not modify throttle safety limits without understanding consequences
- Incorrect CAN messages can disable ABS/DSC stability control
- Keep factory ECU for restoration if needed

Use at your own risk. Authors not responsible for vehicle damage or injury.

## Integration Scenarios

**Minimal**: ECU_Module only - Basic dashboard and systems
**Full Monitoring**: ECU + AC Display + Coolant Monitor + Aftermarket Displays
**EV Conversion**: EV_ECU_Module + AC Display + Aftermarket Displays
**Engine Swap with PDM**: ECU_Module + PDM_Module (complete power control)
**Track Day**: ECU + PDM + ESP8266 (telemetry logging)

## Contributing

Test thoroughly. Document CAN changes. Follow existing code style.

This repository integrates work from 15+ RX8 Arduino projects and hundreds of hours of reverse engineering.

## Attribution

**David Blackhurst** - ECU_Module (paypal.me/DBlackhurst)
**EV8-Steve** - EV_ECU_Module
**michaelprowacki** - AC_Display_Module, ESP8266_Companion, PDM_Module, consolidation
**Radivv** - Aftermarket_Display_Module
**topolittle** - Coolant_Monitor_Module
**basilhussain** - Wipers_Module
**Izekeal** - Sim_Racing_Module
**equinox311** - PCM_Analysis tools
**Rx8Man** - ECU_Definitions
**rnd-ash** - rx8_can_database.dbc

See individual module READMEs for licenses and detailed attribution.

## License

Code from multiple sources with varying licenses (BSD, MIT, GPL). Check each module's LICENSE file.

---

**Use at your own risk. Safety-critical vehicle systems.**

Repository: https://github.com/michaelprowacki/MazdaRX8Arduino
Series 1 RX8 (2004-2008) only. Series 2 not supported.
