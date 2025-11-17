/**
 * @file pin_mapping.h
 * @brief STM32F407 pin mapping for RX8 automotive ECU
 *
 * Pin assignments for Nucleo-F407ZG board
 * (144-pin LQFP package, Arduino-compatible headers)
 *
 * @author Created for unified automotive ECU
 * @date 2025-11-16
 */

#ifndef PIN_MAPPING_STM32F407_H
#define PIN_MAPPING_STM32F407_H

// ============================================================================
// CAN Bus
// ============================================================================

#define CAN1_TX_PIN     PB9   // CAN1 transmit
#define CAN1_RX_PIN     PB8   // CAN1 receive
#define CAN2_TX_PIN     PB13  // CAN2 transmit (optional)
#define CAN2_RX_PIN     PB12  // CAN2 receive (optional)

// ============================================================================
// Throttle Pedal (RX8)
// ============================================================================

// Analog input: 1.7V - 4.0V range from RX8 pedal
#define THROTTLE_ADC_PIN        PA0   // ADC1_IN0

// PWM output: 0 - 4.5V for motor controller
#define THROTTLE_PWM_PIN        PA8   // TIM1_CH1 (advanced timer)

// ============================================================================
// Status LEDs
// ============================================================================

#define LED_STATUS_1    PC13  // Onboard LED (green)
#define LED_STATUS_2    PB0   // External LED (optional)
#define LED_STATUS_3    PB7   // External LED (optional)

// ============================================================================
// UART Communication
// ============================================================================

// UART to ESP32 (for vehicle state bridge)
#define UART_ESP32_TX   PA2   // USART2 TX
#define UART_ESP32_RX   PA3   // USART2 RX
#define UART_ESP32_PORT 2     // USART2 (Serial2)

// Debug serial (USB virtual COM port)
#define UART_DEBUG_TX   PA9   // USART1 TX
#define UART_DEBUG_RX   PA10  // USART1 RX
#define UART_DEBUG_PORT 1     // USART1 (Serial)

// ============================================================================
// Motor Control (EV only)
// ============================================================================

#define MOTOR_PWM_PIN       PA15  // TIM2_CH1 (motor controller PWM)
#define MOTOR_DIRECTION_PIN PB4   // Motor direction control
#define MOTOR_ENABLE_PIN    PB5   // Motor enable signal

// ============================================================================
// Analog Sensors
// ============================================================================

#define SENSOR_BATTERY_VOLTAGE  PA1   // ADC1_IN1 (12V battery via divider)
#define SENSOR_COOLANT_TEMP     PA4   // ADC1_IN4 (coolant temp sensor)
#define SENSOR_OIL_PRESSURE     PA5   // ADC1_IN5 (oil pressure sensor)
#define SENSOR_BRAKE_PRESSURE   PA6   // ADC1_IN6 (brake pressure sensor)

// ============================================================================
// Digital Inputs
// ============================================================================

#define INPUT_BRAKE_SWITCH      PC0   // Brake pedal switch
#define INPUT_CLUTCH_SWITCH     PC1   // Clutch pedal switch (MT only)
#define INPUT_GEAR_NEUTRAL      PC2   // Neutral gear switch
#define INPUT_REVERSE_GEAR      PC3   // Reverse gear switch

// ============================================================================
// Digital Outputs
// ============================================================================

#define OUTPUT_FUEL_PUMP        PC4   // Fuel pump relay
#define OUTPUT_FAN_CONTROL      PC5   // Cooling fan relay
#define OUTPUT_AC_COMPRESSOR    PC6   // AC compressor relay
#define OUTPUT_STARTER_RELAY    PC7   // Starter motor relay

// ============================================================================
// SPI (for external peripherals)
// ============================================================================

#define SPI_MOSI        PB15  // SPI2 MOSI
#define SPI_MISO        PB14  // SPI2 MISO
#define SPI_SCK         PB10  // SPI2 SCK
#define SPI_CS          PB11  // SPI2 CS (chip select)

// ============================================================================
// I2C (for sensors, RTC, EEPROM)
// ============================================================================

#define I2C_SDA         PB9   // I2C1 SDA
#define I2C_SCL         PB8   // I2C1 SCL

// ============================================================================
// Timing/Encoder Inputs
// ============================================================================

#define INPUT_CRANK_SENSOR      PA11  // TIM1_CH4 (crankshaft position)
#define INPUT_CAM_SENSOR        PA12  // TIM1_ETR (camshaft position)
#define INPUT_SPEED_SENSOR      PB6   // TIM4_CH1 (vehicle speed sensor)

// ============================================================================
// Pin Mapping Notes
// ============================================================================

/*
 * IMPORTANT: Voltage levels
 * - STM32F407 is 3.3V logic (NOT 5V tolerant on all pins)
 * - Use voltage dividers for 12V signals
 * - Use level shifters for 5V signals if needed
 *
 * CAN transceiver:
 * - Use TJA1050 or MCP2551 (3.3V compatible)
 * - 120Ω termination resistor required on bus
 *
 * Throttle pedal:
 * - RX8 pedal: 1.7V - 4.0V (direct connection OK)
 * - Output to motor controller: Use external circuit for 0-5V
 *
 * Power:
 * - STM32F407: 3.3V supply (via onboard regulator)
 * - Vin: 7-12V (from vehicle 12V via polyfuse)
 * - Max current per pin: 25 mA
 * - Total max current: 150 mA (all GPIO combined)
 */

#endif // PIN_MAPPING_STM32F407_H
