# RX8 Speed-Sensitive Wipers Module

## Overview

This project is an upgrade to the Mazda RX-8's windshield / windscreen wiper system that adds speed-sensitive functionality to the intermittent operation mode.

**Source**: https://github.com/basilhussain/rx8-wipers
**Author**: Basil Hussain
**License**: BSD 3-Clause (see firmware/LICENSE.txt)
**Project Write-up**: https://www.stasisleak.uk/rx8/wipers/

---

## Features

- Speed-sensitive intermittent wiper timing
- Integration with vehicle CAN bus for speed data
- Arduino-based control
- Drop-in replacement for factory wiper relay

See the project write-up at https://www.stasisleak.uk/rx8/wipers/ for complete hardware details, installation instructions, and technical documentation.

---

## Integration with MazdaRX8Arduino

This module has been integrated into the MazdaRX8Arduino repository with:

- **CAN Integration Example**: `wipers_with_CAN_speed.ino` demonstrates reading vehicle speed from CAN bus (0x201) instead of wheel speed sensors
- **Simplified Algorithm**: Speed-based wiper delay calculation (3s at 0mph → 0.5s at 100mph)
- **Compatibility**: Works alongside ECU_Module or EV_ECU_Module

See parent repository README for integration scenarios.

---

## Credits

**Original Project**: Basil Hussain (https://github.com/basilhussain/rx8-wipers)
**License**: BSD 3-Clause
**Integration**: MazdaRX8Arduino project (2025)
