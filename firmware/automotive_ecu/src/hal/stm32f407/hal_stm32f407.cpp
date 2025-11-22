/**
 * @file hal_stm32f407.cpp
 * @brief STM32F407 Hardware Abstraction Layer implementation
 *
 * STM32F407 features:
 * - ARM Cortex-M4F @ 168 MHz
 * - 1 MB Flash, 192 KB RAM
 * - 2x CAN controllers (CAN1, CAN2)
 * - 3x ADC (12-bit, 16 channels each)
 * - 14x Timers (including advanced PWM)
 * - 6x USART/UART
 * - Independent watchdog timer
 * - Operating range: -40°C to 85°C (industrial grade available)
 *
 * Pin mapping for Nucleo-F407ZG:
 * - CAN1 TX: PB9, RX: PB8
 * - CAN2 TX: PB13, RX: PB12
 * - ADC1_IN0: PA0 (throttle pedal input)
 * - TIM1_CH1: PA8 (throttle PWM output)
 * - USART2: PA2 (TX), PA3 (RX) - UART to ESP32
 * - User LED: PC13 (onboard)
 *
 * @author Created for unified automotive ECU
 * @date 2025-11-16
 */

#ifdef MCU_PLATFORM_STM32F407

#include "../hal_interface.h"
#include <Arduino.h>

// STM32 HAL includes (if using STM32Cube HAL)
#ifdef USE_STM32_HAL
    #include <stm32f4xx_hal.h>
#endif

// CAN library for STM32
#include <STM32_CAN.h>

// ============================================================================
// GPIO Implementation
// ============================================================================

void HAL_GPIO_SetMode(uint8_t pin, HAL_GPIO_Mode mode) {
    switch (mode) {
        case HAL_GPIO_MODE_INPUT:
            pinMode(pin, INPUT);
            break;
        case HAL_GPIO_MODE_OUTPUT:
            pinMode(pin, OUTPUT);
            break;
        case HAL_GPIO_MODE_INPUT_PULLUP:
            pinMode(pin, INPUT_PULLUP);
            break;
        case HAL_GPIO_MODE_INPUT_PULLDOWN:
            pinMode(pin, INPUT_PULLDOWN);
            break;
        case HAL_GPIO_MODE_ANALOG:
            pinMode(pin, INPUT_ANALOG);
            break;
        case HAL_GPIO_MODE_PWM:
            pinMode(pin, PWM);
            break;
        case HAL_GPIO_MODE_ALTERNATE:
            // Handled by peripheral initialization
            break;
    }
}

void HAL_GPIO_Write(uint8_t pin, bool state) {
    digitalWrite(pin, state ? HIGH : LOW);
}

bool HAL_GPIO_Read(uint8_t pin) {
    return digitalRead(pin) == HIGH;
}

void HAL_GPIO_Toggle(uint8_t pin) {
    digitalWrite(pin, !digitalRead(pin));
}

// ============================================================================
// ADC Implementation
// ============================================================================

void HAL_ADC_Read(uint8_t pin) {
    return analogRead(pin);
}

uint16_t HAL_ADC_ReadMillivolts(uint8_t pin) {
    uint16_t raw = analogRead(pin);
    // STM32F407 ADC is 12-bit (0-4095), 3.3V reference
    return (raw * 3300) / 4095;
}

// ============================================================================
// PWM Implementation
// ============================================================================

void HAL_PWM_Write(uint8_t pin, uint8_t value) {
    analogWrite(pin, value);
}

void HAL_PWM_Write16(uint8_t pin, uint16_t value) {
    // STM32 Arduino core supports 16-bit PWM on some timers
    analogWrite(pin, value >> 8);  // Scale to 8-bit for compatibility
}

void HAL_PWM_SetFrequency(uint8_t pin, uint32_t frequency) {
    // Platform-specific PWM frequency setting
    // Note: Requires timer configuration
    // For now, use default frequency (~1 kHz typical)
}

// ============================================================================
// CAN Implementation
// ============================================================================

static STM32_CAN* can1 = nullptr;

bool HAL_CAN_Init(uint32_t speed) {
    // Initialize CAN1 on STM32F407
    // Pins: PB9 (TX), PB8 (RX)

    if (!can1) {
        can1 = new STM32_CAN(CAN1, DEF);  // CAN1 peripheral, default pins
    }

    // Map speed to CAN baud rate
    CAN_BaudRate baud;
    switch (speed) {
        case 125000:  baud = CAN_125KBPS; break;
        case 250000:  baud = CAN_250KBPS; break;
        case 500000:  baud = CAN_500KBPS; break;
        case 1000000: baud = CAN_1000KBPS; break;
        default:      baud = CAN_500KBPS; break;  // Default to 500 kbps
    }

    can1->begin();
    can1->setBaudRate(baud);

    return true;
}

