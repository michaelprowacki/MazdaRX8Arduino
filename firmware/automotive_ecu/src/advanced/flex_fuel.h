/**
 * @file flex_fuel.h
 * @brief Flex fuel (E85/gasoline) support with ethanol content sensing
 *
 * Features:
 * - Ethanol content sensing (0-100%)
 * - Adaptive fuel delivery (E0 to E100)
 * - Adaptive ignition timing
 * - Cold start compensation for ethanol
 * - Fuel pressure compensation
 * - Real-time fuel blend detection
 *
 * Ethanol benefits for rotaries:
 * - Higher octane (105+ vs 91-93 for pump gas)
 * - Better cooling (reduces apex seal temps)
 * - More power (+10-15% with proper tuning)
 * - Lower EGTs (exhaust gas temps)
 *
 * @author Created for Phase 5+ Haltech-style features
 * @date 2025-11-16
 */

#ifndef FLEX_FUEL_H
#define FLEX_FUEL_H

#include <stdint.h>

namespace FlexFuel {

/**
 * @brief Flex fuel configuration
 */
struct FlexFuelConfig {
    bool     enabled;                // Flex fuel enabled
    uint8_t  ethanol_target;         // Target ethanol % (for display)
    uint16_t fuel_pressure_offset;   // Pressure increase for E85 (kPa * 10)
    int8_t   timing_advance_e85;     // Extra timing for E85 (degrees)
    uint8_t  cold_start_multiplier;  // Extra fuel for cold E85 starts (%)
    uint16_t stoich_afr_gasoline;    // Stoich AFR for gasoline (14.7 * 10)
    uint16_t stoich_afr_e85;         // Stoich AFR for E85 (9.8 * 10)
};

/**
 * @brief Flex fuel status
 */
struct FlexFuelStatus {
    uint8_t  ethanol_content;        // Current ethanol % (0-100)
    uint16_t fuel_multiplier;        // Fuel delivery multiplier (% * 10)
    int8_t   timing_adjustment;      // Timing adjustment (degrees)
    uint16_t target_afr;             // Target AFR for current blend (AFR * 10)
    bool     sensor_ok;              // Sensor reading valid
    uint16_t sensor_frequency;       // Sensor frequency (Hz)
};

/**
 * @brief Initialize flex fuel system
 */
void init();

/**
 * @brief Update flex fuel calculations
 *
 * @param coolant_temp Coolant temperature (°C * 10)
 * @return Ethanol content (0-100%)
 */
uint8_t update(int16_t coolant_temp);

/**
 * @brief Read ethanol content sensor
 *
 * GM flex fuel sensor outputs frequency:
 * - 50 Hz = 0% ethanol (E0)
 * - 150 Hz = 100% ethanol (E100)
 * - Linear relationship
 *
 * @return Ethanol content (0-100%)
 */
uint8_t readEthanolSensor();

/**
 * @brief Calculate fuel multiplier for current ethanol blend
 *
 * E85 requires ~30% more fuel than gasoline due to:
 * - Lower energy density
 * - Different stoichiometric AFR (9.8 vs 14.7)
 *
 * @param ethanol_percent Ethanol content (0-100%)
 * @return Fuel multiplier (% * 10, e.g., 1300 = 130%)
 */
uint16_t calculateFuelMultiplier(uint8_t ethanol_percent);

/**
 * @brief Calculate ignition timing adjustment
 *
 * E85 allows more aggressive timing:
 * - Higher octane (105+ vs 91-93)
 * - Better knock resistance
 * - Cooler combustion temps
 *
 * @param ethanol_percent Ethanol content (0-100%)
 * @return Timing adjustment (degrees, positive = advance)
 */
int8_t calculateTimingAdjustment(uint8_t ethanol_percent);

/**
 * @brief Calculate target AFR for current blend
 *
 * Interpolates between gasoline (14.7:1) and E85 (9.8:1)
 *
 * @param ethanol_percent Ethanol content (0-100%)
 * @return Target stoichiometric AFR (AFR * 10)
 */
uint16_t calculateTargetAFR(uint8_t ethanol_percent);

/**
 * @brief Calculate cold start enrichment for ethanol
 *
 * E85 is harder to start when cold due to higher flashpoint
 *
 * @param ethanol_percent Ethanol content (0-100%)
 * @param coolant_temp Coolant temperature (°C * 10)
 * @return Cold start multiplier (% * 10)
 */
uint16_t calculateColdStartEnrichment(uint8_t ethanol_percent, int16_t coolant_temp);

/**
 * @brief Get flex fuel status
 * @return Status structure
 */
const FlexFuelStatus* getStatus();

/**
 * @brief Configure flex fuel
 * @param config Configuration structure
 */
void configure(const FlexFuelConfig& config);

/**
 * @brief Enable/disable flex fuel
 *
 * @param enabled true to enable
 */
void setEnabled(bool enabled);

/**
 * @brief Manual ethanol content override (for testing)
 *
 * @param ethanol_percent Ethanol content (0-100%)
 */
void setEthanolOverride(uint8_t ethanol_percent);

/**
 * @brief Clear ethanol content override
 */
void clearEthanolOverride();

} // namespace FlexFuel

#endif // FLEX_FUEL_H
