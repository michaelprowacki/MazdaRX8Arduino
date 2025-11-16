# Wipers Module - DEPRECATED

**⚠️ This module has been deprecated and archived.**

## Status

- **Deprecated Date**: 2025-11-15 (Phase 1 consolidation)
- **Reason**: Functionality merged into ECU_Module to reduce hardware complexity
- **Current Location**: `archived/Wipers_Module/` (this folder)
- **New Location**: `core/ECU_Module/RX8_CANBUS.ino`

## What Happened?

The standalone Wipers_Module required a separate Arduino board (typically an ATtiny441/841) connected to the CAN bus. This added:
- Extra hardware cost (~$10-15)
- Additional wiring complexity
- Another CAN bus node to manage
- Duplicate CAN message processing

During Phase 1 consolidation, we realized the speed-sensitive wiper functionality could be integrated into the main ECU module as an **optional feature** using compile-time flags.

## Migration Path

### Old Setup (Deprecated)
```
┌─────────────────┐         ┌─────────────────┐
│  ECU Module     │         │ Wipers Module   │
│  (Arduino Leo)  │◄───CAN──►│ (ATtiny441/841) │
│                 │         │                 │
│ - Engine control│         │ - Read CAN speed│
│ - CAN messages  │         │ - Control wipers│
└─────────────────┘         └─────────────────┘
      2 boards                   Separate firmware
```

### New Setup (Recommended)
```
┌─────────────────────────────┐
│      ECU Module             │
│    (Arduino Leonardo)       │
│                             │
│ - Engine control            │
│ - CAN messages              │
│ - Wipers (optional)         │ ◄── All-in-one!
└─────────────────────────────┘
       1 board
```

### How to Use the New Implementation

**1. Open the ECU module:**
```bash
core/ECU_Module/RX8_CANBUS.ino
```

**2. Enable wipers feature (line 37):**
```cpp
// Uncomment this line:
#define ENABLE_WIPERS
```

**3. Pin Configuration:**
| Pin | Function | Type |
|-----|----------|------|
| 6 | Wiper control output | Digital Out (connects to relay) |
| 4 | Wiper position sense (optional) | Digital In |

**4. Upload to Arduino Leonardo**

**5. Done!** Wipers will now auto-adjust based on vehicle speed:
- 0 mph: 3.0 second delay
- <20 mph: 2.0 second delay
- <40 mph: 1.5 second delay
- <60 mph: 1.0 second delay
- 60+ mph: 0.5 second delay

## Benefits of Migration

✅ **Cost Savings**: $10-15 (one fewer Arduino board)
✅ **Simpler Wiring**: No need for separate CAN connection
✅ **Code Reuse**: Uses existing vehicleSpeed variable (no CAN overhead)
✅ **Easier Maintenance**: One codebase instead of two
✅ **Optional**: Can be disabled if not needed (just keep #define commented out)
✅ **Testing**: Wiper failure doesn't affect engine control

## Why Keep This Folder?

This folder is kept in the `archived/` directory for:

1. **Historical Reference**: Understand the original standalone implementation
2. **Hardware Files**: PCB designs and BOM for those who built the standalone version
3. **Firmware Source**: Original ATtiny441/841 firmware for troubleshooting
4. **Rollback**: If integration causes issues, standalone version still available
5. **Documentation**: Original README and wiring diagrams

## Original Hardware Files

This folder still contains the complete standalone implementation:

```
archived/Wipers_Module/
├── DEPRECATED.md           # This file
├── README.md               # Original documentation
├── firmware/               # ATtiny441/841 firmware
│   ├── main.c             # Original C implementation
│   ├── main.h             # Header file
│   ├── uart.c             # UART driver
│   ├── firmware-rev-03.hex # Compiled firmware
│   └── ...
└── hardware/               # PCB designs
    ├── schematic.pdf      # Circuit schematic
    ├── gerbers/           # PCB manufacturing files
    ├── bom.csv            # Bill of materials
    └── ...
```

## If You Already Built This Module

### Option 1: Switch to Integrated Version (Recommended)
1. Remove Wipers_Module Arduino from vehicle
2. Enable `#define ENABLE_WIPERS` in ECU_Module
3. Reconnect wiper relay to ECU Module pin 6
4. Save $10-15 and simplify wiring

### Option 2: Keep Using Standalone Version
- Original firmware still available in `archived/Wipers_Module/firmware/`
- Hardware files in `archived/Wipers_Module/hardware/`
- You can continue using it, but we won't be updating it

## See Also

- **Core/ECU_Module/README.md** - ECU module documentation (including wipers)
- **CONSOLIDATION_SUMMARY.md** - Full consolidation details
- **STRUCTURE.md** - Repository organization guide
- **CLAUDE.md** - Main project documentation

## Questions?

If you have questions about migration:
1. Read `core/ECU_Module/README.md` (wipers section)
2. Check `CONSOLIDATION_SUMMARY.md` (lines 29-105)
3. Review the code in `core/ECU_Module/RX8_CANBUS.ino` (lines 303-371)

---

*This module was deprecated as part of the Phase 1 consolidation effort to reduce hardware complexity and costs.*

*Last Updated: 2025-11-16*
*Original Author: Basil Hussain (https://github.com/basilhussain/rx8-wipers)*
*Integration: Claude AI Assistant*
