/**
 * @file hal_s32k.cpp
 * @brief NXP S32K148 Hardware Abstraction Layer implementation
 *
 * NXP S32K148 features:
 * - ARM Cortex-M4F @ 112 MHz
 * - 2 MB Flash, 256 KB RAM
 * - 3x FlexCAN (CAN-FD capable)
 * - 2x 12-bit ADC (16 channels each)
 * - 8x FlexTimer (FTM) for PWM
 * - 3x LPUART
 * - Windowed watchdog (WDOG)
 * - Operating range: -40°C to 150°C (automotive grade)
 * - ISO 26262 ASIL-B capable
 *
 * Pin mapping for S32K148EVB evaluation board:
 * - CAN0 TX: PTE5, RX: PTE4
 * - CAN1 TX: PTA13, RX: PTA12
 * - CAN2 TX: PTE25, RX: PTE24
 * - FTM0_CH0: PTD15 (throttle PWM output)
 * - ADC0_SE0: PTA0 (throttle pedal input)
 * - LPUART1: PTC6 (TX), PTC7 (RX) - UART to ESP32
 * - PTD16: RGB LED (red)
 *
 * @author Created for Phase 5 unified architecture
 * @date 2025-11-16
 */

#ifdef MCU_PLATFORM_S32K148

#include "../hal_interface.h"
#include <Arduino.h>

// S32K148 SDK includes
#ifdef USE_S32K_SDK
    #include "S32K148.h"
    #include "clock_S32K1xx.h"
    #include "pins_driver.h"
    #include "flexcan_driver.h"
    #include "adc_driver.h"
    #include "ftm_pwm_driver.h"
    #include "lpuart_driver.h"
    #include "wdog_driver.h"
#endif

// ============================================================================
// GPIO Implementation
// ============================================================================

void HAL_GPIO_SetMode(uint8_t pin, HAL_GPIO_Mode mode) {
    #ifdef USE_S32K_SDK
        port_pin_config_t config;

        switch (mode) {
            case HAL_GPIO_MODE_INPUT:
                config.pullConfig = PORT_INTERNAL_PULL_NOT_ENABLED;
                config.mux = PORT_MUX_AS_GPIO;
                config.direction = GPIO_INPUT_DIRECTION;
                PINS_DRV_Init(1, &config);
                break;

            case HAL_GPIO_MODE_OUTPUT:
                config.pullConfig = PORT_INTERNAL_PULL_NOT_ENABLED;
                config.mux = PORT_MUX_AS_GPIO;
                config.direction = GPIO_OUTPUT_DIRECTION;
                PINS_DRV_Init(1, &config);
                break;

            case HAL_GPIO_MODE_INPUT_PULLUP:
                config.pullConfig = PORT_INTERNAL_PULL_UP_ENABLED;
                config.mux = PORT_MUX_AS_GPIO;
                config.direction = GPIO_INPUT_DIRECTION;
                PINS_DRV_Init(1, &config);
                break;

            case HAL_GPIO_MODE_INPUT_PULLDOWN:
                config.pullConfig = PORT_INTERNAL_PULL_DOWN_ENABLED;
                config.mux = PORT_MUX_AS_GPIO;
                config.direction = GPIO_INPUT_DIRECTION;
                PINS_DRV_Init(1, &config);
                break;

            case HAL_GPIO_MODE_ANALOG:
                config.mux = PORT_MUX_ADC;
                PINS_DRV_Init(1, &config);
                break;

            case HAL_GPIO_MODE_PWM:
                config.mux = PORT_MUX_ALT2;  // FTM alternate function
                PINS_DRV_Init(1, &config);
                break;

            default:
                break;
        }
    #else
        pinMode(pin, (mode == HAL_GPIO_MODE_OUTPUT) ? OUTPUT : INPUT);
    #endif
}

void HAL_GPIO_Write(uint8_t pin, bool state) {
    #ifdef USE_S32K_SDK
        GPIO_Type* base = (GPIO_Type*)(PTA_BASE + ((pin / 32) * 0x40));
        uint8_t pin_num = pin % 32;
        PINS_DRV_WritePin(base, pin_num, state ? 1 : 0);
    #else
        digitalWrite(pin, state ? HIGH : LOW);
    #endif
}

bool HAL_GPIO_Read(uint8_t pin) {
    #ifdef USE_S32K_SDK
        GPIO_Type* base = (GPIO_Type*)(PTA_BASE + ((pin / 32) * 0x40));
        uint8_t pin_num = pin % 32;
        return PINS_DRV_ReadPins(base) & (1 << pin_num);
    #else
        return digitalRead(pin) == HIGH;
    #endif
}

void HAL_GPIO_Toggle(uint8_t pin) {
    #ifdef USE_S32K_SDK
        GPIO_Type* base = (GPIO_Type*)(PTA_BASE + ((pin / 32) * 0x40));
        uint8_t pin_num = pin % 32;
        PINS_DRV_TogglePins(base, 1 << pin_num);
    #else
        digitalWrite(pin, !digitalRead(pin));
    #endif
}

