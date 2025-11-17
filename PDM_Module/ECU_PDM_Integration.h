/*
 * ECU_PDM_Integration.h
 *
 * Integration code for adding PDM support to RX8_CANBUS.ino
 *
 * USAGE:
 * 1. Add #include "ECU_PDM_Integration.h" to RX8_CANBUS.ino
 * 2. Call pdm_setup() in setup()
 * 3. Call pdm_update() in sendOnTenth()
 * 4. Call pdm_receive() in loop() when CAN message received
 *
 * Author: MazdaRX8Arduino Project
 * Date: 2025-11-17
 */

#ifndef ECU_PDM_INTEGRATION_H
#define ECU_PDM_INTEGRATION_H

#include <Arduino.h>

// ========== PDM CONFIGURATION ==========
// Uncomment to enable PDM features
#define ENABLE_PDM

#ifdef ENABLE_PDM

// Build type (must match PDM configuration)
#define PDM_ENGINE_SWAP      // LS1, 2JZ, etc.
// #define PDM_EV_CONVERSION    // Electric conversion
// #define PDM_RACE_CAR         // Track/race setup
// #define PDM_STOCK_ROTARY     // Factory rotary

// ========== PDM CONTROL VARIABLES ==========

// Channel enable flags
bool pdm_fuelPumpEnable = false;
bool pdm_fan1Enable = false;
bool pdm_fan2Enable = false;
bool pdm_waterPumpEnable = false;
bool pdm_acClutchEnable = false;
bool pdm_aux1Enable = false;
bool pdm_aux2Enable = false;
bool pdm_aux3Enable = false;

// PWM values (0-255)
byte pdm_fuelPumpPWM = 255;
byte pdm_fan1PWM = 0;
byte pdm_fan2PWM = 0;
byte pdm_waterPumpPWM = 255;
byte pdm_acClutchPWM = 255;
byte pdm_aux1PWM = 0;
byte pdm_aux2PWM = 0;
byte pdm_aux3PWM = 0;

// PDM status (received from PDM)
byte pdm_channelStatus = 0;
byte pdm_channelCurrent[8];
byte pdm_faultFlags = 0;
float pdm_totalCurrent = 0;
float pdm_batteryVoltage = 0;
float pdm_totalEnergy = 0;
bool pdm_killSwitchActive = false;
bool pdm_loadSheddingActive = false;

// Communication
bool pdm_connected = false;
unsigned long pdm_lastReceive = 0;

// ========== PROGRESSIVE FAN CONTROL ==========

/*
 * Temperature-based progressive fan control
 * Ramps fan speed based on coolant temperature
 *
 * Temperature zones:
 * < 75°C (167°F):  Fans OFF
 * 75-85°C (167-185°F): Fan 1 @ 50%
 * 85-95°C (185-203°F): Fan 1 @ 70%, Fan 2 @ 70%
 * > 95°C (203°F):  Both fans @ 100%
 */
void pdm_updateFanControl(byte coolantTemp) {
  if (coolantTemp > 95) {
    // Critical: Both fans 100%
    pdm_fan1Enable = true;
    pdm_fan2Enable = true;
    pdm_fan1PWM = 255;
    pdm_fan2PWM = 255;
  }
  else if (coolantTemp > 85) {
    // Hot: Both fans 70%
    pdm_fan1Enable = true;
    pdm_fan2Enable = true;
    pdm_fan1PWM = 180;  // ~70%
    pdm_fan2PWM = 180;
  }
  else if (coolantTemp > 75) {
    // Warm: Fan 1 only, 50%
    pdm_fan1Enable = true;
    pdm_fan2Enable = false;
    pdm_fan1PWM = 128;  // 50%
  }
  else {
    // Cool: Fans OFF
    pdm_fan1Enable = false;
    pdm_fan2Enable = false;
  }
}

// ========== ENGINE-SPECIFIC CONTROL ==========

#ifdef PDM_ENGINE_SWAP

/*
 * Engine swap control logic
 * Typical setup: LS1, 2JZ, V8, etc.
 */
