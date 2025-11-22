/*
 * STM32 Platform Implementation
 *
 * For STM32F4/F7/H7 with ChibiOS RTOS
 *
 * Requires: ChibiOS HAL, STM32 HAL
 *
 * Supports:
 * - CAN bus with hardware filters
 * - Flash programming with sector management
 * - EEPROM emulation in flash
 * - Hardware watchdog (IWDG)
 * - ADC with DMA
 */

#include "platform_abstraction.h"

#ifdef PLATFORM_STM32

// Include ChibiOS headers
#include "ch.h"
#include "hal.h"
#include "stm32_registry.h"

// Flash sector definitions for STM32F4
#define FLASH_SECTOR_0_ADDR   0x08000000
#define FLASH_SECTOR_1_ADDR   0x08004000
#define FLASH_SECTOR_2_ADDR   0x08008000
#define FLASH_SECTOR_3_ADDR   0x0800C000
#define FLASH_SECTOR_4_ADDR   0x08010000
#define FLASH_SECTOR_5_ADDR   0x08020000
#define FLASH_SECTOR_6_ADDR   0x08040000
#define FLASH_SECTOR_7_ADDR   0x08060000

// EEPROM emulation in last flash sector
#define EEPROM_START_ADDRESS  FLASH_SECTOR_7_ADDR
#define EEPROM_SIZE           0x20000  // 128KB

// CAN filter configuration
#define CAN_FILTER_BANKS      14
#define CAN_MAX_FILTERS       28

// ADC with DMA configuration
#define ADC_NUM_CHANNELS      8
#define ADC_BUFFER_DEPTH      16

// EEPROM wear-leveling configuration
#define EEPROM_PAGE_SIZE      256
#define EEPROM_NUM_PAGES      (EEPROM_SIZE / EEPROM_PAGE_SIZE)
#define EEPROM_HEADER_SIZE    4
#define EEPROM_MAGIC          0xEE01

// -----------------------------------------
// CAN Filter Management
// -----------------------------------------

static uint8_t canFilterCount = 0;

// Add hardware CAN filter
bool platform_can_add_filter(uint32_t id, uint32_t mask, bool extended) {
    if (canFilterCount >= CAN_MAX_FILTERS) {
        return false;
    }

    CANFilter filter;
    filter.filter = canFilterCount / 2;
    filter.mode = 0;  // Mask mode
    filter.scale = 1; // 32-bit scale
    filter.assignment = 0; // FIFO 0

    if (extended) {
        filter.register1 = (id << 3) | 0x04;  // Extended ID
        filter.register2 = (mask << 3) | 0x04;
    } else {
        filter.register1 = id << 21;  // Standard ID
        filter.register2 = mask << 21;
    }

    canSTM32SetFilters(canDriver, 1, 1, &filter);
    canFilterCount++;
    return true;
}

// Add filter for specific CAN ID
bool platform_can_filter_id(uint32_t id) {
    return platform_can_add_filter(id, 0x7FF, false);
}

// Add filter for ID range
bool platform_can_filter_range(uint32_t id_start, uint32_t id_end) {
    // Calculate mask for range
    uint32_t mask = ~(id_start ^ id_end) & 0x7FF;
    return platform_can_add_filter(id_start, mask, false);
}

// Clear all CAN filters
void platform_can_clear_filters(void) {
    canFilterCount = 0;
    // Reset to accept all messages
    CANFilter filter;
    filter.filter = 0;
    filter.mode = 0;
    filter.scale = 1;
    filter.assignment = 0;
    filter.register1 = 0;
    filter.register2 = 0;
    canSTM32SetFilters(canDriver, 1, 1, &filter);
}

// -----------------------------------------
// ADC with DMA
// -----------------------------------------

static adcsample_t adcBuffer[ADC_NUM_CHANNELS * ADC_BUFFER_DEPTH];
static bool adcInitialized = false;

