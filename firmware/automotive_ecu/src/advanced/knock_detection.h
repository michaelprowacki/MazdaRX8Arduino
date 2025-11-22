/**
 * @file knock_detection.h
 * @brief Knock detection and protection for 13B-MSP engine
 *
 * Features:
 * - Analog knock sensor input processing
 * - Digital knock sensor support (piezo)
 * - Frequency-selective knock detection (5-15 kHz)
 * - Adaptive knock threshold
 * - Progressive timing retard
 * - Per-cylinder knock detection (rotor-specific)
 * - Knock event logging
 *
 * @author Created for Phase 5+ advanced features
 * @date 2025-11-16
 */

#ifndef KNOCK_DETECTION_H
#define KNOCK_DETECTION_H

#include <stdint.h>

namespace KnockDetection {

/**
 * @brief Knock sensor type
 */
enum SensorType {
    SENSOR_ANALOG,      // Analog knock sensor (0-5V)
    SENSOR_PIEZO,       // Piezo sensor (digital knock controller)
    SENSOR_ACCELEROMETER // Accelerometer-based detection
};

/**
 * @brief Knock detection configuration
 */
struct KnockConfig {
    SensorType sensor_type;      // Type of knock sensor
    uint8_t  sensitivity;        // Detection sensitivity (0-100%)
    uint16_t knock_frequency;    // Target frequency (Hz) - typically 6000-8000
    uint16_t frequency_window;   // Frequency window (±Hz)
    uint8_t  max_retard;         // Maximum timing retard (degrees)
    uint8_t  retard_step;        // Retard per knock event (degrees)
    uint8_t  recovery_rate;      // Recovery rate (degrees/second)
    bool     enable_protection;  // Enable knock protection
};

/**
 * @brief Knock detection status
 */
struct KnockStatus {
    bool     knock_detected;     // Knock currently detected
    uint8_t  knock_intensity;    // Knock intensity (0-100%)
    uint8_t  current_retard;     // Current timing retard (degrees)
    uint32_t knock_count_total;  // Total knock events
    uint32_t knock_count_session; // Knock events this session
    uint16_t knock_rpm;          // RPM when knock detected
    uint8_t  knock_cylinder;     // Cylinder/rotor that knocked (0-1)
};

/**
 * @brief Knock event log entry
 */
struct KnockEvent {
    uint32_t timestamp;          // Milliseconds since boot
    uint16_t rpm;                // RPM when knock occurred
    uint8_t  throttle;           // Throttle position
    uint8_t  boost;              // Boost pressure (PSI * 10)
    uint8_t  intensity;          // Knock intensity
    uint8_t  cylinder;           // Rotor that knocked
};

/**
 * @brief Initialize knock detection
 */
void init();

/**
 * @brief Update knock detection
 *
 * Call this frequently (every engine cycle or faster)
 *
 * @param rpm Current engine RPM
 * @param throttle Throttle position (0-100%)
 * @param boost Boost pressure (PSI * 10)
 * @return true if knock detected this cycle
 */
bool update(uint16_t rpm, uint8_t throttle, uint16_t boost);

/**
 * @brief Read knock sensor value
 *
 * @return Raw knock sensor value (0-1023 for analog)
 */
uint16_t readSensor();

/**
 * @brief Detect knock from sensor reading
 *
 * Uses frequency-selective detection and adaptive threshold
 *
 * @param sensor_value Raw sensor reading
 * @param rpm Current RPM
 * @return Knock intensity (0-100%), 0 = no knock
 */
uint8_t detectKnock(uint16_t sensor_value, uint16_t rpm);

/**
 * @brief Calculate timing retard based on knock
 *
 * Progressive retard with recovery
 *
 * @param knock_detected true if knock detected this cycle
 * @return Current timing retard (degrees)
 */
uint8_t calculateRetard(bool knock_detected);

/**
 * @brief Get adaptive knock threshold
 *
 * Threshold adapts to background noise level
 *
 * @param rpm Current RPM
 * @return Knock threshold value
 */
uint16_t getAdaptiveThreshold(uint16_t rpm);

/**
 * @brief Log knock event
 *
 * @param rpm RPM when knock occurred
 * @param throttle Throttle position
 * @param boost Boost pressure
 * @param intensity Knock intensity
 * @param cylinder Rotor that knocked
 */
void logKnockEvent(uint16_t rpm, uint8_t throttle, uint16_t boost,
                   uint8_t intensity, uint8_t cylinder);

/**
 * @brief Get knock detection status
 * @return Status structure
 */
const KnockStatus* getStatus();

/**
 * @brief Get knock event log
 *
 * @param max_events Maximum events to retrieve
 * @return Array of knock events (newest first)
 */
const KnockEvent* getEventLog(uint8_t max_events);

/**
 * @brief Configure knock detection
 * @param config Configuration structure
 */
void configure(const KnockConfig& config);

/**
 * @brief Enable/disable knock protection
 *
 * @param enabled true to enable protection
 */
void setEnabled(bool enabled);

/**
 * @brief Reset knock counters
 */
void resetCounters();

/**
 * @brief Clear knock event log
 */
void clearEventLog();

} // namespace KnockDetection

#endif // KNOCK_DETECTION_H
