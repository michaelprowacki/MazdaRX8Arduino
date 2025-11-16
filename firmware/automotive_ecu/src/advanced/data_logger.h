/**
 * @file data_logger.h
 * @brief Black box data logging for engine parameters
 *
 * Features:
 * - High-speed data logging (up to 100 Hz)
 * - SD card storage
 * - CSV export format
 * - Circular buffer for crash data
 * - Trigger-based logging
 * - Session management
 * - Data compression
 *
 * @author Created for Phase 5+ advanced features
 * @date 2025-11-16
 */

#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <stdint.h>

namespace DataLogger {

/**
 * @brief Logging mode
 */
enum LogMode {
    MODE_OFF,               // Logging disabled
    MODE_CONTINUOUS,        // Log all data continuously
    MODE_TRIGGERED,         // Log only when trigger conditions met
    MODE_CIRCULAR_BUFFER    // Keep last N seconds in RAM (crash data)
};

/**
 * @brief Trigger conditions
 */
struct TriggerConfig {
    bool trigger_on_knock;      // Log when knock detected
    bool trigger_on_overboost;  // Log when overboost
    bool trigger_on_high_rpm;   // Log above RPM threshold
    bool trigger_on_launch;     // Log during launch control
    uint16_t rpm_threshold;     // RPM trigger threshold
    uint16_t boost_threshold;   // Boost trigger (PSI * 10)
};

/**
 * @brief Data log entry (one sample)
 */
struct LogEntry {
    uint32_t timestamp;         // Milliseconds since session start
    uint16_t rpm;               // Engine RPM
    uint16_t speed_kmh;         // Vehicle speed (km/h * 10)
    uint8_t  throttle;          // Throttle position (0-100%)
    uint16_t boost_psi;         // Boost pressure (PSI * 10)
    int16_t  coolant_temp;      // Coolant temp (°C * 10)
    uint16_t battery_voltage;   // Battery voltage (V * 100)
    uint8_t  oil_pressure;      // Oil pressure (PSI)
    uint8_t  omp_duty;          // OMP duty cycle (0-100%)
    uint8_t  leading_timing;    // Leading spark timing (degrees BTDC)
    uint8_t  trailing_timing;   // Trailing spark timing (degrees BTDC)
    uint8_t  wastegate_duty;    // Wastegate duty (0-100%)
    uint8_t  knock_retard;      // Knock timing retard (degrees)
    uint16_t lambda;            // AFR lambda (lambda * 1000)
    uint8_t  gear;              // Current gear (0-6)
    uint8_t  flags;             // Status flags (bitfield)
};

/**
 * @brief Status flags bitfield
 */
#define FLAG_KNOCK_DETECTED     (1 << 0)
#define FLAG_LAUNCH_ACTIVE      (1 << 1)
#define FLAG_ANTILAG_ACTIVE     (1 << 2)
#define FLAG_OVERBOOST          (1 << 3)
#define FLAG_CHECK_ENGINE       (1 << 4)
#define FLAG_LOW_OIL            (1 << 5)
#define FLAG_OVERTEMP           (1 << 6)

/**
 * @brief Logger configuration
 */
struct LoggerConfig {
    LogMode mode;               // Logging mode
    uint16_t sample_rate_hz;    // Samples per second (1-100 Hz)
    uint32_t max_log_size_mb;   // Maximum log file size (MB)
    bool enable_sd_card;        // Use SD card storage
    bool enable_serial;         // Output to serial port
    TriggerConfig triggers;     // Trigger configuration
};

/**
 * @brief Logger status
 */
struct LoggerStatus {
    bool     logging_active;    // Currently logging
    uint32_t samples_logged;    // Total samples this session
    uint32_t bytes_written;     // Bytes written to storage
    uint32_t session_id;        // Current session ID
    char     filename[64];      // Current log filename
    bool     sd_card_ok;        // SD card available
    uint16_t buffer_usage;      // Circular buffer usage (%)
};

/**
 * @brief Initialize data logger
 */
void init();

/**
 * @brief Start logging session
 *
 * @param mode Logging mode
 * @return true if started successfully
 */
bool startSession(LogMode mode = MODE_CONTINUOUS);

/**
 * @brief Stop logging session
 */
void stopSession();

/**
 * @brief Log one data sample
 *
 * Call this at the configured sample rate
 *
 * @param entry Data entry to log
 */
void logSample(const LogEntry& entry);

/**
 * @brief Update logger (call from main loop)
 *
 * Handles buffering, writing to storage, etc.
 */
void update();

/**
 * @brief Check if trigger conditions met
 *
 * @param entry Current data
 * @return true if should log this sample
 */
bool checkTriggers(const LogEntry& entry);

/**
 * @brief Get circular buffer data (crash data)
 *
 * Returns last N seconds of data from RAM buffer
 *
 * @param max_samples Maximum samples to retrieve
 * @return Pointer to buffer data
 */
const LogEntry* getCircularBuffer(uint16_t max_samples);

/**
 * @brief Export log to CSV file
 *
 * @param filename Output filename
 * @return true if successful
 */
bool exportToCSV(const char* filename);

/**
 * @brief Get logger status
 * @return Status structure
 */
const LoggerStatus* getStatus();

/**
 * @brief Configure logger
 * @param config Configuration structure
 */
void configure(const LoggerConfig& config);

/**
 * @brief Set logging mode
 *
 * @param mode Logging mode
 */
void setMode(LogMode mode);

/**
 * @brief Set sample rate
 *
 * @param hz Sample rate in Hz (1-100)
 */
void setSampleRate(uint16_t hz);

/**
 * @brief Enable/disable SD card logging
 *
 * @param enabled true to enable
 */
void setSDCardEnabled(bool enabled);

/**
 * @brief Get total sessions logged
 *
 * @return Number of logging sessions
 */
uint32_t getSessionCount();

} // namespace DataLogger

#endif // DATA_LOGGER_H