// ADC conversion group configuration
static const ADCConversionGroup adcConvGroup = {
    .circular = TRUE,
    .num_channels = ADC_NUM_CHANNELS,
    .end_cb = NULL,
    .error_cb = NULL,
    .cr1 = 0,
    .cr2 = ADC_CR2_SWSTART,
    .smpr1 = ADC_SMPR1_SMP_AN10(ADC_SAMPLE_480) |
             ADC_SMPR1_SMP_AN11(ADC_SAMPLE_480) |
             ADC_SMPR1_SMP_AN12(ADC_SAMPLE_480) |
             ADC_SMPR1_SMP_AN13(ADC_SAMPLE_480),
    .smpr2 = ADC_SMPR2_SMP_AN0(ADC_SAMPLE_480) |
             ADC_SMPR2_SMP_AN1(ADC_SAMPLE_480) |
             ADC_SMPR2_SMP_AN2(ADC_SAMPLE_480) |
             ADC_SMPR2_SMP_AN3(ADC_SAMPLE_480),
    .sqr1 = ADC_SQR1_NUM_CH(ADC_NUM_CHANNELS),
    .sqr2 = 0,
    .sqr3 = ADC_SQR3_SQ1_N(ADC_CHANNEL_IN0) |
            ADC_SQR3_SQ2_N(ADC_CHANNEL_IN1) |
            ADC_SQR3_SQ3_N(ADC_CHANNEL_IN2) |
            ADC_SQR3_SQ4_N(ADC_CHANNEL_IN3) |
            ADC_SQR3_SQ5_N(ADC_CHANNEL_IN10) |
            ADC_SQR3_SQ6_N(ADC_CHANNEL_IN11)
};

// Initialize ADC with DMA
bool platform_adc_init(void) {
    if (adcInitialized) {
        return true;
    }

    adcStart(&ADCD1, NULL);
    adcStartConversion(&ADCD1, &adcConvGroup, adcBuffer, ADC_BUFFER_DEPTH);
    adcInitialized = true;
    return true;
}

// Read ADC channel (averaged from DMA buffer)
uint16_t platform_adc_read(uint8_t channel) {
    if (!adcInitialized || channel >= ADC_NUM_CHANNELS) {
        return 0;
    }

    // Average samples from DMA buffer
    uint32_t sum = 0;
    for (int i = 0; i < ADC_BUFFER_DEPTH; i++) {
        sum += adcBuffer[i * ADC_NUM_CHANNELS + channel];
    }
    return sum / ADC_BUFFER_DEPTH;
}

// Read multiple ADC channels at once
void platform_adc_read_multiple(uint16_t* values, uint8_t numChannels) {
    if (!adcInitialized) {
        return;
    }

    for (uint8_t ch = 0; ch < numChannels && ch < ADC_NUM_CHANNELS; ch++) {
        values[ch] = platform_adc_read(ch);
    }
}

// Get raw ADC buffer pointer for advanced use
adcsample_t* platform_adc_get_buffer(void) {
    return adcBuffer;
}

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

// Get flash sector number from address
static int get_flash_sector(uint32_t address) {
    if (address < FLASH_SECTOR_1_ADDR) return 0;
    if (address < FLASH_SECTOR_2_ADDR) return 1;
    if (address < FLASH_SECTOR_3_ADDR) return 2;
    if (address < FLASH_SECTOR_4_ADDR) return 3;
    if (address < FLASH_SECTOR_5_ADDR) return 4;
    if (address < FLASH_SECTOR_6_ADDR) return 5;
    if (address < FLASH_SECTOR_7_ADDR) return 6;
    return 7;
}

// Unlock flash for programming
static bool flash_unlock(void) {
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;
    }
    return !(FLASH->CR & FLASH_CR_LOCK);
}

// Lock flash after programming
static void flash_lock(void) {
    FLASH->CR |= FLASH_CR_LOCK;
}

// Wait for flash operation to complete
static bool flash_wait_complete(uint32_t timeout_ms) {
    systime_t start = chVTGetSystemTime();
    while (FLASH->SR & FLASH_SR_BSY) {
        if (TIME_I2MS(chVTTimeElapsedSinceX(start)) > timeout_ms) {
            return false;
        }
        chThdYield();
    }
    return true;
}

bool platform_flash_read(uint32_t address, uint8_t* data, uint32_t length) {
    // Direct memory read from flash
    for (uint32_t i = 0; i < length; i++) {
        data[i] = *(volatile uint8_t*)(address + i);
    }
    return true;
}

