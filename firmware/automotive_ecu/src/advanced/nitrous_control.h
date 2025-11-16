/**
 * @file nitrous_control.h
 * @brief Progressive multi-stage nitrous oxide injection system
 *
 * Features:
 * - Up to 6 progressive stages
 * - Wet or dry system support
 * - RPM/TPS/boost activation windows
 * - Inter-stage delays
 * - Per-stage HP ratings
 * - Safety interlocks
 * - Purge control
 * - Bottle pressure monitoring
 *
 * Typical Configuration:
 * - Stage 1: 50 HP (3000-4000 RPM)
 * - Stage 2: 50 HP (4000-5000 RPM) - Total 100 HP
 * - Stage 3: 50 HP (5000-6000 RPM) - Total 150 HP
 * - Stage 4: 50 HP (6000-7000 RPM) - Total 200 HP
 * - Stage 5: 50 HP (7000-8000 RPM) - Total 250 HP
 * - Stage 6: 50 HP (8000-9000 RPM) - Total 300 HP
 *
 * Safety Interlocks:
 * - Minimum TPS (prevent idle activation)
 * - Maximum TPS (WOT only)
 * - Coolant temperature check
 * - Boost pressure limit (turbo applications)
 * - Fuel pressure monitoring
 * - Bottle pressure monitoring (900-1100 PSI)
 * - RPM window enforcement
 * - Armed switch required
 *
 * Wet vs Dry:
 * - Dry: Nitrous only (ECU adds fuel)
 * - Wet: Nitrous + fuel solenoids (fixed ratio)
 *
 * @author Created for Phase 5++ Haltech-style features
 * @date 2025-11-16
 */

#ifndef NITROUS_CONTROL_H
#define NITROUS_CONTROL_H

#include <stdint.h>

namespace NitrousControl {

/**
 * @brief Maximum number of nitrous stages
 */
const uint8_t MAX_STAGES = 6;

/**
 * @brief Nitrous system type
 */
enum SystemType {
    SYSTEM_DISABLED,    // Nitrous disabled
    SYSTEM_DRY,         // Dry system (nitrous only, ECU adds fuel)
    SYSTEM_WET          // Wet system (nitrous + fuel solenoids)
};

/**
 * @brief Nitrous stage configuration
 */
struct NitrousStage {
    char name[16];              // Stage name (e.g., "Stage 1")
    uint8_t hp_rating;          // Horsepower rating (HP)

    // Activation windows
    uint16_t rpm_activate;      // RPM to activate this stage
    uint16_t rpm_deactivate;    // RPM to deactivate (0 = no upper limit)
    uint8_t  tps_min;           // Minimum TPS to activate (%)
    uint16_t boost_max;         // Maximum boost allowed (PSI * 10, 0 = no limit)

    // Timing
    uint16_t delay_ms;          // Delay after previous stage (ms)
    uint16_t min_active_time;   // Minimum activation time (ms)

    // Solenoid outputs
    uint8_t nitrous_pin;        // Nitrous solenoid pin
    uint8_t fuel_pin;           // Fuel solenoid pin (wet system only)

    // Enable/disable
    bool enabled;
};

/**
 * @brief Nitrous system configuration
 */
struct NitrousConfig {
    SystemType type;            // System type (dry/wet)

    // Global safety limits
    uint16_t min_coolant_temp;  // Min coolant temp to arm (°C * 10)
    uint16_t max_coolant_temp;  // Max coolant temp to operate (°C * 10)
    uint8_t  min_tps;           // Global minimum TPS (%)
    uint16_t min_fuel_pressure; // Min fuel pressure (PSI * 10)
    uint16_t min_bottle_pressure; // Min bottle pressure (PSI)
    uint16_t max_bottle_pressure; // Max bottle pressure (PSI)

    // Purge
    uint8_t purge_pin;          // Purge solenoid pin
    uint16_t purge_duration;    // Purge duration (ms)