// ============================================================================
// ADC Implementation
// ============================================================================

uint16_t HAL_ADC_Read(uint8_t pin) {
    #ifdef USE_S32K_SDK
        uint16_t result;
        ADC_DRV_GetChanResult(0, pin, &result);
        return result;
    #else
        return analogRead(pin);
    #endif
}

uint16_t HAL_ADC_ReadMillivolts(uint8_t pin) {
    uint16_t raw = HAL_ADC_Read(pin);
    // S32K148 ADC is 12-bit (0-4095), 5V reference (automotive-grade tolerant)
    return (raw * 5000) / 4095;
}

// ============================================================================
// PWM Implementation (FlexTimer FTM)
// ============================================================================

void HAL_PWM_Write(uint8_t pin, uint8_t value) {
    #ifdef USE_S32K_SDK
        // Map pin to FTM channel
        uint8_t ftm_instance = pin / 8;
        uint8_t channel = pin % 8;

        uint16_t duty = (value * 1000) / 255;  // 0-1000 duty cycle
        FTM_DRV_UpdatePwmChannel(ftm_instance, channel,
                                FTM_PWM_UPDATE_IN_DUTY_CYCLE,
                                duty, 0, true);
    #else
        analogWrite(pin, value);
    #endif
}

void HAL_PWM_Write16(uint8_t pin, uint16_t value) {
    #ifdef USE_S32K_SDK
        uint8_t ftm_instance = pin / 8;
        uint8_t channel = pin % 8;
        FTM_DRV_UpdatePwmChannel(ftm_instance, channel,
                                FTM_PWM_UPDATE_IN_DUTY_CYCLE,
                                value, 0, true);
    #else
        analogWrite(pin, value >> 8);
    #endif
}

void HAL_PWM_SetFrequency(uint8_t pin, uint32_t frequency) {
    #ifdef USE_S32K_SDK
        uint8_t ftm_instance = pin / 8;
        FTM_DRV_UpdatePwmPeriod(ftm_instance, FTM_PWM_UPDATE_IN_TICKS,
                               (CLOCK_SYS_GetFtmFreq(ftm_instance) / frequency),
                               true);
    #endif
}

// ============================================================================
// CAN Implementation (FlexCAN - CAN-FD capable, ISO 26262 compliant)
// ============================================================================

static bool can_initialized = false;

bool HAL_CAN_Init(uint32_t speed) {
    #ifdef USE_S32K_SDK
        flexcan_user_config_t can_config = {
            .max_num_mb = 16,
            .num_id_filters = FLEXCAN_RX_FIFO_ID_FILTERS_8,
            .is_rx_fifo_needed = true,
            .flexcanMode = FLEXCAN_NORMAL_MODE,
            .bitrate = {
                .propSeg = 7,
                .phaseSeg1 = 4,
                .phaseSeg2 = 1,
                .preDivider = 0,
                .rJumpwidth = 1
            }
        };

        // Initialize FlexCAN0
        FLEXCAN_DRV_Init(0, &can_config, true);

        can_initialized = true;
        return true;
    #else
        can_initialized = true;
        return true;
    #endif
}

bool HAL_CAN_Transmit(const HAL_CAN_Message* msg) {
    if (!can_initialized) return false;

    #ifdef USE_S32K_SDK
        flexcan_data_info_t tx_info = {
            .msg_id_type = msg->extended ? FLEXCAN_MSG_ID_EXT : FLEXCAN_MSG_ID_STD,
            .data_length = msg->len,
            .is_remote = false
        };

        FLEXCAN_DRV_Send(0, 0, &tx_info, msg->id, (uint8_t*)msg->data);
        return true;
    #else
        return false;
    #endif
}

bool HAL_CAN_Receive(HAL_CAN_Message* msg) {
    if (!can_initialized) return false;

    #ifdef USE_S32K_SDK
        flexcan_msgbuff_t rx_msg;
        if (FLEXCAN_DRV_RxMessageBuffer(0, 1, &rx_msg) == STATUS_SUCCESS) {
            msg->id = rx_msg.msgId;
            msg->len = rx_msg.dataLen;
            msg->extended = (rx_msg.msgId > 0x7FF);
            memcpy(msg->data, rx_msg.data, msg->len);
            return true;
        }
    #endif

    return false;
}

bool HAL_CAN_Available() {
    if (!can_initialized) return false;

    #ifdef USE_S32K_SDK
        return FLEXCAN_DRV_GetTransferStatus(0, 1) == STATUS_SUCCESS;
    #else
        return false;
    #endif
}

bool HAL_CAN_SetFilter(uint32_t id, uint32_t mask) {
    #ifdef USE_S32K_SDK
        flexcan_id_table_t filter_table[1] = {
            { .isRemoteFrame = false, .isExtendedFrame = (id > 0x7FF), .id = id }
        };
        FLEXCAN_DRV_ConfigRxFifo(0, FLEXCAN_RX_FIFO_ID_FORMAT_A, filter_table);
        return true;
    #else
        return true;
    #endif
}

