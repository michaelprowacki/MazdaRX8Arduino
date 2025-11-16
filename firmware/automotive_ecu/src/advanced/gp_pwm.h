/**
 * @file gp_pwm.h
 * @brief General Purpose PWM outputs for auxiliary devices
 *
 * Features:
 * - Up to 8 configurable PWM outputs
 * - Multiple control modes (RPM, TPS, temp, boost, speed, manual)
 * - Duty cycle mapping with interpolation
 * - Frequency control (10 Hz - 1 kHz)
 * - Fault detection and failsafe
 * - Common presets (radiator fan, fuel pump, shift light)
 *
 * Common Use Cases:
 * - PWM 1: Radiator fan (temp-based, 0-100% @ 80-100°C)
 * - PWM 2: Fuel pump staging (boost-based, on above 10 PSI)
 * - PWM 3: Shift light (RPM-based, flash @ 8500-9000 RPM)
 * - PWM 4: Boost solenoid (manual override)
 * - PWM 5: Warning light (fault-based)
 * - PWM 6: Oil cooler fan (oil temp based)
 * - PWM 7: EGT gauge driver (exhaust temp based)
 * - PWM 8: General purpose (manual control)
 *
 * Safety:
 * - Configurable failsafe duty cycle
 * - Output enable/disable
 * - Over-current detection (if hardware supports)
 * - Diagnostic output
 *
 * @author Created for Phase 5++ Haltech-style features
 * @date 2025-11-16
 */

#ifndef GP_PWM_H
#define GP_PWM_H

#include <stdint.h>

namespace GPPWM {

/**
 * @brief Maximum number of PWM outputs
 */
const uint8_t MAX_PWM_OUTPUTS = 8;

/**
 * @brief PWM control mode
 */
enum ControlMode {
    MODE_DISABLED,          // Output disabled
    MODE_MANUAL,            // Manual duty cycle control
    MODE_RPM,               // RPM-based control
    MODE_TPS,               // Throttle position based
    MODE_COOLANT_TEMP,      // Coolant temperature based
    MODE_OIL_TEMP,          // Oil temperature based
    MODE_BOOST,             // Boost pressure based
    MODE_SPEED,             // Vehicle speed based
    MODE_VOLTAGE,           // Battery voltage based
    MODE_FAULT             // Fault condition (on/off)
};

/**
 * @brief PWM output configuration
 */
struct PWMConfig {
    char name[32];              // Output name (e.g., "Radiator Fan")
    uint8_t pin;                // Arduino pin number
    ControlMode mode;           // Control mode

    // Input range (for automatic modes)
    int16_t input_min;          // Input value for 0% duty
    int16_t input_max;          // Input value for 100% duty

    // Output range
    uint8_t duty_min;           // Minimum duty cycle (0-100%)
    uint8_t duty_max;           // Maximum duty cycle (0-100%)

    // PWM frequency
    uint16_t frequency;         // PWM frequency (Hz)

    // Failsafe
    uint8_t failsafe_duty;      // Duty cycle on fault (0-100%)

    // Smoothing
    uint8_t ramp_rate;          // Duty change rate (%/update, 0 = instant)

    // Enable/disable
    bool enabled;               // Output enabled
    bool invert;                // Invert output (for active-low devices)
};

/**
 * @brief PWM output status
 */
struct PWMStatus {
    uint8_t  current_duty;      // Current duty cycle (0-100%)
    uint8_t  target_duty;       // Target duty cycle (0-100%)
    int16_t  input_value;       // Current input value (for display)
    bool     active;            // Output is active
    bool     fault;             // Fault detected
    uint32_t on_time;           // Total on time (ms)
};

/**
 * @brief Initialize GP PWM system
 */
void init();

/**
 * @brief Update all PWM outputs
 *
 * Call this every loop iteration
 *
 * @param rpm Current RPM
 * @param throttle Throttle position (0-100%)
 * @param coolant_temp Coolant temp (°C * 10)
 * @param oil_temp Oil temp (°C * 10)
 * @param boost Boost pressure (PSI * 10)
 * @param speed Vehicle speed (km/h * 10)
 * @param battery_voltage Battery voltage (V * 100)
 */
void update(uint16_t rpm, uint8_t throttle, int16_t coolant_temp,
            int16_t oil_temp, uint16_t boost, uint16_t speed,
            uint16_t battery_voltage);

/**
 * @brief Configure PWM output
 *
 * @param output Output index (0-7)
 * @param config Configuration
 */
void configure(uint8_t output, const PWMConfig& config);

/**
 * @brief Set manual duty cycle
 *
 * Only works in MODE_MANUAL
 *
 * @param output Output index (0-7)
 * @param duty Duty cycle (0-100%)
 */
void setManualDuty(uint8_t output, uint8_t duty);

/**
 * @brief Enable/disable output
 *
 * @param output Output index (0-7)
 * @param enabled Enable/disable
 */
void setEnabled(uint8_t output, bool enabled);

/**
 * @brief Get PWM status
 *
 * @param output Output index (0-7)
 * @return Status structure (nullptr if invalid)
 */
const PWMStatus* getStatus(uint8_t output);

/**
 * @brief Get PWM configuration
 *
 * @param output Output index (0-7)
 * @return Config structure (nullptr if invalid)
 */
const PWMConfig* getConfig(uint8_t output);

/**
 * @brief Calculate duty cycle for current input
 *
 * @param output Output index (0-7)
 * @param input_value Current input value
 * @return Calculated duty cycle (0-100%)
 */
uint8_t calculateDuty(uint8_t output, int16_t input_value);

/**
 * @brief Apply failsafe to all outputs
 */
void applyFailsafe();

/**
 * @brief Print diagnostic information
 */
void printDiagnostics();

// ============================================================================
// PRESET CONFIGURATIONS
// ============================================================================

/**
 * @brief Create radiator fan config (temp-based)
 *
 * 80°C: 0%
 * 100°C: 100%
 *
 * @param pin Arduino pin
 * @return PWM config
 */
PWMConfig createRadiatorFanConfig(uint8_t pin);

/**
 * @brief Create fuel pump staging config (boost-based)
 *
 * Below 10 PSI: Off
 * Above 10 PSI: On (100%)
 *
 * @param pin Arduino pin
 * @return PWM config
 */
PWMConfig createFuelPumpStagingConfig(uint8_t pin);

/**
 * @brief Create shift light config (RPM-based)
 *
 * Below 8500: Off
 * 8500-9000: Flash (PWM)
 * Above 9000: On
 *
 * @param pin Arduino pin
 * @return PWM config
 */
PWMConfig createShiftLightConfig(uint8_t pin);

/**
 * @brief Create oil cooler fan config (oil temp-based)
 *
 * 90°C: 0%
 * 110°C: 100%
 *
 * @param pin Arduino pin
 * @return PWM config
 */
PWMConfig createOilCoolerFanConfig(uint8_t pin);

/**
 * @brief Create boost gauge driver config (boost-based)
 *
 * 0 PSI: 0%
 * 20 PSI: 100%
 *
 * @param pin Arduino pin
 * @return PWM config
 */
PWMConfig createBoostGaugeConfig(uint8_t pin);

/**
 * @brief Create warning light config (fault-based)
 *
 * No fault: Off
 * Fault: On (100%)
 *
 * @param pin Arduino pin
 * @return PWM config
 */
PWMConfig createWarningLightConfig(uint8_t pin);

} // namespace GPPWM

#endif // GP_PWM_H