void pdm_updateEngineSwapControl(int rpm, byte coolantTemp, bool acRequest) {
  // 1. Fuel pump: ON when cranking or running
  if (rpm > 200) {  // Cranking or running
    pdm_fuelPumpEnable = true;
    pdm_fuelPumpPWM = 255;  // Always full power
  } else {
    // Prime fuel system for 2 seconds after key ON
    static unsigned long keyOnTime = 0;
    static bool primed = false;

    if (rpm == 0 && !primed) {
      if (keyOnTime == 0) {
        keyOnTime = millis();
      }
      if (millis() - keyOnTime < 2000) {
        pdm_fuelPumpEnable = true;  // Prime
      } else {
        pdm_fuelPumpEnable = false;
        primed = true;
      }
    }
  }

  // 2. Radiator fans (progressive control)
  pdm_updateFanControl(coolantTemp);

  // 3. Water pump (for engine swaps with electric water pump)
  if (rpm > 0) {
    pdm_waterPumpEnable = true;
    // Optional: Variable speed based on coolant temp
    if (coolantTemp > 90) {
      pdm_waterPumpPWM = 255;  // 100%
    } else {
      pdm_waterPumpPWM = 200;  // ~78% (reduces noise)
    }
  } else {
    pdm_waterPumpEnable = false;
  }

  // 4. A/C clutch (if A/C system present)
  if (acRequest && rpm > 800 && coolantTemp < 100) {
    pdm_acClutchEnable = true;
    pdm_acClutchPWM = 255;
  } else {
    pdm_acClutchEnable = false;
  }

  // 5. Transmission cooler fan (use aux1)
  if (coolantTemp > 80) {  // Trans cooler based on coolant temp as proxy
    pdm_aux1Enable = true;
    pdm_aux1PWM = 180;  // 70%
  } else {
    pdm_aux1Enable = false;
  }
}

#endif

// ========== EV CONVERSION CONTROL ==========

#ifdef PDM_EV_CONVERSION

/*
 * EV conversion control logic
 * Manages motor controller, BMS, coolant pumps, charging
 */
void pdm_updateEVControl(bool keyOn, bool charging, byte motorTemp, byte batteryTemp, float motorCurrent) {
  // 1. BMS power: Always ON when key ON
  if (keyOn) {
    pdm_fuelPumpEnable = true;  // Ch1 = BMS (reused fuel pump channel)
    pdm_fuelPumpPWM = 255;
  } else {
    pdm_fuelPumpEnable = false;
  }

  // 2. Motor controller 12V supply
  if (keyOn) {
    pdm_fan1Enable = true;  // Ch2 = Motor controller
    pdm_fan1PWM = 255;
  } else {
    pdm_fan1Enable = false;
  }

  // 3. Motor coolant pump (temperature-based)
  if (motorTemp > 60) {  // 60°C (140°F)
    pdm_fan2Enable = true;  // Ch3 = Motor coolant
    if (motorTemp > 80) {
      pdm_fan2PWM = 255;  // 100%
    } else {
      pdm_fan2PWM = 180;  // 70%
    }
  } else if (keyOn && motorCurrent > 100) {  // High current = preemptive cooling
    pdm_fan2Enable = true;
    pdm_fan2PWM = 128;  // 50%
  } else {
    pdm_fan2Enable = false;
  }

  // 4. Battery coolant pump (temperature-based)
  if (batteryTemp > 35) {  // 35°C (95°F)
    pdm_waterPumpEnable = true;  // Ch4 = Battery coolant
    if (batteryTemp > 45) {
      pdm_waterPumpPWM = 255;  // 100%
    } else {
      pdm_waterPumpPWM = 180;  // 70%
    }
  } else if (charging && batteryTemp > 30) {  // Cool while charging
    pdm_waterPumpEnable = true;
    pdm_waterPumpPWM = 128;  // 50%
  } else {
    pdm_waterPumpEnable = false;
  }

  // 5. DC-DC converter enable
  if (keyOn) {
    pdm_acClutchEnable = true;  // Ch5 = DC-DC
    pdm_acClutchPWM = 255;
  } else {
    pdm_acClutchEnable = false;
  }

  // 6. Charging system
  if (charging) {
    pdm_aux1Enable = true;  // Ch6 = Charger
    pdm_aux1PWM = 255;
  } else {
    pdm_aux1Enable = false;
  }

  // 7. Cabin heater (if needed)
  // Controlled separately by HVAC system
}

#endif

// ========== RACE CAR CONTROL ==========

#ifdef PDM_RACE_CAR

