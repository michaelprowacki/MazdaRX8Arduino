# FOME Integration

Advanced features for integrating body electronics, professional calibration tools, and model-based development with FOME ECU.

## Overview

This module adds capabilities that make FOME competitive with commercial ECUs:

| Feature | Description | Status |
|---------|-------------|--------|
| **Vehicle Profiles** | Body electronics for different vehicles | ✅ Complete |
| **XCP Protocol** | Professional calibration tool support | ✅ Complete |
| **XCP Flash Programming** | ECU reflashing via XCP | ✅ Complete |
| **A2L Generator** | ASAM MCD-2MC description files | ✅ Complete |
| **Simulink Integration** | Model-based development workflow | ✅ Documented |
| **Platform Abstraction** | Desktop, Arduino, STM32 support | ✅ Complete |

## Quick Start

### 1. Basic Integration

```cpp
#include "vehicle_profiles/vehicle_profile.h"
#include "xcp/xcp_protocol.h"
#include "platform/platform_abstraction.h"

// Create vehicle profile
VehicleProfile* profile = createVehicleProfile(VEHICLE_MAZDA_RX8);
profile->init();

// In your main loop (every 100ms)
profile->setRPM(engineRPM);
profile->setVehicleSpeed(speed);
profile->setCoolantTemp(temp);
profile->sendDashboardMessages();
```

### 2. Generate A2L File

```bash
cd tools
python a2l_generator.py --standard --output fome_ecu.a2l
```

### 3. Run Tests

```bash
cd tests
make test
```

## Directory Structure

```
FOME_Integration/
├── README.md                    # This file
├── vehicle_profiles/            # Vehicle-specific body electronics
│   ├── vehicle_profile.h        # Base class interface
│   ├── rx8_profile.cpp          # Mazda RX8 implementation
│   └── generic_profile.cpp      # OBD-II generic profile
├── xcp/                         # XCP calibration protocol
│   ├── xcp_protocol.h           # XCP slave definitions
│   └── xcp_protocol.cpp         # XCP implementation
├── platform/                    # Platform abstraction layer
│   ├── platform_abstraction.h   # Unified API
│   ├── platform_arduino.cpp     # Arduino Leonardo/Mega
│   ├── platform_stm32.cpp       # STM32 with ChibiOS
│   └── platform_desktop.cpp     # Desktop testing
├── simulink/                    # Model-based development
│   └── SIMULINK_INTEGRATION.md  # Workflow documentation
├── tools/                       # Utilities
│   ├── a2l_generator.py         # A2L file generator
│   ├── rx8_variables.json       # Example configuration
│   └── tunerstudio_config.ini   # TunerStudio setup
├── examples/                    # Integration examples
│   └── fome_integration_example.cpp
└── tests/                       # Unit tests
    ├── test_xcp_protocol.cpp    # 11 tests
    ├── test_can_encoding.cpp    # 17 tests
    ├── test_xcp_flash.cpp       # 21 tests
    ├── test_a2l_generator.py    # 20 tests
    └── Makefile
```

## Vehicle Profiles

### Supported Vehicles

| Vehicle | Profile | CAN Messages | Features |
|---------|---------|--------------|----------|
| Mazda RX8 S1 | `rx8_profile.cpp` | 0x201, 0x420, 0x620, etc. | Full body electronics |
| Generic | `generic_profile.cpp` | OBD-II standard | Basic compatibility |

### RX8 Profile Features

- **Dashboard**: RPM, speed, temperature, warning lights
- **ABS/DSC**: System status, wheel speeds
- **Immobilizer**: Two-part handshake bypass
- **Power Steering**: Enabled via RPM signal
- **Odometer**: 4140 increments per mile

### Adding a New Vehicle

1. Create `vehicle_profiles/your_vehicle_profile.cpp`
2. Inherit from `VehicleProfile` base class
3. Implement required virtual methods
4. Register in factory function

```cpp
class MyVehicleProfile : public VehicleProfile {
public:
    void init() override { /* ... */ }
    void sendDashboardMessages() override { /* ... */ }
    void handleCanRx(uint32_t id, uint8_t* data, uint8_t len) override { /* ... */ }
};
```