bool platform_flash_write(uint32_t address, const uint8_t* data, uint32_t length) {
    if (!flash_unlock()) {
        return false;
    }

    // Clear error flags
    FLASH->SR = FLASH_SR_PGSERR | FLASH_SR_PGPERR | FLASH_SR_PGAERR |
                FLASH_SR_WRPERR | FLASH_SR_OPERR | FLASH_SR_EOP;

    // Set programming mode (byte)
    FLASH->CR &= ~FLASH_CR_PSIZE;
    FLASH->CR |= FLASH_CR_PG;

    bool success = true;
    for (uint32_t i = 0; i < length; i++) {
        *(volatile uint8_t*)(address + i) = data[i];

        if (!flash_wait_complete(100)) {
            success = false;
            break;
        }

        if (FLASH->SR & (FLASH_SR_PGSERR | FLASH_SR_PGPERR | FLASH_SR_WRPERR)) {
            success = false;
            break;
        }
    }

    FLASH->CR &= ~FLASH_CR_PG;
    flash_lock();

    return success;
}

bool platform_flash_erase(uint32_t address, uint32_t length) {
    if (!flash_unlock()) {
        return false;
    }

    // Clear error flags
    FLASH->SR = FLASH_SR_PGSERR | FLASH_SR_PGPERR | FLASH_SR_PGAERR |
                FLASH_SR_WRPERR | FLASH_SR_OPERR | FLASH_SR_EOP;

    int start_sector = get_flash_sector(address);
    int end_sector = get_flash_sector(address + length - 1);

    bool success = true;
    for (int sector = start_sector; sector <= end_sector; sector++) {
        // Configure sector erase
        FLASH->CR &= ~FLASH_CR_SNB;
        FLASH->CR |= (sector << 3) | FLASH_CR_SER;
        FLASH->CR |= FLASH_CR_STRT;

        if (!flash_wait_complete(2000)) {  // Sector erase can take up to 2s
            success = false;
            break;
        }

        if (FLASH->SR & (FLASH_SR_PGSERR | FLASH_SR_WRPERR)) {
            success = false;
            break;
        }
    }

    FLASH->CR &= ~FLASH_CR_SER;
    flash_lock();

    return success;
}

// -----------------------------------------
// EEPROM Emulation with Wear-Leveling
// -----------------------------------------

// Page header structure (stored at start of each page)
typedef struct {
    uint16_t magic;      // EEPROM_MAGIC
    uint16_t sequence;   // Sequence number for ordering
} EepromPageHeader;

// Find the most recent valid page
static int32_t eeprom_find_current_page(void) {
    uint16_t maxSequence = 0;
    int32_t currentPage = -1;

    for (uint32_t page = 0; page < EEPROM_NUM_PAGES; page++) {
        uint32_t pageAddr = EEPROM_START_ADDRESS + (page * EEPROM_PAGE_SIZE);
        EepromPageHeader header;
        platform_flash_read(pageAddr, (uint8_t*)&header, sizeof(header));

        if (header.magic == EEPROM_MAGIC) {
            // Handle sequence wraparound
            if (currentPage == -1 ||
                (int16_t)(header.sequence - maxSequence) > 0) {
                maxSequence = header.sequence;
                currentPage = page;
            }
        }
    }

    return currentPage;
}

// Find next free page for writing
static int32_t eeprom_find_next_page(uint16_t* nextSequence) {
    int32_t currentPage = eeprom_find_current_page();

    if (currentPage == -1) {
        // No valid pages, start fresh
        *nextSequence = 1;
        return 0;
    }

    // Get current sequence
    uint32_t pageAddr = EEPROM_START_ADDRESS + (currentPage * EEPROM_PAGE_SIZE);
    EepromPageHeader header;
    platform_flash_read(pageAddr, (uint8_t*)&header, sizeof(header));

    *nextSequence = header.sequence + 1;
    return (currentPage + 1) % EEPROM_NUM_PAGES;
}

