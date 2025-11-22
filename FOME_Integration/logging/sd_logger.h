/*
 * SD Card Data Logger
 *
 * High-performance logging to SD card for ECU data
 * Supports multiple log formats: CSV, binary, and MegaSquirt MLG
 *
 * Features:
 * - Circular buffer for non-blocking writes
 * - Configurable sample rates
 * - Automatic file rotation
 * - Session timestamps
 */

#ifndef SD_LOGGER_H
#define SD_LOGGER_H

#include <stdint.h>
#include <stdbool.h>

// Log format types
enum LogFormat {
    LOG_FORMAT_CSV = 0,     // Human-readable CSV
    LOG_FORMAT_BINARY = 1,  // Compact binary
    LOG_FORMAT_MLG = 2      // MegaSquirt MLG format
};

// Log rate options (samples per second)
enum LogRate {
    LOG_RATE_1HZ = 1,
    LOG_RATE_10HZ = 10,
    LOG_RATE_20HZ = 20,
    LOG_RATE_50HZ = 50,
    LOG_RATE_100HZ = 100
};

// Logging channel definitions
#define LOG_CH_RPM          0x0001
#define LOG_CH_SPEED        0x0002
#define LOG_CH_THROTTLE     0x0004
#define LOG_CH_COOLANT      0x0008
#define LOG_CH_INTAKE_TEMP  0x0010
#define LOG_CH_MAP          0x0020
#define LOG_CH_MAF          0x0040
#define LOG_CH_AFR          0x0080
#define LOG_CH_FUEL_PRESS   0x0100
#define LOG_CH_OIL_PRESS    0x0200
#define LOG_CH_OIL_TEMP     0x0400
#define LOG_CH_BATTERY      0x0800
#define LOG_CH_TIMING       0x1000
#define LOG_CH_FUEL_TRIM    0x2000
#define LOG_CH_GEAR         0x4000
#define LOG_CH_GPS          0x8000

// Default channels for typical logging
#define LOG_CHANNELS_BASIC  (LOG_CH_RPM | LOG_CH_SPEED | LOG_CH_THROTTLE | LOG_CH_COOLANT)
#define LOG_CHANNELS_FULL   (LOG_CHANNELS_BASIC | LOG_CH_INTAKE_TEMP | LOG_CH_MAP | \
                             LOG_CH_AFR | LOG_CH_BATTERY | LOG_CH_TIMING)

// Buffer configuration
#define LOG_BUFFER_SIZE     4096
#define LOG_FILENAME_LEN    32
#define LOG_MAX_FILE_SIZE   (10 * 1024 * 1024)  // 10MB per file

// Log data structure (one sample)
typedef struct __attribute__((packed)) {
    uint32_t timestamp;     // Milliseconds since start
    uint16_t rpm;           // Engine RPM
    uint8_t speed;          // Vehicle speed (km/h)
    uint8_t throttle;       // Throttle position (%)
    int8_t coolantTemp;     // Coolant temp (°C)
    int8_t intakeTemp;      // Intake temp (°C)
    uint8_t map;            // MAP (kPa)
    uint16_t maf;           // MAF (g/s * 100)
    uint8_t afr;            // AFR * 10
    uint16_t fuelPressure;  // Fuel pressure (kPa)
    uint8_t oilPressure;    // Oil pressure (psi)
    uint8_t oilTemp;        // Oil temp (°C)
    uint16_t batteryMv;     // Battery voltage (mV)
    int8_t timing;          // Timing advance (°)
    int8_t fuelTrim;        // Fuel trim (%)
    uint8_t gear;           // Current gear
    int32_t latitude;       // GPS latitude * 1e6
    int32_t longitude;      // GPS longitude * 1e6
} LogSample;

// Logger statistics
typedef struct {
    uint32_t samplesWritten;
    uint32_t bytesWritten;
    uint32_t bufferOverruns;
    uint32_t writeErrors;
    uint16_t avgWriteTimeUs;
    uint8_t bufferUsagePercent;
} LoggerStats;

// SD Logger class
class SdLogger {
public:
    // Initialize logger
    bool init();

    // Start logging session
    bool startSession(const char* sessionName = nullptr);

    // Stop logging session
    void stopSession();

    // Check if logging is active
    bool isLogging() { return m_logging; }

    // Configure logging
    void setFormat(LogFormat format) { m_format = format; }
    void setRate(LogRate rate) { m_sampleRate = rate; }
    void setChannels(uint16_t channels) { m_channels = channels; }
    void setMaxFileSize(uint32_t bytes) { m_maxFileSize = bytes; }

    // Log current data (call at configured rate)
    void logSample(const LogSample& sample);