## XCP Protocol

### Supported Commands

| Command | Code | Description |
|---------|------|-------------|
| CONNECT | 0xFF | Establish connection |
| DISCONNECT | 0xFE | End session |
| GET_STATUS | 0xFD | Query slave status |
| SET_MTA | 0xF6 | Set memory address |
| UPLOAD | 0xF5 | Read from ECU |
| SHORT_UPLOAD | 0xF4 | Read with address |
| DOWNLOAD | 0xF0 | Write to ECU |
| DAQ commands | 0xD3-0xE2 | Data acquisition |

### Compatible Tools

- ETAS INCA
- Vector CANape
- ATI Vision
- Other ASAP2/XCP tools

### Configuration

Default CAN IDs:
- **RX**: 0x554
- **TX**: 0x555

## A2L File Generation

### Standard ECU Variables

```bash
python a2l_generator.py --standard --output fome.a2l
```

Includes:
- Engine RPM, temps, throttle
- Fuel trim (STFT/LTFT)
- Knock retard
- Boost control
- Calibration parameters

### Custom Variables

Create JSON config:
```json
{
  "measurements": [
    {
      "name": "myVariable",
      "address": "0x20001000",
      "datatype": "UWORD",
      "unit": "rpm"
    }
  ]
}
```

Generate:
```bash
python a2l_generator.py --config my_vars.json --output custom.a2l
```

## Platform Support

### Arduino

```cpp
#define PLATFORM_ARDUINO
#include "platform/platform_arduino.cpp"
```

Requirements:
- Arduino Leonardo or Mega
- MCP2515 CAN module
- mcp_can library

### STM32

```cpp
#define PLATFORM_STM32
#include "platform/platform_stm32.cpp"
```

Requirements:
- STM32F4/F7/H7
- ChibiOS RTOS
- Built-in CAN peripheral

### Desktop

```cpp
#define PLATFORM_DESKTOP
#include "platform/platform_desktop.cpp"
```

For testing without hardware.

## TunerStudio Integration

Copy settings from `tools/tunerstudio_config.ini` to your .ini file:

```ini
[VehicleProfile]
vehicleType = bits, U08, 0, [0:3], "Generic", "Mazda RX8 S1", ...
enableBodyCAN = bits, U08, 1, [0:0], "No", "Yes"
```

## Testing

### Run All Tests

```bash
cd tests
make test
```

### Individual Suites

```bash
make run-xcp    # XCP protocol (11 tests)
make run-can    # CAN encoding (17 tests)
make run-flash  # XCP flash programming (21 tests)
make run-a2l    # A2L generator (20 tests)
```

### Test Coverage

- **69 total tests**
- XCP Protocol: Connect, disconnect, upload, download, DAQ (11 tests)
- XCP Flash: Program start, clear, write, verify, reset (21 tests)
- CAN: RPM, speed, throttle, warnings encoding (17 tests)
- A2L: File structure, variables, formulas (20 tests)

## Development Roadmap

### Completed

- [x] Vehicle profile system
- [x] RX8 full implementation
- [x] XCP slave protocol
- [x] XCP flash programming
- [x] A2L file generator
- [x] Platform abstraction
- [x] Test suite (69 tests)

### Future Work

- [ ] Miata NC profile
- [ ] BMW E46 profile
- [ ] Simulink code generation target
- [ ] CCP (legacy) support

## Contributing

1. Fork repository
2. Create feature branch
3. Add tests for new code
4. Submit pull request

### Code Style

- C++11 compatible
- 4-space indentation
- Descriptive names
- Comment complex logic

## License

Same as parent MazdaRX8Arduino project.

## References

- [XCP Protocol Specification](https://www.asam.net/standards/detail/mcd-1-xcp/)
- [ASAP2 (A2L) Standard](https://www.asam.net/standards/detail/mcd-2-mc/)
- [FOME Firmware](https://github.com/FOME-Tech/fome-fw)
- [MazdaRX8Arduino](../) - Parent project

## Support

- Issues: GitHub repository
- RX8 questions: RX8Club.com forums
- FOME questions: FOME Discord