/*
 * Race car control logic
 * Focus: Reliability, telemetry, performance
 */
void pdm_updateRaceCarControl(int rpm, byte coolantTemp, byte oilTemp, bool onTrack) {
  // 1. Fuel pump: Always ON during session
  if (onTrack || rpm > 0) {
    pdm_fuelPumpEnable = true;
    pdm_fuelPumpPWM = 255;
  } else {
    pdm_fuelPumpEnable = false;
  }

  // 2. Oil cooler fan (aggressive cooling)
  if (oilTemp > 90 || (onTrack && oilTemp > 80)) {
    pdm_fan1Enable = true;
    pdm_fan1PWM = 255;  // Always 100% on track
  } else if (oilTemp > 70) {
    pdm_fan1Enable = true;
    pdm_fan1PWM = 180;  // 70%
  } else {
    pdm_fan1Enable = false;
  }

  // 3. Water pump (high performance)
  if (rpm > 0) {
    pdm_fan2Enable = true;
    pdm_fan2PWM = 255;  // Always 100% (no compromise)
  } else {
    pdm_fan2Enable = false;
  }

  // 4. Transmission oil pump (if separate)
  if (rpm > 1000) {
    pdm_waterPumpEnable = true;
    pdm_waterPumpPWM = 255;
  } else {
    pdm_waterPumpEnable = false;
  }

  // 5. Data logger power
  if (onTrack || rpm > 0) {
    pdm_acClutchEnable = true;
    pdm_acClutchPWM = 255;
  } else {
    pdm_acClutchEnable = false;
  }

  // 6. Shift light (controlled separately by tachometer)
  // pdm_aux1Enable set by shift point logic

  // 7. Brake cooling fan (if installed)
  if (onTrack) {
    pdm_aux2Enable = true;
    pdm_aux2PWM = 255;
  } else {
    pdm_aux2Enable = false;
  }
}

#endif

// ========== CAN COMMUNICATION ==========

/*
 * Send PDM control message
 * CAN ID: 0x600
 * Called every 100ms from sendOnTenth()
 */
void pdm_sendControl(MCP_CAN &CAN0) {
  byte msg[8];

  // Byte 0: Enable bits
  msg[0] = 0;
  if (pdm_fuelPumpEnable) msg[0] |= 0x01;
  if (pdm_fan1Enable) msg[0] |= 0x02;
  if (pdm_fan2Enable) msg[0] |= 0x04;
  if (pdm_waterPumpEnable) msg[0] |= 0x08;
  if (pdm_acClutchEnable) msg[0] |= 0x10;
  if (pdm_aux1Enable) msg[0] |= 0x20;
  if (pdm_aux2Enable) msg[0] |= 0x40;
  if (pdm_aux3Enable) msg[0] |= 0x80;

  // Bytes 1-4: PWM values
  msg[1] = pdm_fuelPumpPWM;
  msg[2] = pdm_fan1PWM;
  msg[3] = pdm_fan2PWM;
  msg[4] = pdm_waterPumpPWM;

  // Bytes 5-7: Reserved
  msg[5] = 0;
  msg[6] = 0;
  msg[7] = 0;

  CAN0.sendMsgBuf(0x600, 0, 8, msg);
}

/*
 * Receive PDM status message
 * CAN ID: 0x601 (channel status) or 0x603 (system telemetry)
 * Called in loop() when CAN message received
 */
void pdm_receiveStatus(unsigned long id, byte buf[]) {
  pdm_lastReceive = millis();
  pdm_connected = true;

  if (id == 0x601) {  // Channel status
    pdm_channelStatus = buf[0];

    // Bytes 1-4: Current per channel
    for (int i = 0; i < 4; i++) {
      pdm_channelCurrent[i] = buf[i + 1];
    }

    // Byte 5: Fault flags
    pdm_faultFlags = buf[5];

    // Bytes 6-7: Total current (16-bit, 0.1A resolution)
    int totalCurrentInt = (buf[6] << 8) | buf[7];
    pdm_totalCurrent = totalCurrentInt / 10.0;

    // Check for faults
    if (pdm_faultFlags != 0) {
      // Could trigger check engine light or warning
      // checkEngineMIL = 1;
    }
  }
  else if (id == 0x603) {  // System telemetry
    // Byte 0: Battery voltage (0.1V resolution)
    pdm_batteryVoltage = buf[0] / 10.0;

    // Byte 1: Status flags
    pdm_killSwitchActive = (buf[1] & 0x01) != 0;
    pdm_loadSheddingActive = (buf[1] & 0x04) != 0;

    // Bytes 2-3: Total energy (Wh, 16-bit)
    int energyInt = (buf[2] << 8) | buf[3];
    pdm_totalEnergy = energyInt;

    // Load shedding warning
    if (pdm_loadSheddingActive) {
      // Could trigger dashboard warning
    }
  }
}

