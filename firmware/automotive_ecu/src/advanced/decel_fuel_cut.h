/**
 * @file decel_fuel_cut.h
 * @brief Deceleration fuel cut-off (DFCO) for fuel economy
 *
 * Features:
 * - Cuts fuel during engine braking
 * - RPM and throttle-based activation
 * - Smooth re-enable to prevent jerking
 * - Configurable thresholds
 *
 * Benefits:
 * - Improved fuel economy (10-15% in city driving)
 * - Reduced emissions
 * - Engine braking preserved
 *
 * @author Created for Phase 5+ Haltech-style features
 * @date 2025-11-16
 */

#ifndef DECEL_FUEL_CUT_H
#define DECEL_FUEL_CUT_H

#include <stdint.h>

namespace DecelFuelCut {

struct DFCOConfig {
    bool     enabled;               // DFCO enabled
    uint16_t activation_rpm;        // Min RPM to activate (e.g., 1500)
    uint16_t deactivation_rpm;      // RPM to re-enable fuel (e.g., 1200)
    uint8_t  throttle_threshold;    // Max throttle for DFCO (e.g., 2%)
    uint16_t min_coolant_temp;      // Min coolant temp for DFCO (°C * 10)
    uint8_t  reenable_delay;        // Delay before re-enabling (ms * 10)
};

struct DFCOStatus {
    bool    active;                 // Fuel currently cut
    uint16_t current_rpm;           // Current RPM
    uint8_t  current_throttle;      // Current throttle
    uint32_t cut_duration_ms;       // How long fuel has been cut
    uint32_t total_cut_time_ms;     // Total fuel cut time this session
};

void init();
bool update(uint16_t rpm, uint8_t throttle, int16_t coolant_temp);
const DFCOStatus* getStatus();
void configure(const DFCOConfig& config);
void setEnabled(bool enabled);

} // namespace DecelFuelCut

#endif
