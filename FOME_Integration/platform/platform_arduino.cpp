/*
 * Arduino Platform Implementation
 *
 * For Arduino Leonardo/Mega with MCP2515 CAN controller
 * Compatible with original RX8 Arduino project
 */

#ifdef PLATFORM_ARDUINO

#include "platform_abstraction.h"
#include <Arduino.h>
#include <mcp_can.h>
#include <SPI.h>
#include <EEPROM.h>

// MCP2515 configuration
#ifndef CAN_CS_PIN
#define CAN_CS_PIN 17
#endif

#ifndef CAN_INT_PIN
#define CAN_INT_PIN 2
#endif

static MCP_CAN CAN(CAN_CS_PIN);
static bool canInitialized = false;

// -----------------------------------------
// Time Functions
// -----------------------------------------

uint32_t platform_millis(void) {
    return millis();
}

uint32_t platform_micros(void) {
    return micros();
}

void platform_delay_ms(uint32_t ms) {
    delay(ms);
}

void platform_delay_us(uint32_t us) {
    delayMicroseconds(us);
}

// -----------------------------------------
// CAN Bus Functions
// -----------------------------------------

bool platform_can_init(uint32_t baudrate) {
    uint8_t canSpeed;

    switch (baudrate) {
        case 500000:
            canSpeed = CAN_500KBPS;
            break;
        case 250000:
            canSpeed = CAN_250KBPS;
            break;
        case 125000:
            canSpeed = CAN_125KBPS;
            break;
        default:
            canSpeed = CAN_500KBPS;
    }

    // Initialize MCP2515 with 16MHz crystal
    if (CAN.begin(MCP_ANY, canSpeed, MCP_16MHZ) == CAN_OK) {
        CAN.setMode(MCP_NORMAL);
        pinMode(CAN_INT_PIN, INPUT);
        canInitialized = true;
        return true;
    }

    return false;
}

bool platform_can_send(const CanMessage* msg) {
    if (!canInitialized) return false;

    byte result = CAN.sendMsgBuf(
        msg->id,
        msg->extended ? 1 : 0,
        msg->length,
        (byte*)msg->data
    );

    return (result == CAN_OK);
}

bool platform_can_receive(CanMessage* msg) {
    if (!canInitialized) return false;

    if (CAN_MSGAVAIL != CAN.checkReceive()) {
        return false;
    }

    unsigned long id;
    byte len;
    byte ext;

    CAN.readMsgBuf(&id, &ext, &len, msg->data);

    msg->id = id;
    msg->length = len;
    msg->extended = (ext == 1);

    return true;
}

bool platform_can_available(void) {
    if (!canInitialized) return false;
    return (CAN_MSGAVAIL == CAN.checkReceive());
}

// -----------------------------------------
// GPIO Functions
// -----------------------------------------

void platform_gpio_init(uint8_t pin, GpioMode mode) {
    switch (mode) {
        case GPIO_MODE_INPUT:
            pinMode(pin, INPUT);
            break;
        case GPIO_MODE_OUTPUT:
            pinMode(pin, OUTPUT);
            break;
        case GPIO_MODE_INPUT_PULLUP:
            pinMode(pin, INPUT_PULLUP);
            break;
        default:
            pinMode(pin, INPUT);
    }
}

void platform_gpio_write(uint8_t pin, bool state) {
    digitalWrite(pin, state ? HIGH : LOW);
}

bool platform_gpio_read(uint8_t pin) {
    return digitalRead(pin) == HIGH;
}

uint16_t platform_adc_read(uint8_t channel) {
    // Arduino ADC is 10-bit, scale to 12-bit
    return analogRead(channel) << 2;
}

void platform_pwm_write(uint8_t pin, uint8_t duty) {
    analogWrite(pin, duty);
}

// -----------------------------------------
// Serial Functions
// -----------------------------------------

bool platform_serial_init(uint32_t baudrate) {
    Serial.begin(baudrate);
    return true;
}

void platform_serial_write(const uint8_t* data, uint32_t length) {
    Serial.write(data, length);
}

void platform_serial_print(const char* str) {
    Serial.print(str);
}

void platform_serial_println(const char* str) {
    Serial.println(str);
}

int platform_serial_read(uint8_t* data, uint32_t max_length) {
    int count = 0;
    while (Serial.available() && count < (int)max_length) {
        data[count++] = Serial.read();
    }
    return count;
}

int platform_serial_available(void) {
    return Serial.available();
}