    // Update individual values (alternative to full sample)
    void setRpm(uint16_t rpm) { m_currentSample.rpm = rpm; }
    void setSpeed(uint8_t speed) { m_currentSample.speed = speed; }
    void setThrottle(uint8_t throttle) { m_currentSample.throttle = throttle; }
    void setCoolantTemp(int8_t temp) { m_currentSample.coolantTemp = temp; }
    void setIntakeTemp(int8_t temp) { m_currentSample.intakeTemp = temp; }
    void setMap(uint8_t map) { m_currentSample.map = map; }
    void setMaf(uint16_t maf) { m_currentSample.maf = maf; }
    void setAfr(uint8_t afr) { m_currentSample.afr = afr; }
    void setFuelPressure(uint16_t pressure) { m_currentSample.fuelPressure = pressure; }
    void setOilPressure(uint8_t pressure) { m_currentSample.oilPressure = pressure; }
    void setOilTemp(uint8_t temp) { m_currentSample.oilTemp = temp; }
    void setBatteryMv(uint16_t mv) { m_currentSample.batteryMv = mv; }
    void setTiming(int8_t timing) { m_currentSample.timing = timing; }
    void setFuelTrim(int8_t trim) { m_currentSample.fuelTrim = trim; }
    void setGear(uint8_t gear) { m_currentSample.gear = gear; }
    void setGps(int32_t lat, int32_t lon) {
        m_currentSample.latitude = lat;
        m_currentSample.longitude = lon;
    }

    // Log current sample (uses values set by setters)
    void logCurrentSample();

    // Flush buffer to SD card
    void flush();

    // Process logging (call from main loop)
    void process();

    // Get statistics
    const LoggerStats& getStats() { return m_stats; }

    // Reset statistics
    void resetStats();

    // Get free space on SD card (bytes)
    uint32_t getFreeSpace();

    // List log files
    uint8_t listFiles(char files[][LOG_FILENAME_LEN], uint8_t maxFiles);

    // Delete log file
    bool deleteFile(const char* filename);

private:
    // Configuration
    LogFormat m_format;
    LogRate m_sampleRate;
    uint16_t m_channels;
    uint32_t m_maxFileSize;

    // State
    bool m_initialized;
    bool m_logging;
    uint32_t m_sessionStart;
    uint32_t m_lastSampleTime;
    uint32_t m_fileSize;
    uint8_t m_fileIndex;
    char m_filename[LOG_FILENAME_LEN];

    // Current sample data
    LogSample m_currentSample;

    // Circular buffer
    uint8_t m_buffer[LOG_BUFFER_SIZE];
    volatile uint16_t m_bufferHead;
    volatile uint16_t m_bufferTail;

    // Statistics
    LoggerStats m_stats;

    // Internal methods
    void writeToBuffer(const uint8_t* data, uint16_t length);
    void flushBuffer();
    void writeHeader();
    void writeCsvSample(const LogSample& sample);
    void writeBinarySample(const LogSample& sample);
    void writeMlgSample(const LogSample& sample);
    bool createNewFile();
    bool openNextFile();
    uint16_t bufferUsed();
};

// -----------------------------------------
// Implementation
// -----------------------------------------

inline bool SdLogger::init() {
    m_format = LOG_FORMAT_CSV;
    m_sampleRate = LOG_RATE_10HZ;
    m_channels = LOG_CHANNELS_BASIC;
    m_maxFileSize = LOG_MAX_FILE_SIZE;
    m_logging = false;
    m_bufferHead = 0;
    m_bufferTail = 0;
    m_fileIndex = 0;

    resetStats();

    // Initialize SD card (platform-specific)
    m_initialized = sdCardInit();

    return m_initialized;
}

inline bool SdLogger::startSession(const char* sessionName) {
    if (!m_initialized || m_logging) {
        return false;
    }

    // Generate filename
    if (sessionName) {
        snprintf(m_filename, LOG_FILENAME_LEN, "%s.log", sessionName);
    } else {
        // Use timestamp
        snprintf(m_filename, LOG_FILENAME_LEN, "log_%lu.log",
                 (unsigned long)platformMillis());
    }

    if (!createNewFile()) {
        return false;
    }

    writeHeader();

    m_sessionStart = platformMillis();
    m_lastSampleTime = m_sessionStart;
    m_fileSize = 0;
    m_logging = true;

    return true;
}

inline void SdLogger::stopSession() {
    if (!m_logging) {
        return;
    }

    flush();
    sdCardClose();
    m_logging = false;
}

inline void SdLogger::logSample(const LogSample& sample) {
    if (!m_logging) {
        return;
    }

    // Check sample rate
    uint32_t now = platformMillis();
    uint32_t interval = 1000 / m_sampleRate;
    if (now - m_lastSampleTime < interval) {
        return;
    }
    m_lastSampleTime = now;

    // Write based on format
    switch (m_format) {
        case LOG_FORMAT_CSV:
            writeCsvSample(sample);
            break;
        case LOG_FORMAT_BINARY:
            writeBinarySample(sample);
            break;
        case LOG_FORMAT_MLG:
            writeMlgSample(sample);
            break;
    }

    m_stats.samplesWritten++;

    // Check file rotation
    if (m_fileSize >= m_maxFileSize) {
        openNextFile();
    }
}

