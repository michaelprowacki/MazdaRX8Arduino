/*
 * Platform Abstraction Layer
 *
 * Provides unified interface for platform-specific functions.
 * Implement platform_*.cpp for each target (STM32, Arduino, Desktop)
 */

#ifndef PLATFORM_ABSTRACTION_H
#define PLATFORM_ABSTRACTION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------
// Time Functions
// -----------------------------------------

// Get milliseconds since boot
uint32_t platform_millis(void);

// Get microseconds since boot
uint32_t platform_micros(void);

// Delay in milliseconds
void platform_delay_ms(uint32_t ms);

// Delay in microseconds
void platform_delay_us(uint32_t us);

// -----------------------------------------
// CAN Bus Functions
// -----------------------------------------

typedef struct {
    uint32_t id;
    uint8_t data[8];
    uint8_t length;
    bool extended;
} CanMessage;

// Initialize CAN bus
bool platform_can_init(uint32_t baudrate);

// Send CAN message
bool platform_can_send(const CanMessage* msg);

// Receive CAN message (non-blocking)
bool platform_can_receive(CanMessage* msg);

// Check if CAN message available
bool platform_can_available(void);

// -----------------------------------------
// GPIO Functions
// -----------------------------------------

typedef enum {
    GPIO_MODE_INPUT,
    GPIO_MODE_OUTPUT,
    GPIO_MODE_INPUT_PULLUP,
    GPIO_MODE_INPUT_PULLDOWN,
    GPIO_MODE_ANALOG
} GpioMode;

// Configure GPIO pin
void platform_gpio_init(uint8_t pin, GpioMode mode);

// Set digital output
void platform_gpio_write(uint8_t pin, bool state);

// Read digital input
bool platform_gpio_read(uint8_t pin);

// Read analog input (returns 0-4095)
uint16_t platform_adc_read(uint8_t channel);

// Set PWM output (0-255 duty cycle)
void platform_pwm_write(uint8_t pin, uint8_t duty);

// -----------------------------------------
// Serial/UART Functions
// -----------------------------------------

// Initialize serial port
bool platform_serial_init(uint32_t baudrate);

// Send data
void platform_serial_write(const uint8_t* data, uint32_t length);

// Print string
void platform_serial_print(const char* str);

// Print with newline
void platform_serial_println(const char* str);

// Read available bytes
int platform_serial_read(uint8_t* data, uint32_t max_length);

// Check bytes available
int platform_serial_available(void);

// -----------------------------------------
// Flash/EEPROM Functions
// -----------------------------------------

// Read from non-volatile storage
bool platform_flash_read(uint32_t address, uint8_t* data, uint32_t length);

// Write to non-volatile storage
bool platform_flash_write(uint32_t address, const uint8_t* data, uint32_t length);

// Erase sector
bool platform_flash_erase(uint32_t address, uint32_t length);

// -----------------------------------------
// Memory Functions (for XCP)
// -----------------------------------------

// Read byte from memory (for XCP upload)
uint8_t platform_mem_read_byte(uint32_t address);

// Write byte to memory (for XCP download)
void platform_mem_write_byte(uint32_t address, uint8_t value);

// Read word from memory
uint16_t platform_mem_read_word(uint32_t address);

// Write word to memory
void platform_mem_write_word(uint32_t address, uint16_t value);

// -----------------------------------------
// System Functions
// -----------------------------------------

// Get unique device ID
void platform_get_device_id(uint8_t* id, uint8_t length);

// Reset device
void platform_reset(void);

// Enter bootloader
void platform_enter_bootloader(void);

// Get reset reason
uint32_t platform_get_reset_reason(void);

// Enable/disable interrupts
void platform_enable_interrupts(void);
void platform_disable_interrupts(void);

// -----------------------------------------
// Watchdog Functions
// -----------------------------------------

// Initialize watchdog
void platform_watchdog_init(uint32_t timeout_ms);

// Feed/reset watchdog
void platform_watchdog_feed(void);

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_ABSTRACTION_H
