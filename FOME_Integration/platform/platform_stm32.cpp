/*
 * STM32 Platform Implementation (Stub)
 *
 * For STM32F4/F7/H7 with ChibiOS RTOS
 *
 * Requires: ChibiOS HAL, STM32 HAL
 */

#include "platform_abstraction.h"

#ifdef PLATFORM_STM32

// Include ChibiOS headers
#include "ch.h"
#include "hal.h"

// -----------------------------------------
// Time Functions
// -----------------------------------------

uint32_t platform_millis(void) {
    return TIME_I2MS(chVTGetSystemTime());
}

uint32_t platform_micros(void) {
    return TIME_I2US(chVTGetSystemTime());
}

void platform_delay_ms(uint32_t ms) {
    chThdSleepMilliseconds(ms);
}

void platform_delay_us(uint32_t us) {
    chThdSleepMicroseconds(us);
}

// -----------------------------------------
// CAN Bus Functions
// -----------------------------------------

// Configure for CAN1 or CAN2
static CANDriver* canDriver = &CAND1;
static CANConfig canConfig;

bool platform_can_init(uint32_t baudrate) {
    // Calculate bit timing for baudrate
    // Assumes 42MHz APB1 clock (STM32F4)
    canConfig.mcr = 0;
    canConfig.btr = 0;  // Calculate based on baudrate

    switch (baudrate) {
        case 500000:
            // 500kbps: prescaler=6, BS1=6, BS2=1, SJW=1
            canConfig.btr = CAN_BTR_BRP(5) | CAN_BTR_TS1(5) |
                           CAN_BTR_TS2(0) | CAN_BTR_SJW(0);
            break;
        case 250000:
            canConfig.btr = CAN_BTR_BRP(11) | CAN_BTR_TS1(5) |
                           CAN_BTR_TS2(0) | CAN_BTR_SJW(0);
            break;
        default:
            return false;
    }

    canStart(canDriver, &canConfig);
    return true;
}

bool platform_can_send(const CanMessage* msg) {
    CANTxFrame txFrame;

    txFrame.IDE = msg->extended ? CAN_IDE_EXT : CAN_IDE_STD;
    txFrame.RTR = CAN_RTR_DATA;
    txFrame.DLC = msg->length;

    if (msg->extended) {
        txFrame.EID = msg->id;
    } else {
        txFrame.SID = msg->id;
    }

    for (int i = 0; i < msg->length; i++) {
        txFrame.data8[i] = msg->data[i];
    }

    msg_t result = canTransmit(canDriver, CAN_ANY_MAILBOX, &txFrame, TIME_MS2I(10));
    return result == MSG_OK;
}

bool platform_can_receive(CanMessage* msg) {
    CANRxFrame rxFrame;

    msg_t result = canReceive(canDriver, CAN_ANY_MAILBOX, &rxFrame, TIME_IMMEDIATE);

    if (result == MSG_OK) {
        msg->extended = (rxFrame.IDE == CAN_IDE_EXT);
        msg->id = msg->extended ? rxFrame.EID : rxFrame.SID;
        msg->length = rxFrame.DLC;

        for (int i = 0; i < msg->length; i++) {
            msg->data[i] = rxFrame.data8[i];
        }
        return true;
    }

    return false;
}

bool platform_can_available(void) {
    return canReceive(canDriver, CAN_ANY_MAILBOX, NULL, TIME_IMMEDIATE) == MSG_OK;
}

// -----------------------------------------
// GPIO Functions
// -----------------------------------------

void platform_gpio_init(uint8_t pin, GpioMode mode) {
    // Pin mapping depends on board
    // This is a stub - implement based on your board
    (void)pin;
    (void)mode;
}

void platform_gpio_write(uint8_t pin, bool state) {
    // Implement based on board pin mapping
    (void)pin;
    (void)state;
}

bool platform_gpio_read(uint8_t pin) {
    (void)pin;
    return false;
}

