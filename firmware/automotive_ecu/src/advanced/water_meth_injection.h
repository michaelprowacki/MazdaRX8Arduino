/**
 * @file water_meth_injection.h
 * @brief Water/methanol injection system for charge cooling and knock suppression
 *
 * Features:
 * - Progressive injection based on boost pressure
 * - Multi-stage injection (low/medium/high boost)
 * - Tank level monitoring with low-level warning
 * - Flow verification (detects clogged nozzles)
 * - Pump control with PWM duty cycle
 * - Failsafe modes (cuts boost if system fails)
 * - Mixture ratio presets (50/50, 100% meth, 100% water)
 * - Optional solenoid staging for multiple nozzles
 *
 * Benefits for Turbocharged Rotaries:
 * - Charge cooling: -40°C intake temps at high boost
 * - Knock suppression: Allows +3-5° more timing
 * - Safe boost increase: +2-3 PSI over dry tune
 * - Reduced EGTs: Exhaust temps drop 50-100°C
 * - Cleaner combustion: Methanol cleans carbon deposits
 * - Power gains: 5-10% from cooling + timing advance
 *
 * Safety Features:
 * - Low tank level warning (cuts boost if empty)
 * - Flow monitoring (detects nozzle clogs)
 * - Pump overheat protection
 * - Gradual pump ramping (prevents pressure spikes)
 * - Failsafe boost reduction
 *
 * @author Created for Phase 5++ Haltech-style features
 * @date 2025-11-16
 */

#ifndef WATER_METH_INJECTION_H
#define WATER_METH_INJECTION_H

#include <stdint.h>

