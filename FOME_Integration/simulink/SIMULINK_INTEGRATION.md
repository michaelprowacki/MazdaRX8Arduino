# Simulink Integration for FOME ECU

Model-based development workflow using MATLAB/Simulink for ECU control strategy development.

## Overview

This integration enables:
- Graphical control strategy development
- Automatic code generation for STM32
- Hardware-in-the-loop (HIL) simulation
- XCP-based calibration with generated A2L files

## Prerequisites

### Software
- MATLAB R2020b or later
- Simulink
- Embedded Coder
- Stateflow (for state machines)
- Motor Control Blockset (optional)
- Simscape (for plant models)

### Hardware
- STM32F4/H7 target board
- ST-LINK debugger
- CAN interface for XCP

## Development Workflow

### 1. Model Architecture

```
┌─────────────────────────────────────────────────┐
│              FOME_ECU_Model.slx                 │
│                                                 │
│  ┌─────────────┐    ┌─────────────────────┐    │
│  │   Inputs    │    │   Control Strategy  │    │
│  │             │    │                     │    │
│  │ - RPM       │───►│ - Fuel Control      │    │
│  │ - MAP       │    │ - Ignition Timing   │    │
│  │ - TPS       │    │ - Boost Control     │    │
│  │ - AFR       │    │ - Traction Control  │    │
│  │ - Temps     │    │ - Launch Control    │    │
│  └─────────────┘    └──────────┬──────────┘    │
│                                │                │
│                     ┌──────────▼──────────┐    │
│                     │      Outputs        │    │
│                     │                     │    │
│                     │ - Injector PW       │    │
│                     │ - Ignition Angle    │    │
│                     │ - Wastegate Duty    │    │
│                     │ - Warning Flags     │    │
│                     └─────────────────────┘    │
└─────────────────────────────────────────────────┘
```

### 2. Create Plant Model (Optional)

For simulation, create a simplified engine model:

```matlab
% Engine plant model parameters
engine.cylinders = 2;           % Rotary equivalent
engine.displacement = 1.3;      % Liters
engine.compression = 10.0;
engine.inertia = 0.15;          % kg*m^2

% Create transfer functions for dynamics
engine.intake_tf = tf(1, [0.05 1]);   % Intake manifold
engine.exhaust_tf = tf(1, [0.02 1]);  % Exhaust dynamics
```

### 3. Implement Control Strategy

#### Fuel Control Subsystem

```matlab
% In Simulink, create subsystem "FuelControl"
% Inputs: RPM, MAP, TPS, CLT, IAT, AFR_target, AFR_measured
% Outputs: InjectorPW, STFT, LTFT

% Base fuel calculation
VE = lookup2D(RPM, MAP, VE_table);
fuel_mass = (MAP * VE * displacement) / (R * IAT);
base_pw = fuel_mass / injector_flow_rate;

% Closed-loop correction
afr_error = AFR_target - AFR_measured;
STFT = PI_controller(afr_error);
corrected_pw = base_pw * (1 + STFT/100);
```

#### Ignition Timing Subsystem

```matlab
% In Simulink, create subsystem "IgnitionControl"
% Inputs: RPM, MAP, CLT, IAT, knock_level
% Outputs: ignition_angle, knock_retard

% Base timing from table
base_timing = lookup2D(RPM, MAP, timing_table);

% Temperature corrections
clt_correction = lookup1D(CLT, clt_timing_table);
iat_correction = lookup1D(IAT, iat_timing_table);

% Knock retard
if knock_level > threshold
    knock_retard = min(knock_retard + 1, max_retard);
else
    knock_retard = max(knock_retard - 0.1, 0);
end

ignition_angle = base_timing + clt_correction + iat_correction - knock_retard;
```

#### Traction Control Subsystem

```matlab
% In Simulink, create Stateflow chart "TractionControl"
% Inputs: wheel_speeds[4], vehicle_speed, throttle
% Outputs: throttle_cut, timing_retard, fuel_cut

slip_ratio = (drive_wheel_speed - vehicle_speed) / vehicle_speed;

if slip_ratio > slip_target
    % Progressive intervention
    intervention = (slip_ratio - slip_target) * gain;
    throttle_cut = min(intervention * 50, max_throttle_cut);
    timing_retard = min(intervention * 10, max_timing_retard);
end
```

### 4. Configure Code Generation

#### Create STM32 Target Configuration

```matlab
% Model Configuration Parameters
set_param('FOME_ECU_Model', 'SystemTargetFile', 'ert.tlc');
set_param('FOME_ECU_Model', 'TargetLang', 'C');
set_param('FOME_ECU_Model', 'GenerateMakefile', 'on');

% Hardware Implementation
set_param('FOME_ECU_Model', 'ProdHWDeviceType', 'ARM Compatible->ARM Cortex-M');
set_param('FOME_ECU_Model', 'ProdBitPerChar', 8);
set_param('FOME_ECU_Model', 'ProdBitPerShort', 16);
set_param('FOME_ECU_Model', 'ProdBitPerInt', 32);
set_param('FOME_ECU_Model', 'ProdBitPerLong', 32);
set_param('FOME_ECU_Model', 'ProdWordSize', 32);

% Code Style
set_param('FOME_ECU_Model', 'InlinedParameterPlacement', 'NonHierarchical');
set_param('FOME_ECU_Model', 'ParenthesesLevel', 'Nominal');
```

#### XCP/A2L Configuration