uint16_t platform_adc_read(uint8_t channel) {
    // Use STM32 ADC peripheral
    (void)channel;
    return 0;
}

void platform_pwm_write(uint8_t pin, uint8_t duty) {
    (void)pin;
    (void)duty;
}

// -----------------------------------------
// Serial Functions
// -----------------------------------------

static SerialDriver* serialDriver = &SD1;

bool platform_serial_init(uint32_t baudrate) {
    SerialConfig serialConfig = {
        baudrate,
        0,
        USART_CR2_STOP1_BITS,
        0
    };
    sdStart(serialDriver, &serialConfig);
    return true;
}

void platform_serial_write(const uint8_t* data, uint32_t length) {
    sdWrite(serialDriver, data, length);
}

void platform_serial_print(const char* str) {
    while (*str) {
        sdPut(serialDriver, *str++);
    }
}

void platform_serial_println(const char* str) {
    platform_serial_print(str);
    sdPut(serialDriver, '\r');
    sdPut(serialDriver, '\n');
}

int platform_serial_read(uint8_t* data, uint32_t max_length) {
    return sdReadTimeout(serialDriver, data, max_length, TIME_IMMEDIATE);
}

int platform_serial_available(void) {
    return sdGetTimeout(serialDriver, TIME_IMMEDIATE) != Q_TIMEOUT;
}

// -----------------------------------------
// Flash Functions
// -----------------------------------------

bool platform_flash_read(uint32_t address, uint8_t* data, uint32_t length) {
    // Direct memory read from flash
    for (uint32_t i = 0; i < length; i++) {
        data[i] = *(volatile uint8_t*)(address + i);
    }
    return true;
}

bool platform_flash_write(uint32_t address, const uint8_t* data, uint32_t length) {
    // Use STM32 HAL flash programming
    // This is a stub - requires proper flash unlock/program sequence
    (void)address;
    (void)data;
    (void)length;
    return false;
}

bool platform_flash_erase(uint32_t address, uint32_t length) {
    (void)address;
    (void)length;
    return false;
}

// -----------------------------------------
// Memory Functions (for XCP)
// -----------------------------------------

uint8_t platform_mem_read_byte(uint32_t address) {
    return *(volatile uint8_t*)address;
}

void platform_mem_write_byte(uint32_t address, uint8_t value) {
    // Only allow RAM writes
    if (address >= 0x20000000 && address < 0x20020000) {
        *(volatile uint8_t*)address = value;
    }
}

uint16_t platform_mem_read_word(uint32_t address) {
    return *(volatile uint16_t*)address;
}

void platform_mem_write_word(uint32_t address, uint16_t value) {
    if (address >= 0x20000000 && address < 0x20020000) {
        *(volatile uint16_t*)address = value;
    }
}

// -----------------------------------------
// System Functions
// -----------------------------------------

void platform_get_device_id(uint8_t* id, uint8_t length) {
    // STM32 unique ID at 0x1FFF7A10
    uint32_t* uid = (uint32_t*)0x1FFF7A10;
    for (uint8_t i = 0; i < length && i < 12; i++) {
        id[i] = ((uint8_t*)uid)[i];
    }
}

void platform_reset(void) {
    NVIC_SystemReset();
}

void platform_enter_bootloader(void) {
    // Set boot pin or flag, then reset
    NVIC_SystemReset();
}

uint32_t platform_get_reset_reason(void) {
    return RCC->CSR;
}

void platform_enable_interrupts(void) {
    chSysUnlock();
}

void platform_disable_interrupts(void) {
    chSysLock();
}

// -----------------------------------------
// Watchdog Functions
// -----------------------------------------

void platform_watchdog_init(uint32_t timeout_ms) {
    // Configure IWDG
    (void)timeout_ms;
}

void platform_watchdog_feed(void) {
    // Reload IWDG
    IWDG->KR = 0xAAAA;
}

#endif // PLATFORM_STM32