// -----------------------------------------
// Flash/EEPROM Functions
// -----------------------------------------

bool platform_flash_read(uint32_t address, uint8_t* data, uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
        data[i] = EEPROM.read(address + i);
    }
    return true;
}

bool platform_flash_write(uint32_t address, const uint8_t* data, uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
        EEPROM.write(address + i, data[i]);
    }
    return true;
}

bool platform_flash_erase(uint32_t address, uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
        EEPROM.write(address + i, 0xFF);
    }
    return true;
}

// -----------------------------------------
// Memory Functions (for XCP)
// -----------------------------------------

uint8_t platform_mem_read_byte(uint32_t address) {
    // On AVR, direct memory access
    return *(volatile uint8_t*)address;
}

void platform_mem_write_byte(uint32_t address, uint8_t value) {
    // Only allow SRAM writes
    if (address >= 0x100 && address < 0x1100) {  // AVR SRAM range
        *(volatile uint8_t*)address = value;
    }
}

uint16_t platform_mem_read_word(uint32_t address) {
    return *(volatile uint16_t*)address;
}

void platform_mem_write_word(uint32_t address, uint16_t value) {
    if (address >= 0x100 && address < 0x1100) {
        *(volatile uint16_t*)address = value;
    }
}

// -----------------------------------------
// System Functions
// -----------------------------------------

void platform_get_device_id(uint8_t* id, uint8_t length) {
    // Arduino doesn't have unique ID, use EEPROM serial
    for (uint8_t i = 0; i < length; i++) {
        id[i] = EEPROM.read(0x7F0 + i);  // Use last bytes of EEPROM
    }
}

void platform_reset(void) {
    // Software reset via watchdog
    void (*resetFunc)(void) = 0;
    resetFunc();
}

void platform_enter_bootloader(void) {
    // For Leonardo, jump to bootloader
    #ifdef __AVR_ATmega32U4__
    cli();
    UDCON = 1;
    USBCON = (1 << FRZCLK);
    UCSR1B = 0;
    _delay_ms(5);
    EIMSK = 0;
    PCICR = 0;
    SPCR = 0;
    ACSR = 0;
    EECR = 0;
    ADCSRA = 0;
    TIMSK0 = 0;
    TIMSK1 = 0;
    TIMSK3 = 0;
    TIMSK4 = 0;
    UCSR1B = 0;
    TWCR = 0;
    DDRB = 0;
    DDRC = 0;
    DDRD = 0;
    DDRE = 0;
    DDRF = 0;
    PORTB = 0;
    PORTC = 0;
    PORTD = 0;
    PORTE = 0;
    PORTF = 0;
    asm volatile("jmp 0x7000");
    #endif
}

uint32_t platform_get_reset_reason(void) {
    return MCUSR;
}

void platform_enable_interrupts(void) {
    sei();
}

void platform_disable_interrupts(void) {
    cli();
}

// -----------------------------------------
// Watchdog Functions
// -----------------------------------------

#include <avr/wdt.h>

void platform_watchdog_init(uint32_t timeout_ms) {
    uint8_t wdtConfig;

    if (timeout_ms <= 15) {
        wdtConfig = WDTO_15MS;
    } else if (timeout_ms <= 30) {
        wdtConfig = WDTO_30MS;
    } else if (timeout_ms <= 60) {
        wdtConfig = WDTO_60MS;
    } else if (timeout_ms <= 120) {
        wdtConfig = WDTO_120MS;
    } else if (timeout_ms <= 250) {
        wdtConfig = WDTO_250MS;
    } else if (timeout_ms <= 500) {
        wdtConfig = WDTO_500MS;
    } else if (timeout_ms <= 1000) {
        wdtConfig = WDTO_1S;
    } else if (timeout_ms <= 2000) {
        wdtConfig = WDTO_2S;
    } else {
        wdtConfig = WDTO_4S;
    }

    wdt_enable(wdtConfig);
}

void platform_watchdog_feed(void) {
    wdt_reset();
}

// -----------------------------------------
// FOME CAN Transmit Bridge
// -----------------------------------------

// This is called by vehicle profiles
extern "C" void fomeCanTransmit(uint32_t id, uint8_t* data, uint8_t length) {
    CanMessage msg;
    msg.id = id;
    msg.length = length;
    msg.extended = false;
    memcpy(msg.data, data, length);
    platform_can_send(&msg);
}

#endif // PLATFORM_ARDUINO