inline void SdLogger::logCurrentSample() {
    m_currentSample.timestamp = platformMillis() - m_sessionStart;
    logSample(m_currentSample);
}

inline void SdLogger::flush() {
    flushBuffer();
}

inline void SdLogger::process() {
    // Flush buffer periodically
    if (bufferUsed() > LOG_BUFFER_SIZE / 2) {
        flushBuffer();
    }
}

inline void SdLogger::resetStats() {
    m_stats.samplesWritten = 0;
    m_stats.bytesWritten = 0;
    m_stats.bufferOverruns = 0;
    m_stats.writeErrors = 0;
    m_stats.avgWriteTimeUs = 0;
    m_stats.bufferUsagePercent = 0;
}

inline void SdLogger::writeToBuffer(const uint8_t* data, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        uint16_t nextHead = (m_bufferHead + 1) % LOG_BUFFER_SIZE;
        if (nextHead == m_bufferTail) {
            // Buffer full
            m_stats.bufferOverruns++;
            return;
        }
        m_buffer[m_bufferHead] = data[i];
        m_bufferHead = nextHead;
    }
    m_fileSize += length;
    m_stats.bytesWritten += length;
}

inline void SdLogger::flushBuffer() {
    while (m_bufferTail != m_bufferHead) {
        uint8_t byte = m_buffer[m_bufferTail];
        if (!sdCardWrite(&byte, 1)) {
            m_stats.writeErrors++;
            break;
        }
        m_bufferTail = (m_bufferTail + 1) % LOG_BUFFER_SIZE;
    }
    sdCardFlush();
}

inline uint16_t SdLogger::bufferUsed() {
    if (m_bufferHead >= m_bufferTail) {
        return m_bufferHead - m_bufferTail;
    }
    return LOG_BUFFER_SIZE - m_bufferTail + m_bufferHead;
}

inline void SdLogger::writeHeader() {
    if (m_format == LOG_FORMAT_CSV) {
        const char* header = "Time,RPM,Speed,Throttle,Coolant,Intake,MAP,MAF,AFR,"
                            "FuelPress,OilPress,OilTemp,Battery,Timing,FuelTrim,Gear\n";
        writeToBuffer((const uint8_t*)header, strlen(header));
    } else if (m_format == LOG_FORMAT_BINARY) {
        // Binary header with format version
        uint8_t header[8] = {'F', 'O', 'M', 'E', 0x01, 0x00,
                             (uint8_t)m_sampleRate, (uint8_t)(m_channels >> 8)};
        writeToBuffer(header, 8);
    }
}

inline void SdLogger::writeCsvSample(const LogSample& sample) {
    char line[128];
    int len = snprintf(line, sizeof(line),
        "%lu,%u,%u,%u,%d,%d,%u,%u,%u,%u,%u,%u,%u,%d,%d,%u\n",
        (unsigned long)sample.timestamp,
        sample.rpm, sample.speed, sample.throttle,
        sample.coolantTemp, sample.intakeTemp, sample.map,
        sample.maf, sample.afr, sample.fuelPressure,
        sample.oilPressure, sample.oilTemp, sample.batteryMv,
        sample.timing, sample.fuelTrim, sample.gear);
    writeToBuffer((const uint8_t*)line, len);
}

inline void SdLogger::writeBinarySample(const LogSample& sample) {
    writeToBuffer((const uint8_t*)&sample, sizeof(LogSample));
}

inline void SdLogger::writeMlgSample(const LogSample& sample) {
    // MegaSquirt MLG format (simplified)
    writeBinarySample(sample);
}

inline bool SdLogger::createNewFile() {
    return sdCardOpen(m_filename, true);
}

inline bool SdLogger::openNextFile() {
    flush();
    sdCardClose();
    m_fileIndex++;
    snprintf(m_filename, LOG_FILENAME_LEN, "log_%u_%u.log",
             (unsigned)(m_sessionStart / 1000), m_fileIndex);
    m_fileSize = 0;
    if (!createNewFile()) {
        return false;
    }
    writeHeader();
    return true;
}

// Platform-specific functions (to be implemented)
extern bool sdCardInit();
extern bool sdCardOpen(const char* filename, bool write);
extern void sdCardClose();
extern bool sdCardWrite(const uint8_t* data, uint16_t length);
extern void sdCardFlush();
extern uint32_t sdCardFreeSpace();
extern uint32_t platformMillis();
extern int snprintf(char* str, size_t size, const char* format, ...);
extern size_t strlen(const char* str);

// Global instance
extern SdLogger sdLogger;

#endif // SD_LOGGER_H