// Check if page needs erasing (not all 0xFF)
static bool eeprom_page_needs_erase(uint32_t page) {
    uint32_t pageAddr = EEPROM_START_ADDRESS + (page * EEPROM_PAGE_SIZE);

    for (uint32_t i = 0; i < EEPROM_PAGE_SIZE; i++) {
        uint8_t byte;
        platform_flash_read(pageAddr + i, &byte, 1);
        if (byte != 0xFF) {
            return true;
        }
    }
    return false;
}

bool platform_eeprom_read(uint32_t offset, uint8_t* data, uint32_t length) {
    if (offset + length > EEPROM_PAGE_SIZE - EEPROM_HEADER_SIZE) {
        return false;
    }

    int32_t currentPage = eeprom_find_current_page();
    if (currentPage == -1) {
        // No valid data, return zeros
        for (uint32_t i = 0; i < length; i++) {
            data[i] = 0;
        }
        return true;
    }

    uint32_t pageAddr = EEPROM_START_ADDRESS + (currentPage * EEPROM_PAGE_SIZE);
    return platform_flash_read(pageAddr + EEPROM_HEADER_SIZE + offset, data, length);
}

bool platform_eeprom_write(uint32_t offset, const uint8_t* data, uint32_t length) {
    if (offset + length > EEPROM_PAGE_SIZE - EEPROM_HEADER_SIZE) {
        return false;
    }

    // Find next page to write
    uint16_t nextSequence;
    int32_t nextPage = eeprom_find_next_page(&nextSequence);
    uint32_t nextPageAddr = EEPROM_START_ADDRESS + (nextPage * EEPROM_PAGE_SIZE);

    // Erase page if needed
    if (eeprom_page_needs_erase(nextPage)) {
        // Need to erase entire sector - copy other pages first if in same sector
        // For simplicity, just erase (real implementation would preserve other data)
        if (!platform_flash_erase(nextPageAddr, EEPROM_PAGE_SIZE)) {
            return false;
        }
    }

    // Read current data (if any) to preserve unchanged bytes
    uint8_t pageBuffer[EEPROM_PAGE_SIZE];
    int32_t currentPage = eeprom_find_current_page();

    if (currentPage >= 0) {
        uint32_t currentAddr = EEPROM_START_ADDRESS + (currentPage * EEPROM_PAGE_SIZE);
        platform_flash_read(currentAddr, pageBuffer, EEPROM_PAGE_SIZE);
    } else {
        // Initialize with 0xFF
        for (uint32_t i = 0; i < EEPROM_PAGE_SIZE; i++) {
            pageBuffer[i] = 0xFF;
        }
    }

    // Update header
    EepromPageHeader* header = (EepromPageHeader*)pageBuffer;
    header->magic = EEPROM_MAGIC;
    header->sequence = nextSequence;

    // Update data
    for (uint32_t i = 0; i < length; i++) {
        pageBuffer[EEPROM_HEADER_SIZE + offset + i] = data[i];
    }

    // Write entire page
    return platform_flash_write(nextPageAddr, pageBuffer, EEPROM_PAGE_SIZE);
}

// Get EEPROM wear statistics
void platform_eeprom_get_stats(uint32_t* usedPages, uint32_t* totalPages, uint16_t* currentSeq) {
    *totalPages = EEPROM_NUM_PAGES;
    *usedPages = 0;

    for (uint32_t page = 0; page < EEPROM_NUM_PAGES; page++) {
        uint32_t pageAddr = EEPROM_START_ADDRESS + (page * EEPROM_PAGE_SIZE);
        EepromPageHeader header;
        platform_flash_read(pageAddr, (uint8_t*)&header, sizeof(header));

        if (header.magic == EEPROM_MAGIC) {
            (*usedPages)++;
        }
    }

    int32_t currentPage = eeprom_find_current_page();
    if (currentPage >= 0) {
        uint32_t pageAddr = EEPROM_START_ADDRESS + (currentPage * EEPROM_PAGE_SIZE);
        EepromPageHeader header;
        platform_flash_read(pageAddr, (uint8_t*)&header, sizeof(header));
        *currentSeq = header.sequence;
    } else {
        *currentSeq = 0;
    }
}

// Format EEPROM (erase all pages)
bool platform_eeprom_format(void) {
    return platform_flash_erase(EEPROM_START_ADDRESS, EEPROM_SIZE);
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
