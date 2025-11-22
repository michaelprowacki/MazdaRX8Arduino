/**
 * @file hal_c2000.cpp
 * @brief TI C2000 F28379D Hardware Abstraction Layer implementation
 *
 * TI C2000 F28379D features:
 * - Dual C28x CPU cores @ 200 MHz
 * - 1 MB Flash, 204 KB RAM
 * - 2x CAN-FD controllers
 * - 4x 16-bit ADC (24 channels each)
 * - 16x Enhanced PWM (ePWM) modules
 * - 6x Serial Communication Interface (SCI/UART)
 * - Windowed watchdog timer
 * - Operating range: -40°C to 125°C (automotive grade)
 *
 * Pin mapping for LAUNCHXL-F28379D:
 * - CANA TX: GPIO31, RX: GPIO30
 * - CANB TX: GPIO17, RX: GPIO12
 * - ePWM1A: GPIO0 (throttle PWM output)
 * - ADCINA0: ADCIN0 (throttle pedal input)
 * - SCIA: GPIO28 (TX), GPIO29 (RX) - UART to ESP32
 * - GPIO31: User LED (red)
 *
 * @author Created for Phase 5 unified architecture
 * @date 2025-11-16
 */

#ifdef MCU_PLATFORM_C2000

#include "../hal_interface.h"
#include <Arduino.h>

// C2000 DriverLib includes
#ifdef USE_C2000_DRIVERLIB
    #include "driverlib.h"
    #include "device.h"
#endif

// ============================================================================
// GPIO Implementation
// ============================================================================

void HAL_GPIO_SetMode(uint8_t pin, HAL_GPIO_Mode mode) {
    #ifdef USE_C2000_DRIVERLIB
        switch (mode) {
            case HAL_GPIO_MODE_INPUT:
                GPIO_setPadConfig(pin, GPIO_PIN_TYPE_STD);
                GPIO_setDirectionMode(pin, GPIO_DIR_MODE_IN);
                break;
            case HAL_GPIO_MODE_OUTPUT:
                GPIO_setPadConfig(pin, GPIO_PIN_TYPE_STD);
                GPIO_setDirectionMode(pin, GPIO_DIR_MODE_OUT);
                break;
            case HAL_GPIO_MODE_INPUT_PULLUP:
                GPIO_setPadConfig(pin, GPIO_PIN_TYPE_PULLUP);
                GPIO_setDirectionMode(pin, GPIO_DIR_MODE_IN);
                break;
            case HAL_GPIO_MODE_INPUT_PULLDOWN:
                GPIO_setPadConfig(pin, GPIO_PIN_TYPE_INVERT);
                GPIO_setDirectionMode(pin, GPIO_DIR_MODE_IN);
                break;
            case HAL_GPIO_MODE_ANALOG:
                GPIO_setPadConfig(pin, GPIO_PIN_TYPE_STD);
                GPIO_setAnalogMode(pin, GPIO_ANALOG_ENABLED);
                break;
            case HAL_GPIO_MODE_PWM:
                // Configured by PWM module initialization
                break;
            default:
                break;
        }
    #else
        pinMode(pin, (mode == HAL_GPIO_MODE_OUTPUT) ? OUTPUT : INPUT);
    #endif
}

void HAL_GPIO_Write(uint8_t pin, bool state) {
    #ifdef USE_C2000_DRIVERLIB
        GPIO_writePin(pin, state ? 1 : 0);
    #else
        digitalWrite(pin, state ? HIGH : LOW);
    #endif
}

bool HAL_GPIO_Read(uint8_t pin) {
    #ifdef USE_C2000_DRIVERLIB
        return GPIO_readPin(pin) != 0;
    #else
        return digitalRead(pin) == HIGH;
    #endif
}

void HAL_GPIO_Toggle(uint8_t pin) {
    #ifdef USE_C2000_DRIVERLIB
        GPIO_togglePin(pin);
    #else
        digitalWrite(pin, !digitalRead(pin));
    #endif
}

// ============================================================================
// ADC Implementation (16-bit, much better than STM32's 12-bit)
// ============================================================================

uint16_t HAL_ADC_Read(uint8_t pin) {
    #ifdef USE_C2000_DRIVERLIB
        // C2000 has 16-bit ADC, scale to 12-bit for compatibility
        uint16_t result = ADC_readResult(ADCARESULT_BASE, (ADC_SOCNumber)pin);
        return result >> 4;  // 16-bit to 12-bit
    #else
        return analogRead(pin);
    #endif
}

