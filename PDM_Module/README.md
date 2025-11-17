# Power Distribution Module (PDM)

Electronic power distribution system for RX8 engine swaps and EV conversions.

**IMPORTANT**: This is V1 (proof-of-concept). See PDM_V2_COMPACT_DESIGN.md for proper compact design with true electronic circuit breakers.

## Features

**Engine Swap**: Fuel pump, progressive fans (50-70-100%), water pump, A/C, lights
**EV Conversion**: BMS, motor controller, coolant pumps, DC-DC converter, charging
**Race Car**: Aggressive cooling, data logging, telemetry, shift lights
**Advanced**: Soft-start (500ms ramp), load shedding, kill switch, overcurrent protection, CAN failsafe

## Hardware

**Required**:
- Arduino Mega 2560 ($18)
- MCP2515 CAN module ($8)
- 8x MOSFET modules (IRLB8721, 30A, $7 each)
- 8x Current sensors (ACS712-30A, $4 each)
- 8x Inline fuses ($2 each)
- Enclosure, wire, connectors ($55)

**Total**: $230 (vs $1,200-5,500 commercial PDMs)

**Limitations**: V1 still requires inline fuses (not truly fuse-less), bulky (8" x 6"), no auto-retry circuit breakers. See PDM_V2_COMPACT_DESIGN.md for proper compact design ($177-227, 4" x 6", true electronic circuit breakers, no fuses).

## Quick Start

**1. Select Build Configuration**

Edit PDM_Module.ino, uncomment one:
```cpp
#define BUILD_ENGINE_SWAP
#define BUILD_EV_CONVERSION
#define BUILD_RACE_CAR
#define BUILD_STOCK_ROTARY
```

**2. Upload**
- Arduino IDE: Tools → Board → Mega 2560, Upload
- PlatformIO: `pio run -t upload`

**3. ECU Integration**

Add to ECU code:
```cpp
#include "ECU_PDM_Integration.h"

void setup() {
    pdm_init(CAN0);
}

void loop() {
    pdm_updateEngineSwapControl(rpm, coolantTemp, acRequest);
    pdm_sendControl(CAN0);
    pdm_receiveStatus(canID, canBuffer);
}
```

## Channel Configuration

**Engine Swap (BUILD_ENGINE_SWAP)**:
- CH1: Fuel Pump (20A, CRITICAL)
- CH2: Radiator Fan 1 (25A, ESSENTIAL)
- CH3: Radiator Fan 2 (25A, ESSENTIAL)
- CH4: Water Pump (15A, ESSENTIAL)
- CH5: A/C Clutch (10A, COMFORT)
- CH6: Trans Cooler Fan (15A, ESSENTIAL)
- CH7: Aux Lights (10A, AUXILIARY)
- CH8: Accessory (10A, AUXILIARY)

**EV Conversion (BUILD_EV_CONVERSION)**:
- CH1: BMS (10A, CRITICAL)
- CH2: Motor Controller (15A, CRITICAL)
- CH3: Motor Coolant Pump (15A, ESSENTIAL)
- CH4: Battery Coolant Pump (15A, ESSENTIAL)
- CH5: DC-DC Converter (20A, ESSENTIAL)
- CH6: Charging System (20A, COMFORT)
- CH7: Cabin Heater (20A, COMFORT)
- CH8: Accessory (10A, AUXILIARY)

**Race Car (BUILD_RACE_CAR)**:
- CH1: Fuel Pump (20A, CRITICAL)
- CH2: Radiator Fan 1 (25A, CRITICAL, always 100%)
- CH3: Radiator Fan 2 (25A, CRITICAL, always 100%)
- CH4: Brake Cooling (15A, ESSENTIAL)
- CH5: Data Logger (5A, ESSENTIAL)
- CH6: Shift Light (5A, COMFORT)
- CH7: Aux Lights (10A, AUXILIARY)
- CH8: Camera (5A, AUXILIARY)

## CAN Protocol

**ECU → PDM (0x600)** - Control message (100ms):
- Byte 0: Enable bits (CH1-CH8)
- Byte 1-4: PWM values (0-255)

**PDM → ECU (0x601)** - Status message (100ms):
- Byte 0: Channel status bits
- Byte 1-4: Current readings
- Byte 5: Fault flags
- Byte 6-7: Total current (0.1A resolution)

**ECU → PDM (0x602)** - Commands (on demand):
- Reset faults, emergency stop, enable/disable load shedding

**PDM → ECU (0x603)** - Telemetry (1s):
- Uptime, battery voltage, energy consumption

## Serial Commands

```
ENABLE <ch>      - Enable channel
DISABLE <ch>     - Disable channel
PWM <ch> <val>   - Set PWM (0-255)
STATUS           - Print all channels
RESET            - Clear faults
LOG              - Toggle CSV logging
EMERGENCY        - Emergency stop
```

## Testing

**Bench Test**:
1. Connect to 12V bench power supply
2. Connect test loads (light bulbs, 5-10W each)
3. Upload firmware, open Serial Monitor (115200)
4. Command: `STATUS` - verify all channels
5. Test channels: `ENABLE 1`, `PWM 1 128`, monitor current
6. Test kill switch (pin 2 to GND, all channels OFF)

**Vehicle Test**:
1. Install PDM in engine bay (waterproof enclosure)
2. Wire channels to loads (fuel pump, fans, etc.)
3. Connect CAN to ECU (CAN_H pin 6, CAN_L pin 14 on OBD2)
4. Test progressive fans (drive until temp rises)
5. Test load shedding (disconnect alternator, voltage drops)
6. Monitor telemetry via Serial or CAN

## Safety

**Overcurrent Protection**: Each channel has current limit. Exceeding limit = channel disabled + fault flag set.

**Load Shedding**: On low voltage (<11.5V), disables COMFORT and AUXILIARY channels, keeps CRITICAL.

**Kill Switch**: Pin 2 to GND = instant shutdown all channels (<1ms), safety override.

**CAN Failsafe**: If ECU stops sending (1s timeout), disables all non-critical channels.

**Watchdog Timer**: Code freeze = automatic reset.

## Troubleshooting

**Channel won't enable**: Check current limit, verify MOSFET wiring, check fuse
**High current reading**: Verify load draw, check current sensor calibration
**CAN errors**: Check wiring (CANH pin 6, CANL pin 14), verify 120 ohm termination
**Voltage sag on start**: Enable soft-start, increase ramp time (default 500ms)
**Load shedding activating**: Check battery/alternator, adjust threshold (default 11.5V)

## Advanced Configuration

Edit PDM_Module.ino:

```cpp
#define BATTERY_CRITICAL 11.0    // Load shedding threshold
#define SOFTSTART_RAMP_TIME 500  // ms
#define CAN_TIMEOUT 1000         // ms
#define LOG_INTERVAL 1000        // ms
```

Per-channel current limits:
```cpp
ch->currentLimit = 25.0;  // Amps
```

## Cost Comparison

**DIY PDM**: $230
**AEM PDU-8**: $700 (requires VCU, total $1,200+)
**Haltech PD16**: $1,099 (requires Elite ECU, total $2,600)
**MoTeC PDM30**: $2,500+ (requires M1 ECU, total $5,500+)

**Savings**: 81-96% vs commercial, and ours is standalone (no expensive ECU required).

## Files

**PDM_Module.ino** (1,040 lines) - Main firmware
**ECU_PDM_Integration.h** (645 lines) - ECU integration library (copy to ECU_Module)

## License

MIT License. Use at your own risk for automotive applications.

---

**Safety-critical power control. Test thoroughly before vehicle use.**
