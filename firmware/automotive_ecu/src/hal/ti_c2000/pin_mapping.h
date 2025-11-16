/**
 * @file pin_mapping.h
 * @brief TI C2000 F28379D pin mapping for RX8 automotive ECU
 *
 * Pin assignments for LAUNCHXL-F28379D LaunchPad
 * (176-pin LQFP package)
 *
 * @author Created for Phase 5 unified architecture
 * @date 2025-11-16
 */

#ifndef PIN_MAPPING_C2000_H
#define PIN_MAPPING_C2000_H

// ============================================================================
// CAN Bus
// ============================================================================

#define CANA_TX_PIN     31  // GPIO31 - CANTXA
#define CANA_RX_PIN     30  // GPIO30 - CANRXA
#define CANB_TX_PIN     17  // GPIO17 - CANTXB
#define CANB_RX_PIN     12  // GPIO12 - CANRXB

// ============================================================================
// Throttle Pedal (RX8)
// ============================================================================

// Analog input: 1.7V - 4.0V range from RX8 pedal
#define THROTTLE_ADC_PIN        0   // ADCINA0

// PWM output: 0 - 4.5V for motor controller (ePWM1A)
#define THROTTLE_PWM_PIN        0   // GPIO0 - EPWM1A

// ============================================================================
// Status LEDs
// ============================================================================

#define LED_STATUS_1    31  // GPIO31 (red LED on LaunchPad)
#define LED_STATUS_2    34  // GPIO34 (blue LED on LaunchPad)
#define LED_STATUS_3    23  // GPIO23 (external LED)

// ============================================================================
// UART Communication
// ============================================================================

// UART to ESP32 (SCIA)
#define UART_ESP32_TX   28  // GPIO28 - SCITXDA
#define UART_ESP32_RX   29  // GPIO29 - SCIRXDA
#define UART_ESP32_PORT 0   // SCIA

// Debug serial (SCIB)
#define UART_DEBUG_TX   14  // GPIO14 - SCITXDB (USB on LaunchPad)
#define UART_DEBUG_RX   15  // GPIO15 - SCIRXDB
#define UART_DEBUG_PORT 1   // SCIB

// ============================================================================
// Motor Control (EV only) - C2000 specialty!
// ============================================================================

// High-resolution ePWM for motor control
#define MOTOR_PWM_U     0   // GPIO0 - EPWM1A (Phase U)
#define MOTOR_PWM_V     2   // GPIO2 - EPWM2A (Phase V)
#define MOTOR_PWM_W     4   // GPIO4 - EPWM3A (Phase W)

// Complementary outputs for 3-phase inverter
#define MOTOR_PWM_U_N   1   // GPIO1 - EPWM1B (Phase U complementary)
#define MOTOR_PWM_V_N   3   // GPIO3 - EPWM2B (Phase V complementary)
#define MOTOR_PWM_W_N   5   // GPIO5 - EPWM3B (Phase W complementary)

// Motor enable/direction
#define MOTOR_ENABLE_PIN    6   // GPIO6
#define MOTOR_DIRECTION_PIN 7   // GPIO7

// Encoder inputs (eQEP - quadrature encoder)
#define ENCODER_A_PIN   20  // GPIO20 - EQEP1A
#define ENCODER_B_PIN   21  // GPIO21 - EQEP1B
#define ENCODER_I_PIN   23  // GPIO23 - EQEP1I (index)

// ============================================================================
// Analog Sensors (16-bit ADC!)
// ============================================================================

#define SENSOR_BATTERY_VOLTAGE  1   // ADCINA1
#define SENSOR_COOLANT_TEMP     2   // ADCINA2
#define SENSOR_OIL_PRESSURE     3   // ADCINA3
#define SENSOR_BRAKE_PRESSURE   4   // ADCINA4
#define SENSOR_PHASE_U_CURRENT  14  // ADCINB0 (motor current sensing)
#define SENSOR_PHASE_V_CURRENT  15  // ADCINB1
#define SENSOR_PHASE_W_CURRENT  16  // ADCINB2

// ============================================================================
// Digital Inputs
// ============================================================================

#define INPUT_BRAKE_SWITCH      8   // GPIO8
#define INPUT_CLUTCH_SWITCH     9   // GPIO9
#define INPUT_GEAR_NEUTRAL      10  // GPIO10
#define INPUT_REVERSE_GEAR      11  // GPIO11

// ============================================================================
// Digital Outputs
// ============================================================================

#define OUTPUT_FUEL_PUMP        16  // GPIO16
#define OUTPUT_FAN_CONTROL      18  // GPIO18
#define OUTPUT_AC_COMPRESSOR    19  // GPIO19
#define OUTPUT_STARTER_RELAY    22  // GPIO22

// ============================================================================
// SPI (for external peripherals)
// ============================================================================

#define SPI_SIMO        16  // GPIO16 - SPISIMOA (MOSI)
#define SPI_SOMI        17  // GPIO17 - SPISOMIA (MISO)
#define SPI_CLK         18  // GPIO18 - SPICLKA
#define SPI_CS          19  // GPIO19 - SPISTEA (CS)

// ============================================================================
// I2C (for sensors, RTC, EEPROM)
// ============================================================================

#define I2C_SDA         32  // GPIO32 - SDAA
#define I2C_SCL         33  // GPIO33 - SCLA

// ============================================================================
// Timing/Capture Inputs (eCAP for crank/cam sensors)
// ============================================================================

#define INPUT_CRANK_SENSOR      24  // GPIO24 - ECAP1
#define INPUT_CAM_SENSOR        25  // GPIO25 - ECAP2
#define INPUT_SPEED_SENSOR      26  // GPIO26 - ECAP3

// ============================================================================
// Pin Mapping Notes
// ============================================================================

/*
 * IMPORTANT: Voltage levels
 * - C2000 F28379D is 3.3V logic core with 5V-tolerant I/O
 * - ADC inputs: 0-3.3V (use voltage dividers for higher voltages)
 * - GPIO outputs: 3.3V logic (can drive 5V loads with pull-up)
 *
 * CAN transceiver:
 * - Use TJA1050 or MCP2551 (3.3V compatible)
 * - 120Ω termination resistor required on bus
 * - CAN-FD capable (up to 5 Mbps data rate)
 *
 * Throttle pedal:
 * - RX8 pedal: 1.7V - 4.0V (direct connection OK)
 * - Output to motor controller: Use ePWM for precise control
 *
 * Motor control (EV applications):
 * - ePWM modules support high-resolution PWM (150 ps resolution!)
 * - Dead-time insertion for complementary outputs
 * - Trip-zone protection for overcurrent/overvoltage
 * - eQEP for encoder feedback (position/speed)
 *
 * Power:
 * - C2000 F28379D: 3.3V core, 1.2V internal
 * - Vin: 5V (from USB or external regulator)
 * - Max current per pin: 4 mA (low drive), 8 mA (high drive)
 * - Total max current: 200 mA (all GPIO combined)
 *
 * Specialty features:
 * - 16-bit ADC (vs 12-bit on STM32) = 16x better resolution
 * - High-resolution PWM = precise motor control
 * - Windowed watchdog = more sophisticated fault detection
 * - Dual C28x cores = parallel processing capability
 */

#endif // PIN_MAPPING_C2000_H
