/**
 * @file data_logger.cpp
 * @brief Black box data logging implementation
 *
 * @author Created for Phase 5+ advanced features
 * @date 2025-11-16
 */

#include "data_logger.h"
#include <Arduino.h>
// #include <SD.h>  // Uncomment if SD card available

namespace DataLogger {

// ============================================================================
// CONFIGURATION
// ============================================================================

static LoggerConfig config = {
    .mode = MODE_OFF,
    .sample_rate_hz = 10,       // 10 Hz default
    .max_log_size_mb = 100,     // 100 MB max
    .enable_sd_card = false,    // Disabled by default
    .enable_serial = true,      // Serial output enabled
    .triggers = {
        .trigger_on_knock = true,
        .trigger_on_overboost = true,
        .trigger_on_high_rpm = true,
        .trigger_on_launch = true,
        .rpm_threshold = 7000,
        .boost_threshold = 100   // 10 PSI
    }
};

static LoggerStatus status = {
    .logging_active = false,
    .samples_logged = 0,
    .bytes_written = 0,
    .session_id = 0,
    .filename = "",
    .sd_card_ok = false,
    .buffer_usage = 0
};

// Circular buffer for crash data (last 10 seconds at 10 Hz = 100 samples)
static const uint16_t BUFFER_SIZE = 100;
static LogEntry circular_buffer[BUFFER_SIZE];
static uint16_t buffer_index = 0;
static uint16_t buffer_count = 0;

// Timing
static uint32_t last_sample_ms = 0;
static uint32_t session_start_ms = 0;

// Session counter (persisted across resets in EEPROM ideally)
static uint32_t session_count = 0;

// File handle (if using SD card)
// static File log_file;

// ============================================================================
// INITIALIZATION
// ============================================================================

void init() {
    Serial.println("[LOGGER] Data logger initialized");
    Serial.printf("[LOGGER] Mode: %s\n",
                 config.mode == MODE_OFF ? "OFF" :
                 config.mode == MODE_CONTINUOUS ? "Continuous" :
                 config.mode == MODE_TRIGGERED ? "Triggered" : "Circular Buffer");
    Serial.printf("[LOGGER] Sample rate: %d Hz\n", config.sample_rate_hz);

    // Initialize SD card (if enabled)
    if (config.enable_sd_card) {
        // TODO: Initialize SD card
        // status.sd_card_ok = SD.begin(SD_CS_PIN);
        status.sd_card_ok = false;  // Placeholder

        if (status.sd_card_ok) {
            Serial.println("[LOGGER] SD card initialized");
        } else {
            Serial.println("[LOGGER] SD card init failed");
        }
    }

    // Load session count from EEPROM
    // TODO: Implement EEPROM persistence
    session_count = 0;
}

// ============================================================================
// SESSION MANAGEMENT
// ============================================================================

bool startSession(LogMode mode) {
    if (status.logging_active) {
        Serial.println("[LOGGER] Session already active");
        return false;
    }

    config.mode = mode;
    session_count++;
    status.session_id = session_count;
    status.samples_logged = 0;
    status.bytes_written = 0;
    status.logging_active = true;
    session_start_ms = millis();
    last_sample_ms = session_start_ms;

    // Generate filename
    snprintf(status.filename, sizeof(status.filename),
             "log_%04lu.csv", status.session_id);

    Serial.printf("[LOGGER] Session %lu started: %s\n",
                 status.session_id, status.filename);

    // Open log file (if SD card enabled)
    if (config.enable_sd_card && status.sd_card_ok) {
        // log_file = SD.open(status.filename, FILE_WRITE);
        // if (log_file) {
        //     // Write CSV header
        //     log_file.println("timestamp,rpm,speed,throttle,boost,coolant,voltage,oil_psi,omp_duty,lead_timing,trail_timing,wastegate,knock_retard,lambda,gear,flags");
        // }
    }

    return true;
}

void stopSession() {
    if (!status.logging_active) {
        return;
    }

    status.logging_active = false;

    Serial.printf("[LOGGER] Session %lu stopped\n", status.session_id);
    Serial.printf("[LOGGER] Samples logged: %lu\n", status.samples_logged);
    Serial.printf("[LOGGER] Bytes written: %lu\n", status.bytes_written);

    // Close log file
    if (config.enable_sd_card && status.sd_card_ok) {
        // if (log_file) {
        //     log_file.close();
        // }
    }
}

// ============================================================================
// DATA LOGGING
// ============================================================================

void logSample(const LogEntry& entry) {
    if (!status.logging_active) {
        return;
    }

    // Check if should log this sample (trigger mode)
    if (config.mode == MODE_TRIGGERED && !checkTriggers(entry)) {
        return;
    }

    // Add to circular buffer (always, for crash data)
    circular_buffer[buffer_index] = entry;
    buffer_index = (buffer_index + 1) % BUFFER_SIZE;
    if (buffer_count < BUFFER_SIZE) {
        buffer_count++;
    }
    status.buffer_usage = (buffer_count * 100) / BUFFER_SIZE;

    status.samples_logged++;

    // Write to SD card
    if (config.enable_sd_card && status.sd_card_ok) {
        // Format as CSV
        char line[256];
        snprintf(line, sizeof(line),
                "%lu,%u,%u,%u,%u,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
                entry.timestamp, entry.rpm, entry.speed_kmh, entry.throttle,
                entry.boost_psi, entry.coolant_temp, entry.battery_voltage,
                entry.oil_pressure, entry.omp_duty, entry.leading_timing,
                entry.trailing_timing, entry.wastegate_duty, entry.knock_retard,
                entry.lambda, entry.gear, entry.flags);

        // if (log_file) {
        //     log_file.print(line);
        //     status.bytes_written += strlen(line);
        // }
        status.bytes_written += strlen(line);  // Placeholder
    }

    // Output to serial
    if (config.enable_serial) {
        // Compact format for serial (not full CSV)
        Serial.printf("[LOG] T=%lu RPM=%u SPD=%u THR=%u BST=%u TMP=%d\n",
                     entry.timestamp, entry.rpm, entry.speed_kmh / 10,
                     entry.throttle, entry.boost_psi / 10, entry.coolant_temp / 10);
    }
}

void update() {
    if (!status.logging_active) {
        return;
    }

    // Calculate sample interval
    uint32_t sample_interval_ms = 1000 / config.sample_rate_hz;
    uint32_t now = millis();

    // Time to take a sample?
    if ((now - last_sample_ms) >= sample_interval_ms) {
        // This function is called from main loop
        // Actual sampling is done via logSample() called externally
        last_sample_ms = now;
    }

    // Periodic flush to SD card
    static uint32_t last_flush = 0;
    if ((now - last_flush) > 1000) {  // Flush every second
        if (config.enable_sd_card && status.sd_card_ok) {
            // if (log_file) {
            //     log_file.flush();
            // }
        }
        last_flush = now;
    }

    // Check file size limit
    if (status.bytes_written > (config.max_log_size_mb * 1024 * 1024)) {
        Serial.println("[LOGGER] Max log size reached, stopping session");
        stopSession();
    }
}

// ============================================================================
// TRIGGERS
// ============================================================================

bool checkTriggers(const LogEntry& entry) {
    // Check all trigger conditions
    bool triggered = false;

    if (config.triggers.trigger_on_knock && (entry.flags & FLAG_KNOCK_DETECTED)) {
        triggered = true;
    }

    if (config.triggers.trigger_on_overboost && (entry.flags & FLAG_OVERBOOST)) {
        triggered = true;
    }

    if (config.triggers.trigger_on_high_rpm && entry.rpm > config.triggers.rpm_threshold) {
        triggered = true;
    }

    if (config.triggers.trigger_on_launch && (entry.flags & FLAG_LAUNCH_ACTIVE)) {
        triggered = true;
    }

    return triggered;
}

// ============================================================================
// DATA RETRIEVAL
// ============================================================================

const LogEntry* getCircularBuffer(uint16_t max_samples) {
    // Return pointer to circular buffer
    // Caller must handle wraparound logic
    return circular_buffer;
}

bool exportToCSV(const char* filename) {
    if (!status.sd_card_ok) {
        Serial.println("[LOGGER] SD card not available");
        return false;
    }

    // TODO: Implement CSV export from circular buffer
    Serial.printf("[LOGGER] Exporting to %s\n", filename);

    return true;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

const LoggerStatus* getStatus() {
    return &status;
}

void configure(const LoggerConfig& new_config) {
    config = new_config;
    Serial.println("[LOGGER] Configuration updated");
    init();
}

void setMode(LogMode mode) {
    config.mode = mode;
    Serial.printf("[LOGGER] Mode set to: %s\n",
                 mode == MODE_OFF ? "OFF" :
                 mode == MODE_CONTINUOUS ? "Continuous" :
                 mode == MODE_TRIGGERED ? "Triggered" : "Circular Buffer");
}

void setSampleRate(uint16_t hz) {
    config.sample_rate_hz = constrain(hz, 1, 100);
    Serial.printf("[LOGGER] Sample rate set to %d Hz\n", config.sample_rate_hz);
}

void setSDCardEnabled(bool enabled) {
    config.enable_sd_card = enabled;
    Serial.printf("[LOGGER] SD card logging %s\n",
                 enabled ? "ENABLED" : "DISABLED");

    if (enabled) {
        init();  // Re-initialize SD card
    }
}

uint32_t getSessionCount() {
    return session_count;
}

} // namespace DataLogger
