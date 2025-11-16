/**
 * @file pin_mapping.h
 * @brief NXP S32K148 pin mapping for RX8 automotive ECU
 *
 * Pin assignments for S32K148EVB evaluation board
 * (100-pin LQFP package)
 *
 * ISO 26262 ASIL-B capable platform
 *
 * @author Created for Phase 5 unified architecture
 * @date 2025-11-16
 */

#ifndef PIN_MAPPING_S32K148_H
#define PIN_MAPPING_S32K148_H

// ============================================================================
// CAN Bus (3x FlexCAN, CAN-FD capable)
// ============================================================================

#define CAN0_TX_PIN     PTE5   // FlexCAN0 TX
#define CAN0_RX_PIN     PTE4   // FlexCAN0 RX
#define CAN1_TX_PIN     PTA13  // FlexCAN1 TX
#define CAN1_RX_PIN     PTA12  // FlexCAN1 RX
#define CAN2_TX_PIN     PTE25  // FlexCAN2 TX
#define CAN2_RX_PIN     PTE24  // FlexCAN2 RX

// ============================================================================
// Throttle Pedal (RX8)
// ============================================================================

// Analog input: 1.7V - 4.0V range from RX8 pedal
#define THROTTLE_ADC_PIN        PTA0   // ADC0_SE0

// PWM output: 0 - 4.5V for motor controller (FTM0_CH0)
#define THROTTLE_PWM_PIN        PTD15  // FTM0_CH0

// ============================================================================
// Status LEDs
// ============================================================================

#define LED_STATUS_RED      PTD16  // RGB LED red
#define LED_STATUS_GREEN    PTD15  // RGB LED green
#define LED_STATUS_BLUE     PTD0   // RGB LED blue

// ============================================================================
// UART Communication
// ============================================================================

// UART to ESP32 (LPUART1)
#define UART_ESP32_TX   PTC6   // LPUART1 TX
#define UART_ESP32_RX   PTC7   // LPUART1 RX
#define UART_ESP32_PORT 1      // LPUART1

// Debug serial (LPUART0 - via OpenSDA)
#define UART_DEBUG_TX   PTC2   // LPUART0 TX
#define UART_DEBUG_RX   PTC3   // LPUART0 RX
#define UART_DEBUG_PORT 0      // LPUART0

// ============================================================================
// Motor Control (EV only)
// ============================================================================

#define MOTOR_PWM_U     PTD16  // FTM3_CH0 (Phase U)
#define MOTOR_PWM_V     PTD1   // FTM3_CH1 (Phase V)
#define MOTOR_PWM_W     PTE8   // FTM3_CH2 (Phase W)

#define MOTOR_ENABLE_PIN    PTE2
#define MOTOR_DIRECTION_PIN PTE3

// ============================================================================
// Analog Sensors (12-bit ADC with PDB trigger)
// ============================================================================

#define SENSOR_BATTERY_VOLTAGE  PTA1   // ADC0_SE1
#define SENSOR_COOLANT_TEMP     PTB0   // ADC0_SE4
#define SENSOR_OIL_PRESSURE     PTB1   // ADC0_SE5
#define SENSOR_BRAKE_PRESSURE   PTB2   // ADC0_SE6

// ============================================================================
// Digital Inputs (5V tolerant)
// ============================================================================

#define INPUT_BRAKE_SWITCH      PTC8
#define INPUT_CLUTCH_SWITCH     PTC9
#define INPUT_GEAR_NEUTRAL      PTC10
#define INPUT_REVERSE_GEAR      PTC11

// ============================================================================
// Digital Outputs
// ============================================================================

#define OUTPUT_FUEL_PUMP        PTC12
#define OUTPUT_FAN_CONTROL      PTC13
#define OUTPUT_AC_COMPRESSOR    PTD2
#define OUTPUT_STARTER_RELAY    PTD3

// ============================================================================
// SPI (LPSPI for external peripherals)
// ============================================================================

#define SPI_MOSI        PTB16  // LPSPI1_SOUT
#define SPI_MISO        PTB17  // LPSPI1_SIN
#define SPI_SCK         PTB14  // LPSPI1_SCK
#define SPI_CS          PTB15  // LPSPI1_PCS0

// ============================================================================
// I2C (LPI2C for sensors)
// ============================================================================

#define I2C_SDA         PTA2   // LPI2C0_SDA
#define I2C_SCL         PTA3   // LPI2C0_SCL

// ============================================================================
// Timing/Capture Inputs (FTM input capture)
// ============================================================================

#define INPUT_CRANK_SENSOR      PTD4   // FTM0_CH4
#define INPUT_CAM_SENSOR        PTD5   // FTM0_CH5
#define INPUT_SPEED_SENSOR      PTC1   // FTM0_CH1

// ============================================================================
// Pin Mapping Notes
// ============================================================================

/*
 * IMPORTANT: Voltage levels
 * - S32K148 is 5V-tolerant on all GPIO pins
 * - ADC inputs: 0-5V (automotive-grade capability)
 * - GPIO outputs: 3.3V logic (5V-tolerant inputs)
 *
 * ISO 26262 ASIL-B features:
 * - ECC on Flash and RAM
 * - Hardware CRC module
 * - Cyclic redundancy check on critical registers
 * - Lockstep core option (S32K144 dual-core variant)
 *
 * CAN transceiver:
 * - Use TJA1043 or TJA1145 (CAN-FD, partial networking)
 * - 120Ω termination resistor required on bus
 * - CAN-FD capable: up to 5 Mbps data rate
 *
 * Throttle pedal:
 * - RX8 pedal: 1.7V - 4.0V (direct connection OK)
 * - Output to motor controller: Use FTM for precise PWM
 * - 5V-tolerant inputs allow direct connection to sensors
 *
 * FlexTimer (FTM) features:
 * - 8 channels per module
 * - Combine mode for complementary PWM
 * - Dead-time insertion
 * - Fault detection with safe state
 *
 * Power:
 * - S32K148: 3.3V I/O, 1.2V core
 * - Vin: 5V (from external regulator, automotive-grade)
 * - Max current per pin: 10 mA
 * - Total max current: 400 mA (all GPIO combined)
 *
 * Safety features:
 * - Hardware watchdog (WDOG) with window mode
 * - Memory protection unit (MPU)
 * - ECC on Flash and RAM
 * - Clock monitoring (CMU)
 * - Voltage monitoring (PMC)
 * - Temperature sensor
 *
 * Production automotive use:
 * - AEC-Q100 qualified
 * - -40°C to 150°C operating range
 * - ISO 26262 ASIL-B certified
 * - Long-term availability (15+ years)
 */

#endif // PIN_MAPPING_S32K148_H
