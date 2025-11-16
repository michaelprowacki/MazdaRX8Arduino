/**
 * @file flex_fuel.cpp
 * @brief Flex fuel implementation
 *
 * @author Created for Phase 5+ Haltech-style features
 * @date 2025-11-16
 */

#include "flex_fuel.h"
#include <Arduino.h>

namespace FlexFuel {

// ============================================================================
// CONFIGURATION
// ============================================================================

static FlexFuelConfig config = {
    .enabled = false,
    .ethanol_target = 85,           // E85 target
    .fuel_pressure_offset = 50,     // +5 kPa for E85
    .timing_advance_e85 = 5,        // +5° for E85
    .cold_start_multiplier = 30,    // +30% for cold E85 starts
    .stoich_afr_gasoline = 147,     // 14.7:1
    .stoich_afr_e85 = 98            // 9.8:1
};

static FlexFuelStatus status = {
    .ethanol_content = 0,
    .fuel_multiplier = 1000,        // 100% (no adjustment)
    .timing_adjustment = 0,
    .target_afr = 147,              // 14.7:1 default
    .sensor_ok = false,
    .sensor_frequency = 0
};

// Ethanol override (for testing without sensor)
static bool override_enabled = false;
static uint8_t override_ethanol = 0;

// Sensor reading (frequency input)
static volatile uint32_t last_pulse_time = 0;
static volatile uint32_t pulse_count = 0;
static volatile uint16_t sensor_frequency = 0;

// ============================================================================
// SENSOR INTERRUPT HANDLER
// ============================================================================

/**
 * Interrupt handler for flex fuel sensor
 * GM sensor outputs 50-150 Hz based on ethanol content
 */
void IRAM_ATTR flexFuelSensorISR() {
    uint32_t now = micros();
    uint32_t period = now - last_pulse_time;

    if (period > 1000) {  // Debounce (> 1ms)
        sensor_frequency = 1000000 / period;  // Hz
        last_pulse_time = now;
        pulse_count++;
    }
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void init() {
    Serial.println("[FLEX] Flex fuel system initialized");
    Serial.printf("[FLEX] Status: %s\n", config.enabled ? "ENABLED" : "DISABLED");

    if (config.enabled) {
        Serial.println("[FLEX] GM flex fuel sensor expected:");
        Serial.println("[FLEX]   50 Hz = E0 (gasoline)");
        Serial.println("[FLEX]   150 Hz = E100 (pure ethanol)");
        Serial.printf("[FLEX] Timing advance for E85: +%d degrees\n",
                     config.timing_advance_e85);

        // TODO: Attach interrupt to flex fuel sensor pin
        // pinMode(FLEX_FUEL_SENSOR_PIN, INPUT_PULLUP);
        // attachInterrupt(digitalPinToInterrupt(FLEX_FUEL_SENSOR_PIN),
        //                 flexFuelSensorISR, RISING);
    }
}

// ============================================================================
// MAIN UPDATE
// ============================================================================

uint8_t update(int16_t coolant_temp) {
    if (!config.enabled) {
        status.ethanol_content = 0;
        status.fuel_multiplier = 1000;
        status.timing_adjustment = 0;
        status.target_afr = config.stoich_afr_gasoline;
        return 0;
    }

    // Read ethanol content
    status.ethanol_content = readEthanolSensor();

    // Calculate fuel delivery adjustment
    status.fuel_multiplier = calculateFuelMultiplier(status.ethanol_content);

    // Calculate ignition timing adjustment
    status.timing_adjustment = calculateTimingAdjustment(status.ethanol_content);

    // Calculate target AFR
    status.target_afr = calculateTargetAFR(status.ethanol_content);

    // Sensor health check
    status.sensor_ok = (sensor_frequency >= 40 && sensor_frequency <= 160);

    return status.ethanol_content;
}

// ============================================================================
// ETHANOL SENSOR READING
// ============================================================================

uint8_t readEthanolSensor() {
    if (override_enabled) {
        return override_ethanol;
    }

    status.sensor_frequency = sensor_frequency;

    // GM flex fuel sensor: 50 Hz = E0, 150 Hz = E100
    const uint16_t MIN_FREQ = 50;
    const uint16_t MAX_FREQ = 150;

    // Bounds checking
    if (sensor_frequency < MIN_FREQ) {
        return 0;  // Below minimum = pure gasoline
    }
    if (sensor_frequency > MAX_FREQ) {
        return 100;  // Above maximum = pure ethanol
    }

    // Linear interpolation
    uint16_t freq_range = MAX_FREQ - MIN_FREQ;
    uint16_t freq_offset = sensor_frequency - MIN_FREQ;
    uint8_t ethanol = (freq_offset * 100) / freq_range;

    return ethanol;
}

// ============================================================================
// FUEL DELIVERY CALCULATIONS
// ============================================================================

uint16_t calculateFuelMultiplier(uint8_t ethanol_percent) {
    // E85 requires ~30% more fuel than gasoline
    // This is due to:
    // 1. Lower energy density (26.8 MJ/L vs 32 MJ/L)
    // 2. Different stoichiometric ratio (9.8 vs 14.7)

    // Base multiplier: 100% for gasoline, 130% for E85
    const uint16_t BASE_MULTIPLIER = 1000;      // 100%
    const uint16_t E85_MULTIPLIER = 1300;       // 130%

    // Linear interpolation
    uint16_t multiplier = BASE_MULTIPLIER +
                         ((E85_MULTIPLIER - BASE_MULTIPLIER) * ethanol_percent) / 100;

    return multiplier;
}

// ============================================================================
// IGNITION TIMING CALCULATIONS
// ============================================================================

int8_t calculateTimingAdjustment(uint8_t ethanol_percent) {
    // E85 benefits:
    // - Higher octane (105+ vs 91-93)
    // - Better knock resistance
    // - Cooler combustion
    //
    // Can safely advance timing by 3-5 degrees

    int8_t max_advance = config.timing_advance_e85;

    // Linear interpolation from 0° (E0) to max_advance (E85)
    int8_t adjustment = (max_advance * ethanol_percent) / 100;

    return adjustment;
}

uint16_t calculateTargetAFR(uint8_t ethanol_percent) {
    // Stoichiometric AFR varies with ethanol content:
    // - E0 (gasoline): 14.7:1
    // - E85: ~9.8:1
    // - E100: 9.0:1

    uint16_t gasoline_afr = config.stoich_afr_gasoline;
    uint16_t e85_afr = config.stoich_afr_e85;

    // Linear interpolation
    // Note: This is simplified; real curve is slightly nonlinear
    uint16_t afr = gasoline_afr -
                  ((gasoline_afr - e85_afr) * ethanol_percent) / 100;

    return afr;
}

uint16_t calculateColdStartEnrichment(uint8_t ethanol_percent, int16_t coolant_temp) {
    // E85 is harder to start when cold due to:
    // - Higher flashpoint (13°C vs -40°C for gasoline)
    // - Lower vapor pressure
    //
    // Need significant enrichment below 10°C

    uint16_t base_enrichment = 1000;  // 100% (no enrichment)

    // Only enrich if ethanol content > 50%
    if (ethanol_percent < 50) {
        return base_enrichment;
    }

    // Only enrich if cold (< 20°C)
    if (coolant_temp > 200) {  // 20.0°C
        return base_enrichment;
    }

    // Calculate enrichment based on temperature
    // 0°C: +50% enrichment
    // -20°C: +100% enrichment
    uint16_t enrichment = base_enrichment;

    if (coolant_temp < 0) {
        // Below 0°C: 50-100% enrichment
        int16_t temp_below_zero = abs(coolant_temp);
        uint16_t extra = 500 + min(500, temp_below_zero * 25);  // 2.5% per degree C
        enrichment += extra;
    } else {
        // 0-20°C: 20-50% enrichment
        uint16_t extra = 200 + ((200 - coolant_temp) * 15) / 10;
        enrichment += extra;
    }

    // Scale by ethanol content
    enrichment = base_enrichment +
                ((enrichment - base_enrichment) * ethanol_percent) / 100;

    return enrichment;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

const FlexFuelStatus* getStatus() {
    return &status;
}

void configure(const FlexFuelConfig& new_config) {
    config = new_config;
    Serial.println("[FLEX] Flex fuel configuration updated");
    init();
}

void setEnabled(bool enabled) {
    config.enabled = enabled;
    Serial.printf("[FLEX] Flex fuel %s\n", enabled ? "ENABLED" : "DISABLED");

    if (enabled) {
        Serial.println("[FLEX] IMPORTANT: Ensure flex fuel sensor is connected!");
        Serial.println("[FLEX] IMPORTANT: Verify fuel system can handle E85!");
        Serial.println("[FLEX] - Check fuel pump flow rate (+30% for E85)");
        Serial.println("[FLEX] - Check fuel line compatibility");
        Serial.println("[FLEX] - Check injector sizing (+30% for E85)");
    }
}

void setEthanolOverride(uint8_t ethanol_percent) {
    override_enabled = true;
    override_ethanol = constrain(ethanol_percent, 0, 100);
    Serial.printf("[FLEX] Ethanol override: E%d\n", override_ethanol);
}

void clearEthanolOverride() {
    override_enabled = false;
    Serial.println("[FLEX] Ethanol override cleared, using sensor");
}

} // namespace FlexFuel
