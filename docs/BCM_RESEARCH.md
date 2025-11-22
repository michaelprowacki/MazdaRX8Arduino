# RX8 BCM CAN Bus Research

## BCM Architecture

The RX8 "Keyless Module" is the BCM controlling:
- Central door locking
- Keyless entry
- Immobilizer integration

Main CAN modules: PCM, EBCM, P/S Module, Steering Angle Sensor, Instrument Panel, Keyless Module (BCM)

## Known CAN Messages

Confirmed:
- 0x047: Immobilizer request from BCM
- 0x041: Immobilizer response from ECU
- 0x39E: Door status (Mazdaspeed 3, may differ on RX8)

Unknown (need discovery):
- Door lock/unlock commands
- Trunk release
- Interior lights control

## Discovery Method

Use examples/BCM_Discovery_Sniffer:
1. Baseline capture (all doors closed/locked)
2. Trigger function (unlock via key fob)
3. Filter for NEW or CHANGED messages
4. Replay to verify

Expected CAN ID range: 0x300-0x4FF
Expected discovery time: 1-2 hours per function

## Lock Actuator Hardware

Wire colors: Red/Blue and Red/Black
Control: Direct BCM control

## Sources

- RX8Club.com forums
- Chamber of Understanding blog (Parts 5, 6, 21)
- Community reverse engineering
