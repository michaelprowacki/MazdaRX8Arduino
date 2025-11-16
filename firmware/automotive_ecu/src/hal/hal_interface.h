/**
 * @file hal_interface.h
 * @brief Hardware Abstraction Layer interface
 *
 * Platform-independent API for hardware access.
 * All platform-specific implementations (STM32, C2000, S32K, etc.)
 * must implement these functions.
 *
 * Supported platforms:
 * - STM32F407 (ARM Cortex-M4, entry-level automotive)
 * - TI C2000 F28379D (motor control specialist)
 * - NXP S32K148 (production automotive, ISO 26262)
 * - Infineon AURIX TC375 (high-end automotive)
 * - TI Hercules RM46 (safety-critical automotive)
 *
 * @author Created for unified automotive ECU
 * @date 2025-11-16
 */

#ifndef HAL_INTERFACE_H
#define HAL_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// GPIO (General Purpose Input/Output)
// ============================================================================

/**
 * @brief GPIO pin modes
 */
typedef enum {
    HAL_GPIO_MODE_INPUT,        // Digital input
    HAL_GPIO_MODE_OUTPUT,       // Digital output
    HAL_GPIO_MODE_INPUT_PULLUP, // Input with pull-up resistor
    HAL_GPIO_MODE_INPUT_PULLDOWN, // Input with pull-down resistor
    HAL_GPIO_MODE_ANALOG,       // Analog input (ADC)
    HAL_GPIO_MODE_PWM,          // PWM output
    HAL_GPIO_MODE_ALTERNATE     // Alternate function (CAN, UART, etc.)
} HAL_GPIO_Mode;

/**
 * @brief Set GPIO pin mode
 * @param pin Pin number (platform-specific mapping)
 * @param mode Pin mode
 */
void HAL_GPIO_SetMode(uint8_t pin, HAL_GPIO_Mode mode);

/**
 * @brief Write digital output
 * @param pin Pin number
 * @param state true = HIGH, false = LOW
 */
void HAL_GPIO_Write(uint8_t pin, bool state);

/**
 * @brief Read digital input
 * @param pin Pin number
 * @return true = HIGH, false = LOW
 */
bool HAL_GPIO_Read(uint8_t pin);

/**
 * @brief Toggle digital output
 * @param pin Pin number
 */
void HAL_GPIO_Toggle(uint8_t pin);

// ============================================================================
// ADC (Analog to Digital Converter)
// ============================================================================

/**
 * @brief Read analog input
 * @param pin Analog pin number (platform-specific)
 * @return ADC value (12-bit: 0-4095 typical)
 */
uint16_t HAL_ADC_Read(uint8_t pin);

/**
 * @brief Read analog input (millivolts)
 * @param pin Analog pin number
 * @return Voltage in millivolts
 */
uint16_t HAL_ADC_ReadMillivolts(uint8_t pin);

// ============================================================================
// PWM (Pulse Width Modulation)
// ============================================================================

/**
 * @brief Write PWM output
 * @param pin PWM pin number
 * @param value PWM duty cycle (8-bit: 0-255)
 */
void HAL_PWM_Write(uint8_t pin, uint8_t value);

/**
 * @brief Write PWM output (16-bit)
 * @param pin PWM pin number
 * @param value PWM duty cycle (16-bit: 0-65535)
 */
void HAL_PWM_Write16(uint8_t pin, uint16_t value);

/**
 * @brief Set PWM frequency
 * @param pin PWM pin number
 * @param frequency Frequency in Hz
 */
void HAL_PWM_SetFrequency(uint8_t pin, uint32_t frequency);

// ============================================================================
// CAN (Controller Area Network)
// ============================================================================

/**
 * @brief CAN message structure
 */
typedef struct {
    uint32_t id;        // CAN message ID
    uint8_t  len;       // Data length (0-8)
    uint8_t  data[8];   // Data bytes
    bool     extended;  // Extended ID (29-bit) vs standard (11-bit)
} HAL_CAN_Message;

/**
 * @brief Initialize CAN controller
 * @param speed CAN bus speed in bps (e.g., 500000 for 500 kbps)
 * @return true if successful
 */
bool HAL_CAN_Init(uint32_t speed);

/**
 * @brief Transmit CAN message
 * @param msg Pointer to CAN message structure
 * @return true if message queued successfully
 */
bool HAL_CAN_Transmit(const HAL_CAN_Message* msg);

/**
 * @brief Receive CAN message (non-blocking)
 * @param msg Pointer to CAN message structure to fill
 * @return true if message received
 */
bool HAL_CAN_Receive(HAL_CAN_Message* msg);

/**
 * @brief Check if CAN message available
 * @return true if message waiting in RX buffer
 */
bool HAL_CAN_Available();

/**
 * @brief Set CAN filter (accept specific message IDs)
 * @param id Message ID to accept
 * @param mask Mask for ID matching
 * @return true if filter set successfully
 */
bool HAL_CAN_SetFilter(uint32_t id, uint32_t mask);

// ============================================================================
// UART (Serial Communication)
// ============================================================================

/**
 * @brief Initialize UART
 * @param port UART port number (0, 1, 2, etc.)
 * @param baud Baud rate (e.g., 115200)
 * @return true if successful
 */
bool HAL_UART_Init(uint8_t port, uint32_t baud);

/**
 * @brief Write data to UART
 * @param port UART port number
 * @param data Pointer to data buffer
 * @param len Number of bytes to write
 * @return Number of bytes written
 */
uint32_t HAL_UART_Write(uint8_t port, const uint8_t* data, uint32_t len);

/**
 * @brief Read data from UART (non-blocking)
 * @param port UART port number
 * @param data Pointer to buffer for received data
 * @param max_len Maximum bytes to read
 * @return Number of bytes read
 */
uint32_t HAL_UART_Read(uint8_t port, uint8_t* data, uint32_t max_len);

/**
 * @brief Check if UART data available
 * @param port UART port number
 * @return Number of bytes available
 */
uint32_t HAL_UART_Available(uint8_t port);

// ============================================================================
// Watchdog Timer
// ============================================================================

/**
 * @brief Initialize hardware watchdog
 *
 * Platform-specific timeout:
 * - STM32F407: 4 seconds (typical)
 * - C2000: Configurable
 * - S32K: Configurable
 */
void HAL_Watchdog_Init();

/**
 * @brief Kick/refresh watchdog timer
 *
 * MUST be called before timeout period expires to prevent reset.
 */
void HAL_Watchdog_Kick();

// ============================================================================
// System Functions
// ============================================================================

/**
 * @brief Initialize HAL system
 *
 * Platform-specific initialization:
 * - Clock configuration
 * - System timer
 * - Interrupt controller
 * - Peripheral power
 */
void HAL_Init();

/**
 * @brief Get system tick count (milliseconds)
 * @return Milliseconds since boot
 */
uint32_t HAL_GetTick();

/**
 * @brief Delay for specified milliseconds
 * @param ms Milliseconds to delay
 */
void HAL_Delay(uint32_t ms);

/**
 * @brief Get unique device ID
 * @param id Pointer to buffer for ID (min 12 bytes)
 * @return Length of ID in bytes
 */
uint8_t HAL_GetDeviceID(uint8_t* id);

#endif // HAL_INTERFACE_H
