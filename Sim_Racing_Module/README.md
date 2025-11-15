# RX8-Arduino - Sim Racing Cluster Controller

## Overview

This project allows you to control a Mazda RX8 instrument cluster with PC racing simulators (Forza Horizon 5 or Dirt Rally 2.0) through an Arduino.

**Source**: https://github.com/Izekeal/rx8-arduino (fork of Christian Groleau's original project)
**Original Project**: https://gitlab.com/christiangroleau/rx8-arduino/ by Christian Groleau
**Fork Author**: Izekeal
**License**: MIT (see LICENSE file)

---

## Features

- Drive real RX8 instrument cluster from PC games
- Support for Forza Horizon 5
- Support for Dirt Rally 2.0
- Handbrake control via momentary button (added by Izekeal)
- Future: RPM limiter beeper (upshift warning)
- Future: Dynamic game switching button

---

## Modifications from Original

This fork adds:
- Handbrake controlled via a momentary button
- Integration into MazdaRX8Arduino repository

Planned additions:
- RPM limiter beeper control (upshift warning)
- Button to dynamically switch between supported games

---

## Getting Started

For details about hardware requirements, wiring, and setup, see the original project:
- **Fork**: https://github.com/Izekeal/rx8-arduino
- **Original**: https://gitlab.com/christiangroleau/rx8-arduino/

---

## Integration with MazdaRX8Arduino

This module has been integrated into the MazdaRX8Arduino repository for:

- **Standalone Operation**: No vehicle CAN bus required
- **PC Connection**: Serial communication with racing simulator
- **Bench Testing**: Use spare RX8 cluster on desk for sim racing

**Use Case**: Perfect for using a spare instrument cluster as a PC gaming peripheral.

---

## Hardware Requirements

- Arduino (compatible with original project specifications)
- Mazda RX8 instrument cluster (Series 1)
- PC running Forza Horizon 5 or Dirt Rally 2.0
- USB cable for Arduino-PC connection
- Power supply for cluster (12V)

See original project documentation for complete wiring diagrams and setup instructions.

---

## Credits

**Original Project**: Christian Groleau (https://gitlab.com/christiangroleau/rx8-arduino/)
**Fork**: Izekeal (https://github.com/Izekeal/rx8-arduino)
**License**: MIT
**Integration**: MazdaRX8Arduino project (2025)