```matlab
% Enable ASAP2 file generation
set_param('FOME_ECU_Model', 'GenerateASAP2', 'on');
set_param('FOME_ECU_Model', 'ASAP2FileName', 'FOME_ECU.a2l');

% Configure XCP integration
set_param('FOME_ECU_Model', 'XCPOnCAN', 'on');
```

### 5. Define Calibration Parameters

Use Simulink.Parameter objects for tunable values:

```matlab
% Create calibratable parameters
VE_table = Simulink.Parameter;
VE_table.Value = zeros(16, 16);  % 16x16 table
VE_table.DataType = 'single';
VE_table.CoderInfo.StorageClass = 'ExportedGlobal';

timing_table = Simulink.Parameter;
timing_table.Value = zeros(16, 16);
timing_table.DataType = 'single';
timing_table.CoderInfo.StorageClass = 'ExportedGlobal';

idle_rpm_target = Simulink.Parameter;
idle_rpm_target.Value = 850;
idle_rpm_target.DataType = 'uint16';
idle_rpm_target.CoderInfo.StorageClass = 'ExportedGlobal';
idle_rpm_target.Min = 500;
idle_rpm_target.Max = 2000;
idle_rpm_target.Unit = 'rpm';
```

### 6. Generate Code

```matlab
% Build model
slbuild('FOME_ECU_Model');

% Generated files in FOME_ECU_Model_ert_rtw/:
%   - FOME_ECU_Model.c      (main algorithm)
%   - FOME_ECU_Model.h      (header)
%   - FOME_ECU_Model_data.c (calibration data)
%   - FOME_ECU.a2l          (XCP description)
```

### 7. Integrate with FOME

Copy generated code into FOME project:

```cpp
// In FOME main loop
#include "FOME_ECU_Model.h"

void engineControl_init() {
    FOME_ECU_Model_initialize();
}

void engineControl_update() {
    // Set inputs from sensors
    FOME_ECU_Model_U.RPM = getRPM();
    FOME_ECU_Model_U.MAP = getMAP();
    FOME_ECU_Model_U.TPS = getTPS();
    FOME_ECU_Model_U.CLT = getCoolantTemp();
    FOME_ECU_Model_U.AFR = getAFR();

    // Execute model step
    FOME_ECU_Model_step();

    // Get outputs
    setInjectorPulseWidth(FOME_ECU_Model_Y.InjectorPW);
    setIgnitionTiming(FOME_ECU_Model_Y.IgnitionAngle);
    setWastegateDuty(FOME_ECU_Model_Y.WastegateDuty);
}
```

## Hardware-in-the-Loop (HIL) Testing

### Setup

1. Connect CAN interface to Simulink PC
2. Configure XCP master in Simulink
3. Run model in external mode

### External Mode Configuration

```matlab
set_param('FOME_ECU_Model', 'SimulationMode', 'external');
set_param('FOME_ECU_Model', 'ExtModeTrigType', 'manual');
```

### Real-Time Tuning

1. Start external mode simulation
2. Modify parameters in Simulink
3. Changes download to ECU via XCP
4. Monitor signals in real-time scopes

## Example Models

### Closed-Loop Fuel Control

See `examples/closed_loop_fuel.slx`:
- Wideband O2 feedback
- Short-term fuel trim (±25%)
- Long-term learning (per-cell)
- Decel fuel cut

### Launch Control

See `examples/launch_control.slx`:
- Two-step rev limiter
- Clutch switch input
- Adjustable launch RPM
- Anti-lag option

### Traction Control

See `examples/traction_control.slx`:
- 4-wheel speed inputs
- Slip ratio calculation
- Progressive intervention
- Gear-based targets

## XCP Calibration Workflow

### 1. Load A2L in CANape

```
File → Open → FOME_ECU.a2l
```

### 2. Create Measurement Configuration

- Add RPM, MAP, AFR, timing to measurement list
- Set sampling rate (10ms typical)
- Configure display (oscilloscope, digital)

### 3. Online Calibration

1. Connect to ECU via CAN
2. Start measurement
3. Modify calibration values
4. Changes take effect immediately
5. Save calibration to file

### 4. Flash Calibration

```
Calibration → Download → Flash
```

## Troubleshooting

### Code Generation Fails

- Check all blocks have supported code generation
- Verify no unsupported MATLAB functions
- Check memory allocation settings

### XCP Connection Issues

- Verify CAN IDs match (default 0x555 TX, 0x554 RX)
- Check baud rate (500kbps)
- Ensure XCP slave initialized

### A2L Address Mismatch

- Rebuild after any code changes
- Verify linker script memory sections
- Check ASAP2 address assignment

## File Structure

```
FOME_Integration/simulink/
├── SIMULINK_INTEGRATION.md     # This file
├── models/
│   ├── FOME_ECU_Model.slx      # Main ECU model
│   ├── FuelControl.slx         # Fuel subsystem
│   ├── IgnitionControl.slx     # Ignition subsystem
│   └── TractionControl.slx     # Traction subsystem
├── examples/
│   ├── closed_loop_fuel.slx
│   ├── launch_control.slx
│   └── traction_control.slx
├── data/
│   ├── engine_params.m         # Engine parameters
│   ├── calibration_default.m   # Default cal values
│   └── vehicle_params.m        # Vehicle config
└── generated/                  # Auto-generated code
```

## Next Steps

1. Create basic FOME_ECU_Model.slx structure
2. Implement fuel control algorithm
3. Add ignition timing control
4. Configure code generation for STM32H7
5. Test XCP calibration with CANape/INCA
6. Validate against rusEFI behavior
