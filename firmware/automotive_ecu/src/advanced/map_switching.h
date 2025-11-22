/**
 * @file map_switching.h
 * @brief Multi-map switching for different driving scenarios
 *
 * Features:
 * - Up to 4 complete tune profiles
 * - Switch entire calibration on-the-fly
 * - EEPROM storage for persistence
 * - Smooth transitions between maps
 * - Safety validation before switching
 * - Map-specific feature enable/disable
 *
 * Use Cases:
 * - Map 1: Street (E0, conservative, fuel economy)
 * - Map 2: Race (E85, aggressive, high boost)
 * - Map 3: Valet (power limited, RPM limited)
 * - Map 4: Diagnostic (safe mode, logging enabled)
 *
 * Each map includes:
 * - Boost targets (per gear)
 * - Timing advance curves
 * - Fuel multipliers
 * - RPM limits
 * - Feature enable flags
 * - Safety limits
 *
 * Safety:
 * - Can only switch at idle or low speed
 * - Validates map before switching
 * - Fallback to safe map if corruption detected
 *
 * @author Created for Phase 5++ Haltech-style features
 * @date 2025-11-16
 */

#ifndef MAP_SWITCHING_H
#define MAP_SWITCHING_H

#include <stdint.h>

namespace MapSwitching {

/**
 * @brief Maximum number of maps
 */
const uint8_t MAX_MAPS = 4;

/**
 * @brief Tune map definition
 */
struct TuneMap {
    char name[32];                  // Map name (e.g., "Street", "Race")
    char description[64];           // Description

    // Boost control
    uint16_t boost_target[6];       // Per-gear boost targets (PSI * 10)
    uint16_t boost_limit_max;       // Absolute max boost (PSI * 10)

    // Ignition timing
    int8_t timing_offset;           // Global timing offset (degrees)
    int8_t timing_idle;             // Idle timing (degrees BTDC)
    int8_t timing_cruise;           // Cruise timing (degrees BTDC)
    int8_t timing_wot;              // WOT timing (degrees BTDC)

    // Fuel delivery
    uint16_t fuel_multiplier;       // Global fuel multiplier (% * 10)
    uint16_t target_afr;            // Target AFR (AFR * 10)

    // RPM limits
    uint16_t rpm_limit;             // Rev limiter (RPM)
    uint16_t rpm_warning;           // Warning threshold (RPM)

    // Feature enables
    bool enable_boost_control;
    bool enable_launch_control;
    bool enable_antilag;
    bool enable_water_meth;
    bool enable_traction_control;

    // Power limiting (for valet mode)
    uint8_t power_limit_percent;    // 0-100% (100 = no limit)

    // Safety
    uint16_t coolant_temp_limit;    // Max coolant temp (°C * 10)
    uint8_t min_oil_pressure;       // Min oil pressure (PSI)

    // Validation
    uint16_t checksum;              // Data integrity check
};

/**
 * @brief Map switching status
 */
struct MapStatus {
    uint8_t active_map;             // Currently active map (0-3)
    uint8_t requested_map;          // Requested map (during switch)
    bool switching;                 // Switch in progress
    bool safe_to_switch;            // Conditions OK for switching
    char active_name[32];           // Active map name
    uint32_t switch_count;          // Number of switches this session
    uint32_t last_switch_time;      // Millis of last switch
};

/**
 * @brief Initialize map switching
 */
void init();

/**
 * @brief Update map switching
 *
 * @param rpm Current RPM
 * @param speed Vehicle speed (km/h * 10)
 * @param throttle Throttle position (0-100%)
 */
void update(uint16_t rpm, uint16_t speed, uint8_t throttle);

/**
 * @brief Switch to different map
 *
 * Safety checks:
 * - Only allows switching at idle or low speed
 * - Validates map before activating
 * - Smooth transition
 *
 * @param map_index Map to switch to (0-3)
 * @return true if switch initiated successfully
 */
bool switchToMap(uint8_t map_index);

/**
 * @brief Check if safe to switch maps
 *
 * Safe conditions:
 * - RPM < 2000 (near idle)
 * - Speed < 10 km/h (nearly stopped)
 * - Throttle < 10% (closed)
 *
 * @param rpm Current RPM
 * @param speed Vehicle speed
 * @param throttle Throttle position
 * @return true if safe to switch
 */
bool isSafeToSwitch(uint16_t rpm, uint16_t speed, uint8_t throttle);

/**
 * @brief Apply map settings
 *
 * @param map Map to apply
 */
void applyMap(const TuneMap& map);

/**
 * @brief Validate map integrity
 *
 * @param map Map to validate
 * @return true if map is valid
 */
bool validateMap(const TuneMap& map);

/**
 * @brief Calculate map checksum
 *
 * @param map Map to checksum
 * @return Checksum value
 */
uint16_t calculateChecksum(const TuneMap& map);

/**
 * @brief Load map from EEPROM
 *
 * @param map_index Map slot (0-3)
 * @param map Output map structure
 * @return true if loaded successfully
 */
bool loadMap(uint8_t map_index, TuneMap& map);

/**
 * @brief Save map to EEPROM
 *
 * @param map_index Map slot (0-3)
 * @param map Map to save
 * @return true if saved successfully
 */
bool saveMap(uint8_t map_index, const TuneMap& map);

/**
 * @brief Get currently active map
 *
 * @return Active map structure
 */
const TuneMap* getActiveMap();

/**
 * @brief Get map by index
 *
 * @param map_index Map slot (0-3)
 * @return Map structure (nullptr if invalid)
 */
const TuneMap* getMap(uint8_t map_index);

/**
 * @brief Set map name
 *
 * @param map_index Map slot (0-3)
 * @param name Map name
 */
void setMapName(uint8_t map_index, const char* name);

/**
 * @brief Get map switching status
 * @return Status structure
 */
const MapStatus* getStatus();

/**
 * @brief Create preset maps
 */
TuneMap createStreetMap();          // Conservative street tune
TuneMap createRaceMap();            // Aggressive race tune
TuneMap createValetMap();           // Power-limited valet mode
TuneMap createDiagnosticMap();      // Safe diagnostic mode

} // namespace MapSwitching

#endif // MAP_SWITCHING_H