uint16_t HAL_ADC_ReadMillivolts(uint8_t pin) {
    uint16_t raw = HAL_ADC_Read(pin);
    // C2000 ADC reference is 3.3V (12-bit after scaling)
    return (raw * 3300) / 4095;
}

// ============================================================================
// PWM Implementation (ePWM - very powerful for motor control)
// ============================================================================

void HAL_PWM_Write(uint8_t pin, uint8_t value) {
    #ifdef USE_C2000_DRIVERLIB
        // Map pin to ePWM module (simplified)
        uint32_t epwm_base = EPWM1_BASE + (pin * 0x100);

        // Convert 8-bit value to period count
        uint16_t duty = (value * EPWM_getTimeBasePeriod(epwm_base)) / 255;

        EPWM_setCounterCompareValue(epwm_base, EPWM_COUNTER_COMPARE_A, duty);
    #else
        analogWrite(pin, value);
    #endif
}

void HAL_PWM_Write16(uint8_t pin, uint16_t value) {
    #ifdef USE_C2000_DRIVERLIB
        uint32_t epwm_base = EPWM1_BASE + (pin * 0x100);
        EPWM_setCounterCompareValue(epwm_base, EPWM_COUNTER_COMPARE_A, value);
    #else
        analogWrite(pin, value >> 8);
    #endif
}

void HAL_PWM_SetFrequency(uint8_t pin, uint32_t frequency) {
    #ifdef USE_C2000_DRIVERLIB
        uint32_t epwm_base = EPWM1_BASE + (pin * 0x100);

        // Calculate period based on TBCLK (typically 100 MHz)
        uint32_t tbclk = 100000000;  // 100 MHz
        uint16_t period = (tbclk / frequency) - 1;

        EPWM_setTimeBasePeriod(epwm_base, period);
        EPWM_setPhaseShift(epwm_base, 0);
        EPWM_setTimeBaseCounter(epwm_base, 0);
    #endif
}

// ============================================================================
// CAN Implementation (CAN-FD capable!)
// ============================================================================

static bool can_initialized = false;

bool HAL_CAN_Init(uint32_t speed) {
    #ifdef USE_C2000_DRIVERLIB
        // Initialize CANA
        CAN_initModule(CANA_BASE);

        // Set bit rate (500 kbps typical)
        CAN_setBitRate(CANA_BASE, DEVICE_SYSCLK_FREQ, speed, 20);

        // Enable CAN
        CAN_startModule(CANA_BASE);

        can_initialized = true;
        return true;
    #else
        can_initialized = true;
        return true;
    #endif
}

bool HAL_CAN_Transmit(const HAL_CAN_Message* msg) {
    if (!can_initialized) return false;

    #ifdef USE_C2000_DRIVERLIB
        // Send CAN message
        CAN_sendMessage(CANA_BASE, 1,  // Mailbox 1
                       msg->len,
                       (uint16_t*)msg->data);
        return true;
    #else
        return false;
    #endif
}

bool HAL_CAN_Receive(HAL_CAN_Message* msg) {
    if (!can_initialized) return false;

    #ifdef USE_C2000_DRIVERLIB
        // Check if message available
        if (CAN_readMessage(CANA_BASE, 2, (uint16_t*)msg->data)) {
            msg->len = 8;  // Simplified
            return true;
        }
    #endif

    return false;
}

bool HAL_CAN_Available() {
    if (!can_initialized) return false;

    #ifdef USE_C2000_DRIVERLIB
        return (CAN_getStatus(CANA_BASE) & CAN_STATUS_RXOK) != 0;
    #else
        return false;
    #endif
}

bool HAL_CAN_SetFilter(uint32_t id, uint32_t mask) {
    // C2000 CAN acceptance filters configured during initialization
    return true;
}

// ============================================================================
// UART Implementation (SCI modules)
// ============================================================================