bool HAL_CAN_Transmit(const HAL_CAN_Message* msg) {
    if (!can1) return false;

    CAN_message_t can_msg;
    can_msg.id = msg->id;
    can_msg.len = msg->len;
    can_msg.flags.extended = msg->extended ? 1 : 0;
    memcpy(can_msg.buf, msg->data, msg->len);

    return can1->write(can_msg) == 1;
}

bool HAL_CAN_Receive(HAL_CAN_Message* msg) {
    if (!can1) return false;

    CAN_message_t can_msg;
    if (can1->read(can_msg)) {
        msg->id = can_msg.id;
        msg->len = can_msg.len;
        msg->extended = can_msg.flags.extended;
        memcpy(msg->data, can_msg.buf, can_msg.len);
        return true;
    }

    return false;
}

bool HAL_CAN_Available() {
    return can1 && can1->available();
}

bool HAL_CAN_SetFilter(uint32_t id, uint32_t mask) {
    if (!can1) return false;

    // STM32 CAN filter configuration
    // Note: STM32 has complex filter banks, simplified here
    CAN_filter_t filter;
    filter.id = id;
    filter.mask = mask;
    filter.flags.extended = (id > 0x7FF) ? 1 : 0;

    can1->setFilter(filter, 0);  // Filter bank 0
    return true;
}

// ============================================================================
// UART Implementation
// ============================================================================

// UART instances (USART2 = Serial2 for ESP32 bridge)
static HardwareSerial* uart_ports[6] = {
    &Serial,   // USART1 (USB virtual COM)
    &Serial2,  // USART2 (ESP32 bridge, PA2/PA3)
    &Serial3,  // USART3
    nullptr,   // UART4
    nullptr,   // UART5
    nullptr    // USART6
};

bool HAL_UART_Init(uint8_t port, uint32_t baud) {
    if (port >= 6 || !uart_ports[port]) return false;

    uart_ports[port]->begin(baud);
    return true;
}

uint32_t HAL_UART_Write(uint8_t port, const uint8_t* data, uint32_t len) {
    if (port >= 6 || !uart_ports[port]) return 0;

    return uart_ports[port]->write(data, len);
}

uint32_t HAL_UART_Read(uint8_t port, uint8_t* data, uint32_t max_len) {
    if (port >= 6 || !uart_ports[port]) return 0;

    uint32_t count = 0;
    while (count < max_len && uart_ports[port]->available()) {
        data[count++] = uart_ports[port]->read();
    }

    return count;
}

uint32_t HAL_UART_Available(uint8_t port) {
    if (port >= 6 || !uart_ports[port]) return 0;

    return uart_ports[port]->available();
}

// ============================================================================
// Watchdog Implementation
// ============================================================================

#include <IWatchdog.h>

void HAL_Watchdog_Init() {
    // Initialize independent watchdog (IWDG)
    // Timeout: 4 seconds (typical for STM32F407)
    IWatchdog.begin(4000000);  // 4,000,000 microseconds = 4 seconds

    #ifdef ENABLE_SERIAL_DEBUG
        Serial.println("[HAL] STM32F407 watchdog initialized (4s timeout)");
    #endif
}

void HAL_Watchdog_Kick() {
    IWatchdog.reload();
}

// ============================================================================
// System Functions
// ============================================================================

void HAL_Init() {
    // Initialize system clocks, peripherals, etc.
    // Most initialization handled by Arduino core

    #ifdef ENABLE_SERIAL_DEBUG
        Serial.println("╔════════════════════════════════════════╗");
        Serial.println("║  STM32F407 HAL Initialization         ║");
        Serial.println("╚════════════════════════════════════════╝");
        Serial.printf("[HAL] CPU: ARM Cortex-M4F @ %lu MHz\n", SystemCoreClock / 1000000);
        Serial.println("[HAL] Flash: 1 MB, RAM: 192 KB");
        Serial.println("[HAL] CAN: 2x controllers");
        Serial.println("[HAL] ADC: 3x 12-bit");
        Serial.println("[HAL] Timers: 14x (advanced PWM)");
        Serial.println("[HAL] UART: 6x");
        Serial.println("[HAL] Temperature range: -40°C to 85°C");
    #endif
}

uint32_t HAL_GetTick() {
    return millis();
}

void HAL_Delay(uint32_t ms) {
    delay(ms);
}

uint8_t HAL_GetDeviceID(uint8_t* id) {
    // STM32F407 unique device ID (96-bit)
    // Located at 0x1FFF7A10
    uint32_t* uid = (uint32_t*)0x1FFF7A10;

    memcpy(id, uid, 12);
    return 12;
}

#endif // MCU_PLATFORM_STM32F407