/*
 * Send PDM command message
 * CAN ID: 0x602
 */
void pdm_sendCommand(MCP_CAN &CAN0, byte command) {
  byte msg[8] = {command, 0, 0, 0, 0, 0, 0, 0};
  CAN0.sendMsgBuf(0x602, 0, 8, msg);
}

// ========== FAULT MANAGEMENT ==========

/*
 * Reset all PDM faults
 */
void pdm_resetFaults(MCP_CAN &CAN0) {
  pdm_sendCommand(CAN0, 0x01);
  pdm_faultFlags = 0;
}

/*
 * Reset peak current counters
 */
void pdm_resetPeakCurrent(MCP_CAN &CAN0) {
  pdm_sendCommand(CAN0, 0x10);
}

// ========== SETUP & UPDATE FUNCTIONS ==========

/*
 * Initialize PDM integration
 * Call from setup()
 */
void pdm_setup() {
  Serial.println(F("PDM integration enabled"));

  #ifdef PDM_ENGINE_SWAP
  Serial.println(F("  Mode: Engine Swap"));
  #endif
  #ifdef PDM_EV_CONVERSION
  Serial.println(F("  Mode: EV Conversion"));
  #endif
  #ifdef PDM_RACE_CAR
  Serial.println(F("  Mode: Race Car"));
  #endif

  // Initialize all channels to OFF
  pdm_fuelPumpEnable = false;
  pdm_fan1Enable = false;
  pdm_fan2Enable = false;
  pdm_waterPumpEnable = false;
  pdm_acClutchEnable = false;
  pdm_aux1Enable = false;
  pdm_aux2Enable = false;
  pdm_aux3Enable = false;
}

/*
 * Update PDM control logic
 * Call from sendOnTenth() (every 100ms)
 */
void pdm_update(MCP_CAN &CAN0, int rpm, byte coolantTemp) {
  // Update control logic based on build type
  #ifdef PDM_ENGINE_SWAP
    pdm_updateEngineSwapControl(rpm, coolantTemp, false);  // TODO: Add A/C request input
  #endif

  #ifdef PDM_EV_CONVERSION
    // pdm_updateEVControl(true, false, 40, 30, 0);  // TODO: Add actual sensor inputs
  #endif

  #ifdef PDM_RACE_CAR
    // pdm_updateRaceCarControl(rpm, coolantTemp, 90, false);  // TODO: Add oil temp, on-track flag
  #endif

  // Send control message to PDM
  pdm_sendControl(CAN0);

  // Check for PDM timeout (failsafe)
  if (pdm_connected && millis() - pdm_lastReceive > 1000) {
    pdm_connected = false;
    Serial.println(F("WARNING: PDM timeout!"));
  }
}

/*
 * Print PDM telemetry to Serial
 * Call periodically for debugging
 */
void pdm_printTelemetry() {
  Serial.print(F("PDM: "));
  Serial.print(pdm_connected ? F("CONN") : F("DISC"));
  Serial.print(F(" | Batt: "));
  Serial.print(pdm_batteryVoltage, 1);
  Serial.print(F("V | Total: "));
  Serial.print(pdm_totalCurrent, 1);
  Serial.print(F("A | Energy: "));
  Serial.print(pdm_totalEnergy, 0);
  Serial.print(F("Wh"));

  if (pdm_faultFlags != 0) {
    Serial.print(F(" | FAULTS: 0x"));
    Serial.print(pdm_faultFlags, HEX);
  }

  if (pdm_loadSheddingActive) {
    Serial.print(F(" | LOAD SHEDDING"));
  }

  Serial.println();
}

#endif  // ENABLE_PDM

#endif  // ECU_PDM_INTEGRATION_H