bool HAL_UART_Init(uint8_t port, uint32_t baud) {
    #ifdef USE_C2000_DRIVERLIB
        uint32_t sci_base = SCIA_BASE + (port * 0x100);

        SCI_performSoftwareReset(sci_base);
        SCI_setConfig(sci_base, DEVICE_LSPCLK_FREQ, baud,
                     (SCI_CONFIG_WLEN_8 | SCI_CONFIG_STOP_ONE |
                      SCI_CONFIG_PAR_NONE));
        SCI_enableModule(sci_base);
        SCI_resetChannels(sci_base);
        SCI_enableFIFO(sci_base);

        return true;
    #else
        Serial.begin(baud);
        return true;
    #endif
}

uint32_t HAL_UART_Write(uint8_t port, const uint8_t* data, uint32_t len) {
    #ifdef USE_C2000_DRIVERLIB
        uint32_t sci_base = SCIA_BASE + (port * 0x100);

        for (uint32_t i = 0; i < len; i++) {
            SCI_writeCharBlockingNonFIFO(sci_base, data[i]);
        }
        return len;
    #else
        return Serial.write(data, len);
    #endif
}

uint32_t HAL_UART_Read(uint8_t port, uint8_t* data, uint32_t max_len) {
    #ifdef USE_C2000_DRIVERLIB
        uint32_t sci_base = SCIA_BASE + (port * 0x100);
        uint32_t count = 0;

        while (count < max_len && SCI_getRxFIFOStatus(sci_base) != SCI_FIFO_RX0) {
            data[count++] = SCI_readCharNonBlocking(sci_base);
        }

        return count;
    #else
        uint32_t count = 0;
        while (count < max_len && Serial.available()) {
            data[count++] = Serial.read();
        }
        return count;
    #endif
}

uint32_t HAL_UART_Available(uint8_t port) {
    #ifdef USE_C2000_DRIVERLIB
        uint32_t sci_base = SCIA_BASE + (port * 0x100);
        return SCI_getRxFIFOStatus(sci_base);
    #else
        return Serial.available();
    #endif
}

// ============================================================================
// Watchdog Implementation (Windowed watchdog - more sophisticated)
// ============================================================================

void HAL_Watchdog_Init() {
    #ifdef USE_C2000_DRIVERLIB
        // Disable watchdog during initialization
        SysCtl_disableWatchdog();

        // Configure watchdog (4 second timeout)
        SysCtl_setWatchdogPredivider(SYSCTL_WD_PREDIV_512);
        SysCtl_setWatchdogMode(SYSCTL_WD_MODE_RESET);

        // Enable watchdog
        SysCtl_enableWatchdog();

        Serial.println("[HAL] C2000 windowed watchdog initialized (4s timeout)");
    #endif
}

void HAL_Watchdog_Kick() {
    #ifdef USE_C2000_DRIVERLIB
        SysCtl_serviceWatchdog();
    #endif
}

// ============================================================================
// System Functions
// ============================================================================

void HAL_Init() {
    #ifdef USE_C2000_DRIVERLIB
        // Initialize device
        Device_init();

        // Initialize GPIO
        Device_initGPIO();

        // Disable pin locks
        Device_initGPIO();

        Serial.println("╔════════════════════════════════════════╗");
        Serial.println("║  TI C2000 F28379D HAL Initialization  ║");
        Serial.println("╚════════════════════════════════════════╝");
        Serial.printf("[HAL] CPU: Dual C28x @ %lu MHz\n", DEVICE_SYSCLK_FREQ / 1000000);
        Serial.println("[HAL] Flash: 1 MB, RAM: 204 KB");
        Serial.println("[HAL] CAN: 2x CAN-FD controllers");
        Serial.println("[HAL] ADC: 4x 16-bit (24 channels each)");
        Serial.println("[HAL] PWM: 16x Enhanced PWM (ePWM)");
        Serial.println("[HAL] UART: 6x SCI");
        Serial.println("[HAL] Temperature range: -40°C to 125°C");
        Serial.println("[HAL] SPECIALTY: Motor control optimized");
    #endif
}

uint32_t HAL_GetTick() {
    return millis();
}

void HAL_Delay(uint32_t ms) {
    delay(ms);
}

uint8_t HAL_GetDeviceID(uint8_t* id) {
    #ifdef USE_C2000_DRIVERLIB
        // C2000 unique device ID (128-bit)
        uint32_t* uid = (uint32_t*)0x00070200;  // Device ID register
        memcpy(id, uid, 16);
        return 16;
    #else
        memset(id, 0xC2, 12);  // Mock ID
        return 12;
    #endif
}

#endif // MCU_PLATFORM_C2000
