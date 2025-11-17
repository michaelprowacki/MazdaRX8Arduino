# Odometer Implementation

The RX8 cluster expects odometer increments on CAN message 0x420, byte 1.

## Formula

4,140 increments per mile (from Chamber of Understanding blog Part 6)

```cpp
unsigned long interval = 869565UL / vehicleSpeed;  // microseconds per increment

if (micros() - lastOdometerUpdate >= interval) {
    odometerByte++;
    odo = odometerByte;
    lastOdometerUpdate = micros();
}
```

## Implementation

See ECU_Module/RX8_CANBUS.ino lines 99-101, 246-269, 289:

```cpp
// Variables (global)
unsigned long lastOdometerUpdate = 0;
byte odometerByte = 0;

// In setDefaults()
odometerByte = 0;
lastOdometerUpdate = micros();

// In sendOnTenth()
updateOdometer();
updateMIL();  // sets send420[1] = odo
```

## Testing

Bench test: Monitor Serial output for increment timing at simulated speeds
Vehicle test: Drive 1 mile, verify cluster increments by ~4,140

## Legal Compliance

49 USC 32703: Only increments, no rollback. Based on actual wheel speed from CAN 0x4B0/0x4B1.

## Calibration

Adjust if needed for tire size variation (±5% tolerance acceptable).