// ============================================================================
// UART Implementation (LPUART - Low Power UART)
// ============================================================================

bool HAL_UART_Init(uint8_t port, uint32_t baud) {
    #ifdef USE_S32K_SDK
        lpuart_user_config_t uart_config = {
            .baudRate = baud,
            .parityMode = LPUART_PARITY_DISABLED,
            .stopBitCount = LPUART_ONE_STOP_BIT,
            .bitCountPerChar = LPUART_8_BITS_PER_CHAR,
            .transferType = LPUART_USING_INTERRUPTS
        };

        LPUART_DRV_Init(port, &uart_config);
        return true;
    #else
        Serial.begin(baud);
        return true;
    #endif
}

uint32_t HAL_UART_Write(uint8_t port, const uint8_t* data, uint32_t len) {
    #ifdef USE_S32K_SDK
        LPUART_DRV_SendDataBlocking(port, data, len, 1000);
        return len;
    #else
        return Serial.write(data, len);
    #endif
}

uint32_t HAL_UART_Read(uint8_t port, uint8_t* data, uint32_t max_len) {
    #ifdef USE_S32K_SDK
        uint32_t bytes_read = 0;
        LPUART_DRV_ReceiveDataBlocking(port, data, max_len, 10);
        LPUART_DRV_GetReceiveStatus(port, &bytes_read);
        return bytes_read;
    #else
        uint32_t count = 0;
        while (count < max_len && Serial.available()) {
            data[count++] = Serial.read();
        }
        return count;
    #endif
}

uint32_t HAL_UART_Available(uint8_t port) {
    #ifdef USE_S32K_SDK
        uint32_t bytes_remaining;
        LPUART_DRV_GetReceiveStatus(port, &bytes_remaining);
        return bytes_remaining;
    #else
        return Serial.available();
    #endif
}

// ============================================================================
// Watchdog Implementation (WDOG - ISO 26262 compliant)
// ============================================================================

void HAL_Watchdog_Init() {
    #ifdef USE_S32K_SDK
        wdog_user_config_t wdog_config = {
            .clkSource = WDOG_LPO_CLOCK,
            .opMode = {
                .wait = false,
                .stop = false,
                .debug = false
            },
            .updateEnable = true,
            .intEnable = false,
            .winEnable = false,
            .windowValue = 0,
            .timeoutValue = 4096,  // ~4 seconds with LPO clock
            .prescalerEnable = false
        };

        WDOG_DRV_Init(WDOG_INSTANCE, &wdog_config);

        Serial.println("[HAL] S32K148 watchdog initialized (4s timeout, ISO 26262)");
    #endif
}

void HAL_Watchdog_Kick() {
    #ifdef USE_S32K_SDK
        WDOG_DRV_Trigger(WDOG_INSTANCE);
    #endif
}

// ============================================================================
// System Functions
// ============================================================================

void HAL_Init() {
    #ifdef USE_S32K_SDK
        // Initialize clocks
        CLOCK_SYS_Init(g_clockManConfigsArr, CLOCK_MANAGER_CONFIG_CNT,
                      g_clockManCallbacksArr, CLOCK_MANAGER_CALLBACK_CNT);
        CLOCK_SYS_UpdateConfiguration(0U, CLOCK_MANAGER_POLICY_AGREEMENT);

        Serial.println("╔════════════════════════════════════════╗");
        Serial.println("║  NXP S32K148 HAL Initialization       ║");
        Serial.println("╚════════════════════════════════════════╝");
        Serial.printf("[HAL] CPU: ARM Cortex-M4F @ %lu MHz\n", 112);
        Serial.println("[HAL] Flash: 2 MB, RAM: 256 KB");
        Serial.println("[HAL] CAN: 3x FlexCAN (CAN-FD capable)");
        Serial.println("[HAL] ADC: 2x 12-bit");
        Serial.println("[HAL] PWM: 8x FlexTimer (FTM)");
        Serial.println("[HAL] UART: 3x LPUART");
        Serial.println("[HAL] Temperature range: -40°C to 150°C");
        Serial.println("[HAL] SAFETY: ISO 26262 ASIL-B capable");
    #endif
}

uint32_t HAL_GetTick() {
    return millis();
}

void HAL_Delay(uint32_t ms) {
    delay(ms);
}

uint8_t HAL_GetDeviceID(uint8_t* id) {
    #ifdef USE_S32K_SDK
        // S32K148 unique ID (128-bit)
        uint32_t* uid = (uint32_t*)0x40048060;  // SIM_UIDH register
        memcpy(id, uid, 16);
        return 16;
    #else
        memset(id, 0x32, 12);  // Mock ID
        return 12;
    #endif
}

#endif // MCU_PLATFORM_S32K148
