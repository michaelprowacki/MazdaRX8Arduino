/**
 * @file knock_detection.cpp
 * @brief Knock detection implementation
 *
 * @author Created for Phase 5+ advanced features
 * @date 2025-11-16
 */

#include "knock_detection.h"
#include "../hal/hal.h"
#include <Arduino.h>

namespace KnockDetection {

// ============================================================================
// CONFIGURATION
// ============================================================================

static KnockConfig config = {
    .sensor_type = SENSOR_ANALOG,
    .sensitivity = 50,          // Medium sensitivity
    .knock_frequency = 7000,    // 7 kHz typical for rotary
    .frequency_window = 2000,   // ±2 kHz
    .max_retard = 10,           // Max 10° retard
    .retard_step = 2,           // 2° per knock event
    .recovery_rate = 1,         // 1° per second recovery
    .enable_protection = true
};

static KnockStatus status = {
    .knock_detected = false,
    .knock_intensity = 0,
    .current_retard = 0,
    .knock_count_total = 0,
    .knock_count_session = 0,
    .knock_rpm = 0,
    .knock_cylinder = 0
};

// Knock event log (circular buffer)
static const uint8_t MAX_LOG_ENTRIES = 20;
static KnockEvent event_log[MAX_LOG_ENTRIES];
static uint8_t log_index = 0;
static uint8_t log_count = 0;

// Adaptive threshold state
static uint16_t background_noise = 100;  // Running average of noise
static uint16_t knock_threshold = 200;   // Adaptive threshold

// Recovery state
static uint32_t last_update_ms = 0;

// ============================================================================
// INITIALIZATION
// ============================================================================

void init() {
    Serial.println("[KNOCK] Knock detection initialized");
    Serial.printf("[KNOCK] Sensor: %s\n",
                 config.sensor_type == SENSOR_ANALOG ? "Analog" :
                 config.sensor_type == SENSOR_PIEZO ? "Piezo" : "Accelerometer");
    Serial.printf("[KNOCK] Target frequency: %d Hz (±%d Hz)\n",
                 config.knock_frequency, config.frequency_window);
    Serial.printf("[KNOCK] Protection: %s\n",
                 config.enable_protection ? "ENABLED" : "DISABLED");
    Serial.printf("[KNOCK] Max retard: %d°\n", config.max_retard);

    last_update_ms = millis();
}

// ============================================================================
// MAIN UPDATE
// ============================================================================

bool update(uint16_t rpm, uint8_t throttle, uint16_t boost) {
    if (!config.enable_protection) {
        status.knock_detected = false;
        return false;
    }

    // Read knock sensor
    uint16_t sensor_value = readSensor();

    // Detect knock
    uint8_t intensity = detectKnock(sensor_value, rpm);
    bool knock_this_cycle = (intensity > 0);

    if (knock_this_cycle) {
        status.knock_detected = true;
        status.knock_intensity = intensity;
        status.knock_rpm = rpm;
        status.knock_count_total++;
        status.knock_count_session++;

        // TODO: Determine which rotor knocked (requires per-rotor sensing)
        status.knock_cylinder = 0;

        // Log event
        logKnockEvent(rpm, throttle, boost, intensity, status.knock_cylinder);

        Serial.printf("[KNOCK] KNOCK DETECTED! Intensity=%d%%, RPM=%d\n",
                     intensity, rpm);
    } else {
        status.knock_detected = false;
        status.knock_intensity = 0;
    }

    // Calculate timing retard
    status.current_retard = calculateRetard(knock_this_cycle);

    return knock_this_cycle;
}

// ============================================================================
// SENSOR READING
// ============================================================================

uint16_t readSensor() {
    // TODO: Define knock sensor pin in pin_mapping.h
    // For now, use a dummy pin or return test value
    const uint8_t KNOCK_SENSOR_PIN = A0;  // Replace with actual pin

    switch (config.sensor_type) {
        case SENSOR_ANALOG:
            // Read analog knock sensor (0-5V → 0-1023)
            return HAL_ADC_Read(KNOCK_SENSOR_PIN);

        case SENSOR_PIEZO:
            // Digital knock sensor (typically outputs pulses)
            // Would need interrupt-based pulse counting
            return 0;  // Placeholder

        case SENSOR_ACCELEROMETER:
            // Accelerometer-based detection
            // Would need I2C or SPI communication
            return 0;  // Placeholder

        default:
            return 0;
    }
}

// ============================================================================
// KNOCK DETECTION ALGORITHM
// ============================================================================

uint8_t detectKnock(uint16_t sensor_value, uint16_t rpm) {
    // Update adaptive threshold
    knock_threshold = getAdaptiveThreshold(rpm);

    // Simple threshold-based detection
    // Real implementation would use:
    // - Bandpass filtering (5-15 kHz)
    // - FFT analysis
    // - Integration window (specific crank angle)

    if (sensor_value < knock_threshold) {
        // Update background noise estimate
        background_noise = (background_noise * 15 + sensor_value) / 16;
        return 0;  // No knock
    }

    // Calculate knock intensity
    uint16_t knock_margin = sensor_value - knock_threshold;
    uint8_t intensity = min(100, (knock_margin * 100) / knock_threshold);

    // Scale by sensitivity
    intensity = (intensity * config.sensitivity) / 100;

    return intensity;
}

uint16_t getAdaptiveThreshold(uint16_t rpm) {
    // Base threshold = background noise + margin
    uint16_t base_threshold = background_noise + (background_noise / 2);

    // Increase threshold at high RPM (mechanical noise)
    if (rpm > 5000) {
        uint16_t rpm_factor = (rpm - 5000) / 100;  // +1% per 100 RPM
        base_threshold += (base_threshold * rpm_factor) / 100;
    }

    // Apply sensitivity
    base_threshold = (base_threshold * (100 - config.sensitivity)) / 100;

    return base_threshold;
}

// ============================================================================
// TIMING RETARD
// ============================================================================

uint8_t calculateRetard(bool knock_detected) {
    uint32_t now = millis();
    float dt = (now - last_update_ms) / 1000.0f;  // Seconds
    last_update_ms = now;

    if (knock_detected) {
        // Increase retard
        status.current_retard = min(config.max_retard,
                                    status.current_retard + config.retard_step);
    } else {
        // Gradually recover timing
        float recovery = config.recovery_rate * dt;
        if (status.current_retard > recovery) {
            status.current_retard -= (uint8_t)recovery;
        } else {
            status.current_retard = 0;
        }
    }

    return status.current_retard;
}

// ============================================================================
// EVENT LOGGING
// ============================================================================

void logKnockEvent(uint16_t rpm, uint8_t throttle, uint16_t boost,
                   uint8_t intensity, uint8_t cylinder) {
    // Add to circular buffer
    event_log[log_index] = {
        .timestamp = millis(),
        .rpm = rpm,
        .throttle = throttle,
        .boost = (uint8_t)boost,
        .intensity = intensity,
        .cylinder = cylinder
    };

    log_index = (log_index + 1) % MAX_LOG_ENTRIES;
    if (log_count < MAX_LOG_ENTRIES) {
        log_count++;
    }
}

const KnockEvent* getEventLog(uint8_t max_events) {
    // Return pointer to log array (caller must handle circular buffer)
    return event_log;
}

void clearEventLog() {
    log_index = 0;
    log_count = 0;
    Serial.println("[KNOCK] Event log cleared");
}

// ============================================================================
// CONFIGURATION
// ============================================================================

const KnockStatus* getStatus() {
    return &status;
}

void configure(const KnockConfig& new_config) {
    config = new_config;
    Serial.println("[KNOCK] Knock detection configuration updated");
    init();
}

void setEnabled(bool enabled) {
    config.enable_protection = enabled;
    Serial.printf("[KNOCK] Knock protection %s\n",
                 enabled ? "ENABLED" : "DISABLED");

    if (!enabled) {
        status.current_retard = 0;
    }
}

void resetCounters() {
    status.knock_count_session = 0;
    Serial.println("[KNOCK] Session counters reset");
}

} // namespace KnockDetection
