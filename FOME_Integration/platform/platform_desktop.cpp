/*
 * Desktop Platform Implementation
 *
 * For testing on Linux/macOS/Windows
 */

#include "platform_abstraction.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Simulated memory for XCP
static uint8_t simulated_memory[0x10000];

// Start time
static struct timespec start_time;
static bool initialized = false;

static void init_time() {
    if (!initialized) {
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        initialized = true;
    }
}

// -----------------------------------------
// Time Functions
// -----------------------------------------

uint32_t platform_millis(void) {
    init_time();
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    uint64_t ms = (now.tv_sec - start_time.tv_sec) * 1000 +
                  (now.tv_nsec - start_time.tv_nsec) / 1000000;
    return (uint32_t)ms;
}

uint32_t platform_micros(void) {
    init_time();
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    uint64_t us = (now.tv_sec - start_time.tv_sec) * 1000000 +
                  (now.tv_nsec - start_time.tv_nsec) / 1000;
    return (uint32_t)us;
}

void platform_delay_ms(uint32_t ms) {
    usleep(ms * 1000);
}

void platform_delay_us(uint32_t us) {
    usleep(us);
}

// -----------------------------------------
// CAN Bus Functions (Stub)
// -----------------------------------------

bool platform_can_init(uint32_t baudrate) {
    (void)baudrate;
    printf("[CAN] Initialized at %u bps (simulated)\n", baudrate);
    return true;
}

bool platform_can_send(const CanMessage* msg) {
    printf("[CAN TX] ID=0x%03X Len=%d Data=", msg->id, msg->length);
    for (int i = 0; i < msg->length; i++) {
        printf("%02X ", msg->data[i]);
    }
    printf("\n");
    return true;
}

bool platform_can_receive(CanMessage* msg) {
    (void)msg;
    return false;  // No messages in simulation
}

bool platform_can_available(void) {
    return false;
}

// -----------------------------------------
// GPIO Functions (Stub)
// -----------------------------------------

void platform_gpio_init(uint8_t pin, GpioMode mode) {
    (void)pin;
    (void)mode;
}

void platform_gpio_write(uint8_t pin, bool state) {
    (void)pin;
    (void)state;
}

bool platform_gpio_read(uint8_t pin) {
    (void)pin;
    return false;
}

uint16_t platform_adc_read(uint8_t channel) {
    (void)channel;
    return 2048;  // Mid-scale
}

void platform_pwm_write(uint8_t pin, uint8_t duty) {
    (void)pin;
    (void)duty;
}

// -----------------------------------------
// Serial Functions
// -----------------------------------------

bool platform_serial_init(uint32_t baudrate) {
    (void)baudrate;
    return true;
}

void platform_serial_write(const uint8_t* data, uint32_t length) {
    fwrite(data, 1, length, stdout);
}

void platform_serial_print(const char* str) {
    printf("%s", str);
}

void platform_serial_println(const char* str) {
    printf("%s\n", str);
}

int platform_serial_read(uint8_t* data, uint32_t max_length) {
    (void)data;
    (void)max_length;
    return 0;
}

int platform_serial_available(void) {
    return 0;
}

// -----------------------------------------
// Flash Functions (Simulated)
// -----------------------------------------

bool platform_flash_read(uint32_t address, uint8_t* data, uint32_t length) {
    if (address + length > sizeof(simulated_memory)) {
        return false;
    }
    memcpy(data, &simulated_memory[address], length);
    return true;
}

bool platform_flash_write(uint32_t address, const uint8_t* data, uint32_t length) {
    if (address + length > sizeof(simulated_memory)) {
        return false;
    }
    memcpy(&simulated_memory[address], data, length);
    return true;
}

bool platform_flash_erase(uint32_t address, uint32_t length) {
    if (address + length > sizeof(simulated_memory)) {
        return false;
    }
    memset(&simulated_memory[address], 0xFF, length);
    return true;
}

// -----------------------------------------
// Memory Functions (for XCP)
// -----------------------------------------

uint8_t platform_mem_read_byte(uint32_t address) {
    if (address < sizeof(simulated_memory)) {
        return simulated_memory[address];
    }
    return 0xFF;
}

void platform_mem_write_byte(uint32_t address, uint8_t value) {
    if (address < sizeof(simulated_memory)) {
        simulated_memory[address] = value;
    }
}

uint16_t platform_mem_read_word(uint32_t address) {
    if (address + 1 < sizeof(simulated_memory)) {
        return (simulated_memory[address] << 8) | simulated_memory[address + 1];
    }
    return 0xFFFF;
}

void platform_mem_write_word(uint32_t address, uint16_t value) {
    if (address + 1 < sizeof(simulated_memory)) {
        simulated_memory[address] = (value >> 8) & 0xFF;
        simulated_memory[address + 1] = value & 0xFF;
    }
}

// -----------------------------------------
// System Functions
// -----------------------------------------

void platform_get_device_id(uint8_t* id, uint8_t length) {
    memset(id, 0x12, length);  // Fake ID
}

void platform_reset(void) {
    printf("[SYSTEM] Reset requested\n");
}

void platform_enter_bootloader(void) {
    printf("[SYSTEM] Bootloader entry requested\n");
}

uint32_t platform_get_reset_reason(void) {
    return 0;  // Power-on reset
}

void platform_enable_interrupts(void) {
    // No-op on desktop
}

void platform_disable_interrupts(void) {
    // No-op on desktop
}

// -----------------------------------------
// Watchdog Functions
// -----------------------------------------

void platform_watchdog_init(uint32_t timeout_ms) {
    (void)timeout_ms;
}

void platform_watchdog_feed(void) {
    // No-op on desktop
}