    // Armed switch
    uint8_t arm_pin;            // Arm switch input pin
    bool    arm_active_high;    // Arm switch logic (true = high is armed)
};

/**
 * @brief Nitrous system status
 */
struct NitrousStatus {
    bool armed;                 // System armed
    bool active;                // Nitrous flowing
    uint8_t active_stages;      // Number of active stages
    bool stage_active[MAX_STAGES]; // Per-stage active status
    uint32_t stage_activation_time[MAX_STAGES]; // Stage activation time (ms)

    // Safety status
    bool coolant_temp_ok;
    bool fuel_pressure_ok;
    bool bottle_pressure_ok;
    bool tps_ok;
    bool boost_ok;

    // Statistics
    uint32_t total_shots;       // Total number of activations
    uint32_t total_time;        // Total active time (ms)
    uint16_t bottle_pressure;   // Current bottle pressure (PSI)

    // Purge
    bool purging;               // Purge active
    uint32_t purge_start_time;  // Purge start time (ms)
};

/**
 * @brief Initialize nitrous control
 *
 * @param config System configuration
 */
void init(const NitrousConfig& config);

/**
 * @brief Update nitrous control
 *
 * Call this every loop iteration
 *
 * @param rpm Current RPM
 * @param throttle Throttle position (0-100%)
 * @param coolant_temp Coolant temp (°C * 10)
 * @param boost Boost pressure (PSI * 10)
 * @param fuel_pressure Fuel pressure (PSI * 10)
 */
void update(uint16_t rpm, uint8_t throttle, int16_t coolant_temp,
            uint16_t boost, uint16_t fuel_pressure);

/**
 * @brief Configure stage
 *
 * @param stage_index Stage number (0-5)
 * @param stage Stage configuration
 */
void configureStage(uint8_t stage_index, const NitrousStage& stage);

/**
 * @brief Check if stage should be active
 *
 * @param stage_index Stage number (0-5)
 * @param rpm Current RPM
 * @param throttle Throttle position
 * @param boost Boost pressure
 * @return true if stage should activate
 */
bool shouldStageActivate(uint8_t stage_index, uint16_t rpm,
                        uint8_t throttle, uint16_t boost);

/**
 * @brief Activate purge
 *
 * Purges nitrous lines to remove air bubbles
 */
void activatePurge();

/**
 * @brief Deactivate purge
 */
void deactivatePurge();

/**
 * @brief Check if system is armed
 *
 * @return true if armed
 */
bool isArmed();

/**
 * @brief Check safety interlocks
 *
 * @param coolant_temp Coolant temp
 * @param fuel_pressure Fuel pressure
 * @return true if all safety checks pass
 */
bool checkSafetyInterlocks(int16_t coolant_temp, uint16_t fuel_pressure);

/**
 * @brief Emergency shutdown
 *
 * Immediately deactivates all stages
 */
void emergencyShutdown();

/**
 * @brief Get system status
 *
 * @return Status structure
 */
const NitrousStatus* getStatus();

/**
 * @brief Get total horsepower
 *
 * @return Total HP from all active stages
 */
uint16_t getTotalHP();

/**
 * @brief Calculate fuel enrichment required
 *
 * For dry systems, calculates fuel % to add
 *
 * @return Fuel enrichment (% * 10)
 */
uint16_t calculateFuelEnrichment();

/**
 * @brief Calculate timing retard required
 *
 * Retards timing for safety with nitrous
 *
 * @return Timing retard (degrees)
 */
int8_t calculateTimingRetard();

// ============================================================================
// PRESET CONFIGURATIONS
// ============================================================================

/**
 * @brief Create conservative 3-stage street setup
 *
 * 3 stages: 50 HP each (total 150 HP)
 * Safe for street use
 *
 * @return Nitrous config
 */
NitrousConfig create3StageStreetConfig();

/**
 * @brief Create aggressive 6-stage drag setup
 *
 * 6 stages: 50 HP each (total 300 HP)
 * For drag racing only
 *
 * @return Nitrous config
 */
NitrousConfig create6StageDragConfig();

/**
 * @brief Create single-stage setup
 *
 * 1 stage: 100 HP
 * Simplest configuration
 *
 * @return Nitrous config
 */
NitrousConfig createSingleStageConfig();

} // namespace NitrousControl

#endif // NITROUS_CONTROL_H
