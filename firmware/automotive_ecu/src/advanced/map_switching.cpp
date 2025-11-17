/**
 * @file map_switching.cpp
 * @brief Multi-map switching implementation
 *
 * @author Created for Phase 5++ Haltech-style features
 * @date 2025-11-16
 */

#include "map_switching.h"
#include <Arduino.h>
#include <EEPROM.h>

namespace MapSwitching {

// ============================================================================
// CONFIGURATION
// ============================================================================

// EEPROM layout
const uint16_t EEPROM_BASE_ADDRESS = 0;  // Start of map storage
const uint16_t MAP_SIZE = sizeof(TuneMap);
const uint16_t EEPROM_MAP_ADDRESSES[MAX_MAPS] = {
    EEPROM_BASE_ADDRESS + (MAP_SIZE * 0),
    EEPROM_BASE_ADDRESS + (MAP_SIZE * 1),
    EEPROM_BASE_ADDRESS + (MAP_SIZE * 2),
    EEPROM_BASE_ADDRESS + (MAP_SIZE * 3)
};

// Runtime state
static TuneMap maps[MAX_MAPS];
static MapStatus status = {
    .active_map = 0,
    .requested_map = 0,
    .switching = false,
    .safe_to_switch = false,
    .active_name = "Street",
    .switch_count = 0,
    .last_switch_time = 0
};

// ============================================================================
// INITIALIZATION
// ============================================================================

void init() {
    Serial.println("[MapSwitch] Initializing map switching system");

    // Try to load maps from EEPROM
    bool maps_loaded = false;
    for (uint8_t i = 0; i < MAX_MAPS; i++) {
        if (loadMap(i, maps[i])) {
            Serial.printf("[MapSwitch] Loaded map %d: %s\n", i, maps[i].name);
            maps_loaded = true;
        } else {
            Serial.printf("[MapSwitch] Map %d invalid, creating default\n", i);
            // Create default maps based on slot
            switch (i) {
                case 0: maps[i] = createStreetMap(); break;
                case 1: maps[i] = createRaceMap(); break;
                case 2: maps[i] = createValetMap(); break;
                case 3: maps[i] = createDiagnosticMap(); break;
            }
            // Save to EEPROM
            saveMap(i, maps[i]);
        }
    }

    // Activate map 0 by default
    applyMap(maps[0]);
    strcpy(status.active_name, maps[0].name);

    Serial.printf("[MapSwitch] Active map: %s\n", status.active_name);
    Serial.printf("[MapSwitch] Available maps: %d\n", MAX_MAPS);
}

// ============================================================================
// MAIN UPDATE
// ============================================================================

void update(uint16_t rpm, uint16_t speed, uint8_t throttle) {
    // Check if conditions are safe for map switching
    status.safe_to_switch = isSafeToSwitch(rpm, speed, throttle);

    // If switch in progress, complete it
    if (status.switching && status.safe_to_switch) {
        // Apply the requested map
        applyMap(maps[status.requested_map]);

        // Update status
        status.active_map = status.requested_map;
        strcpy(status.active_name, maps[status.active_map].name);
        status.switching = false;
        status.switch_count++;
        status.last_switch_time = millis();

        Serial.printf("[MapSwitch] Switched to map %d: %s\n",
                     status.active_map, status.active_name);
    }
}

// ============================================================================
// MAP SWITCHING
// ============================================================================

bool switchToMap(uint8_t map_index) {
    // Validate map index
    if (map_index >= MAX_MAPS) {
        Serial.printf("[MapSwitch] ERROR: Invalid map index %d\n", map_index);
        return false;
    }

    // Check if already on this map
    if (map_index == status.active_map && !status.switching) {
        Serial.printf("[MapSwitch] Already on map %d\n", map_index);
        return true;
    }

    // Validate map before switching
    if (!validateMap(maps[map_index])) {
        Serial.printf("[MapSwitch] ERROR: Map %d failed validation\n", map_index);
        return false;
    }

    // Set requested map
    status.requested_map = map_index;
    status.switching = true;

    Serial.printf("[MapSwitch] Map switch requested: %d -> %d (%s)\n",
                 status.active_map, map_index, maps[map_index].name);
    Serial.println("[MapSwitch] Waiting for safe conditions (idle/low speed)...");

    return true;
}

bool isSafeToSwitch(uint16_t rpm, uint16_t speed, uint8_t throttle) {
    // Safe conditions:
    // - RPM < 2000 (near idle)
    // - Speed < 100 (10 km/h, nearly stopped)
    // - Throttle < 10% (closed)

    if (rpm >= 2000) return false;
    if (speed >= 100) return false;
    if (throttle >= 10) return false;

    return true;
}

// ============================================================================
// MAP APPLICATION
// ============================================================================

void applyMap(const TuneMap& map) {
    Serial.printf("[MapSwitch] Applying map: %s\n", map.name);
    Serial.printf("[MapSwitch]   RPM limit: %d\n", map.rpm_limit);
    Serial.printf("[MapSwitch]   Boost limit: %.1f PSI\n", map.boost_limit_max / 10.0f);
    Serial.printf("[MapSwitch]   Timing offset: %d°\n", map.timing_offset);
    Serial.printf("[MapSwitch]   Fuel multiplier: %.1f%%\n", map.fuel_multiplier / 10.0f);
    Serial.printf("[MapSwitch]   Power limit: %d%%\n", map.power_limit_percent);

    // Note: Actual application to engine control systems would happen here
    // For now, the map is stored and can be queried by other systems
    // via getActiveMap()

    // Feature enables would be applied to SystemManager:
    // SystemManager::setFeatureEnabled("boost_control", map.enable_boost_control);
    // SystemManager::setFeatureEnabled("launch_control", map.enable_launch_control);
    // etc.
}

// ============================================================================
// MAP VALIDATION
// ============================================================================

bool validateMap(const TuneMap& map) {
    // Calculate expected checksum
    uint16_t calculated = calculateChecksum(map);

    // Compare with stored checksum
    if (map.checksum != calculated) {
        Serial.printf("[MapSwitch] Checksum mismatch: expected %u, got %u\n",
                     calculated, map.checksum);
        return false;
    }

    // Sanity checks
    if (map.rpm_limit < 1000 || map.rpm_limit > 12000) {
        Serial.printf("[MapSwitch] Invalid RPM limit: %d\n", map.rpm_limit);
        return false;
    }

    if (map.boost_limit_max > 300) {  // 30 PSI max
        Serial.printf("[MapSwitch] Boost limit too high: %.1f PSI\n",
                     map.boost_limit_max / 10.0f);
        return false;
    }

    if (map.power_limit_percent > 100) {
        Serial.printf("[MapSwitch] Invalid power limit: %d%%\n",
                     map.power_limit_percent);
        return false;
    }

    return true;
}

uint16_t calculateChecksum(const TuneMap& map) {
    // Simple checksum: sum of all bytes except checksum field
    uint16_t sum = 0;
    const uint8_t* bytes = (const uint8_t*)&map;
    size_t map_size = sizeof(TuneMap) - sizeof(map.checksum);

    for (size_t i = 0; i < map_size; i++) {
        sum += bytes[i];
    }

    return sum;
}

// ============================================================================
// EEPROM STORAGE
// ============================================================================

bool loadMap(uint8_t map_index, TuneMap& map) {
    if (map_index >= MAX_MAPS) {
        return false;
    }

    // Read from EEPROM
    uint16_t addr = EEPROM_MAP_ADDRESSES[map_index];
    uint8_t* bytes = (uint8_t*)&map;

    for (size_t i = 0; i < sizeof(TuneMap); i++) {
        bytes[i] = EEPROM.read(addr + i);
    }

    // Validate loaded map
    return validateMap(map);
}

bool saveMap(uint8_t map_index, const TuneMap& map) {
    if (map_index >= MAX_MAPS) {
        Serial.printf("[MapSwitch] ERROR: Invalid map index %d\n", map_index);
        return false;
    }

    // Validate before saving
    if (!validateMap(map)) {
        Serial.printf("[MapSwitch] ERROR: Map validation failed, not saving\n");
        return false;
    }

    // Write to EEPROM
    uint16_t addr = EEPROM_MAP_ADDRESSES[map_index];
    const uint8_t* bytes = (const uint8_t*)&map;

    for (size_t i = 0; i < sizeof(TuneMap); i++) {
        EEPROM.write(addr + i, bytes[i]);
    }

    Serial.printf("[MapSwitch] Saved map %d: %s\n", map_index, map.name);
    return true;
}

// ============================================================================
// ACCESSORS
// ============================================================================

const TuneMap* getActiveMap() {
    return &maps[status.active_map];
}

const TuneMap* getMap(uint8_t map_index) {
    if (map_index >= MAX_MAPS) {
        return nullptr;
    }
    return &maps[map_index];
}

void setMapName(uint8_t map_index, const char* name) {
    if (map_index >= MAX_MAPS) {
        return;
    }

    strncpy(maps[map_index].name, name, 31);
    maps[map_index].name[31] = '\0';

    // Recalculate checksum
    maps[map_index].checksum = calculateChecksum(maps[map_index]);

    // Save to EEPROM
    saveMap(map_index, maps[map_index]);
}

const MapStatus* getStatus() {
    return &status;
}

// ============================================================================
// PRESET MAP CREATION
// ============================================================================

TuneMap createStreetMap() {
    TuneMap map = {};

    strcpy(map.name, "Street");
    strcpy(map.description, "Conservative street tune, E0 gasoline, fuel economy");

    // Boost control (conservative)
    map.boost_target[0] = 50;   // 1st: 5 PSI
    map.boost_target[1] = 60;   // 2nd: 6 PSI
    map.boost_target[2] = 70;   // 3rd: 7 PSI
    map.boost_target[3] = 80;   // 4th: 8 PSI
    map.boost_target[4] = 80;   // 5th: 8 PSI
    map.boost_target[5] = 80;   // 6th: 8 PSI
    map.boost_limit_max = 100;  // 10 PSI absolute max

    // Ignition timing (conservative)
    map.timing_offset = 0;      // No global offset
    map.timing_idle = 10;       // 10° BTDC idle
    map.timing_cruise = 25;     // 25° BTDC cruise
    map.timing_wot = 15;        // 15° BTDC WOT (safe)

    // Fuel delivery
    map.fuel_multiplier = 1000; // 100% (no adjustment)
    map.target_afr = 147;       // 14.7:1 stoichiometric

    // RPM limits
    map.rpm_limit = 9000;       // 9000 RPM (stock redline)
    map.rpm_warning = 8500;     // Warning at 8500

    // Feature enables
    map.enable_boost_control = true;
    map.enable_launch_control = false;   // No launch on street
    map.enable_antilag = false;          // No anti-lag on street
    map.enable_water_meth = false;       // Not needed at low boost
    map.enable_traction_control = true;  // Keep TC on

    // Power limiting
    map.power_limit_percent = 100;       // No limit

    // Safety
    map.coolant_temp_limit = 1150;       // 115°C max
    map.min_oil_pressure = 10;           // 10 PSI min

    // Calculate checksum
    map.checksum = calculateChecksum(map);

    return map;
}

TuneMap createRaceMap() {
    TuneMap map = {};

    strcpy(map.name, "Race");
    strcpy(map.description, "Aggressive race tune, E85, high boost");

    // Boost control (aggressive)
    map.boost_target[0] = 100;  // 1st: 10 PSI
    map.boost_target[1] = 120;  // 2nd: 12 PSI
    map.boost_target[2] = 140;  // 3rd: 14 PSI
    map.boost_target[3] = 160;  // 4th: 16 PSI
    map.boost_target[4] = 160;  // 5th: 16 PSI
    map.boost_target[5] = 160;  // 6th: 16 PSI
    map.boost_limit_max = 180;  // 18 PSI absolute max

    // Ignition timing (aggressive with E85)
    map.timing_offset = 3;      // +3° global advance (E85)
    map.timing_idle = 10;       // 10° BTDC idle
    map.timing_cruise = 28;     // 28° BTDC cruise
    map.timing_wot = 20;        // 20° BTDC WOT (E85 allows more)

    // Fuel delivery (E85)
    map.fuel_multiplier = 1300; // 130% (E85 needs more fuel)
    map.target_afr = 98;        // 9.8:1 (rich for power)

    // RPM limits
    map.rpm_limit = 9500;       // 9500 RPM (higher for race)
    map.rpm_warning = 9000;     // Warning at 9000

    // Feature enables (everything on)
    map.enable_boost_control = true;
    map.enable_launch_control = true;
    map.enable_antilag = true;
    map.enable_water_meth = true;
    map.enable_traction_control = true;

    // Power limiting
    map.power_limit_percent = 100;       // No limit

    // Safety (more aggressive but still safe)
    map.coolant_temp_limit = 1200;       // 120°C max
    map.min_oil_pressure = 15;           // 15 PSI min (higher)

    // Calculate checksum
    map.checksum = calculateChecksum(map);

    return map;
}

TuneMap createValetMap() {
    TuneMap map = {};

    strcpy(map.name, "Valet");
    strcpy(map.description, "Power limited, RPM limited, safe for valet");

    // Boost control (minimal)
    map.boost_target[0] = 0;    // No boost in any gear
    map.boost_target[1] = 0;
    map.boost_target[2] = 0;
    map.boost_target[3] = 0;
    map.boost_target[4] = 0;
    map.boost_target[5] = 0;
    map.boost_limit_max = 0;    // No boost allowed

    // Ignition timing (conservative)
    map.timing_offset = -3;     // -3° retard (safer)
    map.timing_idle = 10;       // 10° BTDC idle
    map.timing_cruise = 20;     // 20° BTDC cruise (reduced)
    map.timing_wot = 10;        // 10° BTDC WOT (very safe)

    // Fuel delivery
    map.fuel_multiplier = 1000; // 100%
    map.target_afr = 147;       // 14.7:1

    // RPM limits (restricted)
    map.rpm_limit = 5000;       // 5000 RPM max (very limited)
    map.rpm_warning = 4500;     // Warning at 4500

    // Feature enables (all off)
    map.enable_boost_control = false;
    map.enable_launch_control = false;
    map.enable_antilag = false;
    map.enable_water_meth = false;
    map.enable_traction_control = true;  // Keep TC on for safety

    // Power limiting (50% power)
    map.power_limit_percent = 50;        // 50% power limit

    // Safety
    map.coolant_temp_limit = 1100;       // 110°C max (conservative)
    map.min_oil_pressure = 10;           // 10 PSI min

    // Calculate checksum
    map.checksum = calculateChecksum(map);

    return map;
}

TuneMap createDiagnosticMap() {
    TuneMap map = {};

    strcpy(map.name, "Diagnostic");
    strcpy(map.description, "Safe mode for diagnostics and troubleshooting");

    // Boost control (safe/minimal)
    map.boost_target[0] = 30;   // All gears: 3 PSI (very safe)
    map.boost_target[1] = 30;
    map.boost_target[2] = 30;
    map.boost_target[3] = 30;
    map.boost_target[4] = 30;
    map.boost_target[5] = 30;
    map.boost_limit_max = 50;   // 5 PSI absolute max

    // Ignition timing (safe)
    map.timing_offset = -2;     // -2° retard
    map.timing_idle = 10;       // 10° BTDC idle
    map.timing_cruise = 22;     // 22° BTDC cruise
    map.timing_wot = 12;        // 12° BTDC WOT

    // Fuel delivery (slightly rich for safety)
    map.fuel_multiplier = 1050; // 105% (slightly rich)
    map.target_afr = 140;       // 14.0:1 (safer)

    // RPM limits
    map.rpm_limit = 7000;       // 7000 RPM (limited)
    map.rpm_warning = 6500;     // Warning at 6500

    // Feature enables (minimal)
    map.enable_boost_control = true;     // Limited boost only
    map.enable_launch_control = false;
    map.enable_antilag = false;
    map.enable_water_meth = false;
    map.enable_traction_control = true;

    // Power limiting
    map.power_limit_percent = 80;        // 80% power

    // Safety
    map.coolant_temp_limit = 1100;       // 110°C max
    map.min_oil_pressure = 10;           // 10 PSI min

    // Calculate checksum
    map.checksum = calculateChecksum(map);

    return map;
}

} // namespace MapSwitching