namespace WaterMethInjection {

/**
 * @brief Injection mixture type
 */
enum MixtureType {
    MIXTURE_50_50,          // 50% water, 50% methanol (safest, best cooling)
    MIXTURE_100_METH,       // 100% methanol (max octane boost)
    MIXTURE_100_WATER,      // 100% water (max cooling, distilled only!)
    MIXTURE_30_70,          // 30% water, 70% methanol (aggressive)
    MIXTURE_CUSTOM          // Custom ratio
};

/**
 * @brief Injection stage configuration
 */
struct InjectionStage {
    uint16_t boost_start;       // Boost to start this stage (PSI * 10)
    uint16_t boost_end;         // Boost at max flow (PSI * 10)
    uint8_t  pump_duty_min;     // Pump duty at boost_start (0-100%)
    uint8_t  pump_duty_max;     // Pump duty at boost_end (0-100%)
    bool     solenoid_active;   // Activate solenoid for this stage
};

/**
 * @brief Water/meth injection configuration
 */
struct WMIConfig {
    bool     enabled;                   // System enabled
    MixtureType mixture_type;           // Mixture type
    uint8_t  num_stages;                // Number of stages (1-4)
    InjectionStage stages[4];           // Stage configurations
    uint16_t tank_capacity_ml;          // Tank capacity (ml)
    uint16_t low_level_threshold_ml;    // Low level warning (ml)
    bool     failsafe_enabled;          // Cut boost if system fails
    uint16_t failsafe_boost_limit;      // Max boost in failsafe (PSI * 10)
    bool     flow_monitoring_enabled;   // Monitor flow rate
    uint16_t expected_flow_rate;        // Expected flow at full duty (ml/min)
};

/**
 * @brief Water/meth injection status
 */
struct WMIStatus {
    bool     active;                    // Currently injecting
    uint8_t  active_stage;              // Current stage (0-3, 255=none)
    uint8_t  pump_duty;                 // Current pump duty (0-100%)
    uint16_t current_boost;             // Current boost (PSI * 10)
    uint16_t tank_level_ml;             // Tank level (ml)
    uint8_t  tank_level_percent;        // Tank level (%)
    bool     low_level_warning;         // Low level warning active
    bool     flow_fault;                // Flow fault detected
    bool     failsafe_active;           // Failsafe mode active
    uint32_t total_injected_ml;         // Total injected this session
    uint16_t current_flow_rate;         // Current flow rate (ml/min)
};

/**
 * @brief Initialize water/methanol injection
 */
void init();

/**
 * @brief Update water/meth injection
 *
 * @param boost Current boost pressure (PSI * 10)
 * @param throttle Throttle position (0-100%)
 * @param iat Intake air temperature (°C * 10)
 * @return Current pump duty cycle (0-100%)
 */
uint8_t update(uint16_t boost, uint8_t throttle, int16_t iat);

/**
 * @brief Calculate pump duty for current boost
 *
 * Uses progressive control through multiple stages
 *
 * @param boost Current boost (PSI * 10)
 * @return Pump duty cycle (0-100%)
 */
uint8_t calculatePumpDuty(uint16_t boost);

/**
 * @brief Determine active injection stage
 *
 * @param boost Current boost (PSI * 10)
 * @return Stage index (0-3) or 255 if no stage active
 */
uint8_t determineStage(uint16_t boost);

/**
 * @brief Monitor tank level
 *
 * Uses analog level sensor or flow-based estimation
 *
 * @return Tank level (ml)
 */
uint16_t monitorTankLevel();

/**
 * @brief Monitor flow rate
 *
 * Compares actual flow to expected flow at current duty
 * Detects clogged nozzles or pump failures
 *
 * @param pump_duty Current pump duty
 * @return true if flow is normal
 */
bool monitorFlowRate(uint8_t pump_duty);

/**
 * @brief Check for failsafe conditions
 *
 * Activates failsafe if:
 * - Tank level critically low
 * - Flow fault detected
 * - Pump overheating
 *
 * @return true if failsafe should activate
 */
bool checkFailsafe();

/**
 * @brief Calculate timing advance from water/meth
 *
 * Water/meth injection allows more aggressive timing:
 * - Charge cooling increases knock threshold
 * - Methanol has high octane (109)
 * - Safe to advance 3-5° with injection
 *
 * @param boost Current boost
 * @param pump_duty Current injection duty
 * @return Timing advance (degrees)
 */
int8_t calculateTimingAdvance(uint16_t boost, uint8_t pump_duty);

/**
 * @brief Calculate safe boost increase with water/meth
 *
 * Water/meth allows 2-3 PSI more boost safely
 *
 * @param base_boost Base boost without injection
 * @param pump_duty Current injection duty
 * @return Safe boost increase (PSI * 10)
 */
uint16_t calculateSafeBoostIncrease(uint16_t base_boost, uint8_t pump_duty);

/**
 * @brief Estimate intake temperature reduction
 *
 * Water/meth injection can reduce intake temps by 30-50°C
 *
 * @param iat_before Intake temp before injection (°C * 10)
 * @param pump_duty Current injection duty
 * @return Estimated temp reduction (°C * 10)
 */
int16_t estimateTempReduction(int16_t iat_before, uint8_t pump_duty);

/**
 * @brief Get water/meth status
 * @return Status structure
 */
const WMIStatus* getStatus();

/**
 * @brief Configure water/meth injection
 * @param config Configuration structure
 */
void configure(const WMIConfig& config);

/**
 * @brief Enable/disable water/meth injection
 *
 * @param enabled true to enable
 */
void setEnabled(bool enabled);

/**
 * @brief Set mixture type
 *
 * @param mixture Mixture type
 */
void setMixtureType(MixtureType mixture);

/**
 * @brief Manually set pump duty (testing/override)
 *
 * @param duty Pump duty (0-100%)
 */
void setPumpDutyOverride(uint8_t duty);

/**
 * @brief Clear pump duty override
 */
void clearPumpDutyOverride();

/**
 * @brief Reset tank level to full
 *
 * Call this after refilling tank
 *
 * @param capacity_ml Tank capacity (ml)
 */
void resetTankLevel(uint16_t capacity_ml);

/**
 * @brief Get expected tank range
 *
 * Estimates how much boost/driving time remains
 *
 * @param current_level Tank level (ml)
 * @param avg_flow_rate Average flow rate (ml/min)
 * @return Estimated minutes of boost remaining
 */
uint16_t getEstimatedRange(uint16_t current_level, uint16_t avg_flow_rate);

/**
 * @brief Create default 2-stage configuration
 *
 * Common setup:
 * - Stage 1: 5-10 PSI, low flow
 * - Stage 2: 10+ PSI, high flow
 *
 * @return Default config
 */
WMIConfig createDefault2StageConfig();

/**
 * @brief Create aggressive 4-stage configuration
 *
 * For high-boost applications (15+ PSI)
 *
 * @return Aggressive config
 */
WMIConfig createAggressive4StageConfig();

} // namespace WaterMethInjection

#endif // WATER_METH_INJECTION_H
