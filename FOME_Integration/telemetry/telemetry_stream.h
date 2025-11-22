/*
 * Real-Time Telemetry Streaming
 *
 * Stream ECU data over WiFi/Bluetooth/Serial for live monitoring
 * Supports multiple protocols: JSON, Binary, MQTT
 *
 * Features:
 * - Multiple simultaneous clients
 * - Configurable update rates per channel
 * - Delta compression for bandwidth efficiency
 * - WebSocket support for web dashboards
 */

#ifndef TELEMETRY_STREAM_H
#define TELEMETRY_STREAM_H

#include <stdint.h>
#include <stdbool.h>

// Transport types
enum TelemetryTransport {
    TRANSPORT_SERIAL = 0,
    TRANSPORT_WIFI_TCP = 1,
    TRANSPORT_WIFI_UDP = 2,
    TRANSPORT_BLUETOOTH = 3,
    TRANSPORT_WEBSOCKET = 4,
    TRANSPORT_MQTT = 5
};

// Protocol formats
enum TelemetryProtocol {
    PROTOCOL_JSON = 0,      // Human-readable JSON
    PROTOCOL_BINARY = 1,    // Compact binary
    PROTOCOL_MSGPACK = 2,   // MessagePack
    PROTOCOL_PROTOBUF = 3   // Protocol Buffers
};

// Channel IDs
enum TelemetryChannel {
    TEL_CH_RPM = 0,
    TEL_CH_SPEED,
    TEL_CH_THROTTLE,
    TEL_CH_COOLANT_TEMP,
    TEL_CH_INTAKE_TEMP,
    TEL_CH_MAP,
    TEL_CH_MAF,
    TEL_CH_AFR,
    TEL_CH_FUEL_PRESSURE,
    TEL_CH_OIL_PRESSURE,
    TEL_CH_OIL_TEMP,
    TEL_CH_BATTERY_V,
    TEL_CH_TIMING,
    TEL_CH_FUEL_TRIM,
    TEL_CH_GEAR,
    TEL_CH_LATITUDE,
    TEL_CH_LONGITUDE,
    TEL_CH_ACCEL_X,
    TEL_CH_ACCEL_Y,
    TEL_CH_ACCEL_Z,
    TEL_CH_GYRO_X,
    TEL_CH_GYRO_Y,
    TEL_CH_GYRO_Z,
    TEL_CH_LAP_TIME,
    TEL_CH_BEST_LAP,
    TEL_CH_COUNT
};

// Channel configuration
typedef struct {
    bool enabled;
    uint8_t updateRateHz;   // Update rate for this channel
    bool deltaOnly;         // Only send when changed
    uint16_t threshold;     // Minimum change to send (for delta)
} ChannelConfig;

// Telemetry packet header
typedef struct __attribute__((packed)) {
    uint8_t magic;          // 0xFE
    uint8_t version;        // Protocol version
    uint16_t sequence;      // Packet sequence number
    uint32_t timestamp;     // Milliseconds
    uint8_t numChannels;    // Number of channels in packet
} TelemetryHeader;

// Channel data in packet
typedef struct __attribute__((packed)) {
    uint8_t channelId;
    int32_t value;          // Scaled integer value
} ChannelData;

// Client connection
#define MAX_CLIENTS 4

typedef struct {
    bool connected;
    TelemetryTransport transport;
    TelemetryProtocol protocol;
    uint32_t lastSendTime;
    uint16_t subscriptions;     // Bitmask of subscribed channels
    void* handle;               // Transport-specific handle
} TelemetryClient;

// Telemetry streamer class
class TelemetryStream {
public:
    // Initialize telemetry system
    bool init(TelemetryTransport transport = TRANSPORT_SERIAL);

    // Start/stop streaming
    bool start();
    void stop();
    bool isRunning() { return m_running; }

    // Configure channels
    void setChannelConfig(TelemetryChannel ch, const ChannelConfig& config);
    void enableChannel(TelemetryChannel ch, uint8_t rateHz = 10);
    void disableChannel(TelemetryChannel ch);
    void setDeltaMode(TelemetryChannel ch, bool delta, uint16_t threshold = 0);

    // Set protocol
    void setProtocol(TelemetryProtocol protocol) { m_protocol = protocol; }

    // Update channel values
    void setValue(TelemetryChannel ch, int32_t value);

    // Convenience setters (with proper scaling)
    void setRpm(uint16_t rpm) { setValue(TEL_CH_RPM, rpm); }
    void setSpeed(uint8_t kmh) { setValue(TEL_CH_SPEED, kmh); }
    void setThrottle(uint8_t percent) { setValue(TEL_CH_THROTTLE, percent); }
    void setCoolantTemp(int8_t celsius) { setValue(TEL_CH_COOLANT_TEMP, celsius); }
    void setIntakeTemp(int8_t celsius) { setValue(TEL_CH_INTAKE_TEMP, celsius); }
    void setMap(uint8_t kpa) { setValue(TEL_CH_MAP, kpa); }
    void setMaf(uint16_t gs100) { setValue(TEL_CH_MAF, gs100); }
    void setAfr(uint8_t afr10) { setValue(TEL_CH_AFR, afr10); }
    void setFuelPressure(uint16_t kpa) { setValue(TEL_CH_FUEL_PRESSURE, kpa); }
    void setOilPressure(uint8_t psi) { setValue(TEL_CH_OIL_PRESSURE, psi); }
    void setOilTemp(uint8_t celsius) { setValue(TEL_CH_OIL_TEMP, celsius); }
    void setBatteryVoltage(uint16_t mv) { setValue(TEL_CH_BATTERY_V, mv); }
    void setTiming(int8_t degrees) { setValue(TEL_CH_TIMING, degrees); }
    void setFuelTrim(int8_t percent) { setValue(TEL_CH_FUEL_TRIM, percent); }
    void setGear(uint8_t gear) { setValue(TEL_CH_GEAR, gear); }
    void setGps(int32_t lat, int32_t lon) {
        setValue(TEL_CH_LATITUDE, lat);
        setValue(TEL_CH_LONGITUDE, lon);
    }
    void setAccel(int16_t x, int16_t y, int16_t z) {
        setValue(TEL_CH_ACCEL_X, x);
        setValue(TEL_CH_ACCEL_Y, y);
        setValue(TEL_CH_ACCEL_Z, z);
    }
    void setGyro(int16_t x, int16_t y, int16_t z) {
        setValue(TEL_CH_GYRO_X, x);
        setValue(TEL_CH_GYRO_Y, y);
        setValue(TEL_CH_GYRO_Z, z);
    }
    void setLapTime(uint32_t ms) { setValue(TEL_CH_LAP_TIME, ms); }
    void setBestLap(uint32_t ms) { setValue(TEL_CH_BEST_LAP, ms); }

    // Process telemetry (call from main loop)
    void process();

    // Client management
    bool acceptClient();
    void disconnectClient(uint8_t clientId);
    uint8_t getClientCount();

    // Get bandwidth usage
    uint32_t getBytesPerSecond();

private:
    // Configuration
    TelemetryTransport m_transport;
    TelemetryProtocol m_protocol;
    ChannelConfig m_channelConfig[TEL_CH_COUNT];

    // State
    bool m_initialized;
    bool m_running;
    uint16_t m_sequence;
    uint32_t m_startTime;

    // Channel values
    int32_t m_values[TEL_CH_COUNT];
    int32_t m_lastSentValues[TEL_CH_COUNT];
    uint32_t m_lastUpdateTime[TEL_CH_COUNT];

    // Clients
    TelemetryClient m_clients[MAX_CLIENTS];

    // Statistics
    uint32_t m_bytesSent;
    uint32_t m_statStartTime;

    // Internal methods
    void sendToClient(uint8_t clientId);
    void buildJsonPacket(uint8_t* buffer, uint16_t* length);
    void buildBinaryPacket(uint8_t* buffer, uint16_t* length);
    bool shouldSendChannel(TelemetryChannel ch);
    void sendPacket(uint8_t clientId, uint8_t* data, uint16_t length);
};

// -----------------------------------------
// Implementation
// -----------------------------------------

inline bool TelemetryStream::init(TelemetryTransport transport) {
    m_transport = transport;
    m_protocol = PROTOCOL_JSON;
    m_running = false;
    m_sequence = 0;
    m_bytesSent = 0;

    // Initialize all channels with defaults
    for (int i = 0; i < TEL_CH_COUNT; i++) {
        m_channelConfig[i].enabled = false;
        m_channelConfig[i].updateRateHz = 10;
        m_channelConfig[i].deltaOnly = false;
        m_channelConfig[i].threshold = 0;
        m_values[i] = 0;
        m_lastSentValues[i] = 0;
        m_lastUpdateTime[i] = 0;
    }

    // Enable common channels by default
    enableChannel(TEL_CH_RPM, 20);
    enableChannel(TEL_CH_SPEED, 10);
    enableChannel(TEL_CH_THROTTLE, 20);
    enableChannel(TEL_CH_COOLANT_TEMP, 1);
    enableChannel(TEL_CH_BATTERY_V, 1);

    // Initialize clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        m_clients[i].connected = false;
    }

    // Initialize transport
    m_initialized = telemetryTransportInit(transport);

    return m_initialized;
}

inline bool TelemetryStream::start() {
    if (!m_initialized) {
        return false;
    }
    m_startTime = platformMillis();
    m_statStartTime = m_startTime;
    m_running = true;
    return true;
}

inline void TelemetryStream::stop() {
    m_running = false;
}

inline void TelemetryStream::setChannelConfig(TelemetryChannel ch, const ChannelConfig& config) {
    if (ch < TEL_CH_COUNT) {
        m_channelConfig[ch] = config;
    }
}

inline void TelemetryStream::enableChannel(TelemetryChannel ch, uint8_t rateHz) {
    if (ch < TEL_CH_COUNT) {
        m_channelConfig[ch].enabled = true;
        m_channelConfig[ch].updateRateHz = rateHz;
    }
}

inline void TelemetryStream::disableChannel(TelemetryChannel ch) {
    if (ch < TEL_CH_COUNT) {
        m_channelConfig[ch].enabled = false;
    }
}

inline void TelemetryStream::setDeltaMode(TelemetryChannel ch, bool delta, uint16_t threshold) {
    if (ch < TEL_CH_COUNT) {
        m_channelConfig[ch].deltaOnly = delta;
        m_channelConfig[ch].threshold = threshold;
    }
}

inline void TelemetryStream::setValue(TelemetryChannel ch, int32_t value) {
    if (ch < TEL_CH_COUNT) {
        m_values[ch] = value;
    }
}

inline void TelemetryStream::process() {
    if (!m_running) {
        return;
    }

    // Accept new clients
    acceptClient();

    // Send to each connected client
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (m_clients[i].connected) {
            sendToClient(i);
        }
    }
}

inline bool TelemetryStream::acceptClient() {
    // Find free slot
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!m_clients[i].connected) {
            void* handle = telemetryTransportAccept(m_transport);
            if (handle) {
                m_clients[i].connected = true;
                m_clients[i].transport = m_transport;
                m_clients[i].protocol = m_protocol;
                m_clients[i].lastSendTime = platformMillis();
                m_clients[i].subscriptions = 0xFFFF;  // All channels
                m_clients[i].handle = handle;
                return true;
            }
            break;
        }
    }
    return false;
}

inline void TelemetryStream::disconnectClient(uint8_t clientId) {
    if (clientId < MAX_CLIENTS && m_clients[clientId].connected) {
        telemetryTransportDisconnect(m_clients[clientId].handle);
        m_clients[clientId].connected = false;
    }
}

inline uint8_t TelemetryStream::getClientCount() {
    uint8_t count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (m_clients[i].connected) count++;
    }
    return count;
}

inline uint32_t TelemetryStream::getBytesPerSecond() {
    uint32_t elapsed = platformMillis() - m_statStartTime;
    if (elapsed == 0) return 0;
    return (m_bytesSent * 1000) / elapsed;
}

inline bool TelemetryStream::shouldSendChannel(TelemetryChannel ch) {
    if (!m_channelConfig[ch].enabled) {
        return false;
    }

    uint32_t now = platformMillis();
    uint32_t interval = 1000 / m_channelConfig[ch].updateRateHz;

    if (now - m_lastUpdateTime[ch] < interval) {
        return false;
    }

    if (m_channelConfig[ch].deltaOnly) {
        int32_t delta = m_values[ch] - m_lastSentValues[ch];
        if (delta < 0) delta = -delta;
        if (delta < m_channelConfig[ch].threshold) {
            return false;
        }
    }

    return true;
}

inline void TelemetryStream::sendToClient(uint8_t clientId) {
    uint8_t buffer[256];
    uint16_t length = 0;

    switch (m_clients[clientId].protocol) {
        case PROTOCOL_JSON:
            buildJsonPacket(buffer, &length);
            break;
        case PROTOCOL_BINARY:
        case PROTOCOL_MSGPACK:
        case PROTOCOL_PROTOBUF:
            buildBinaryPacket(buffer, &length);
            break;
    }

    if (length > 0) {
        sendPacket(clientId, buffer, length);
        m_sequence++;
    }
}

inline void TelemetryStream::buildJsonPacket(uint8_t* buffer, uint16_t* length) {
    char* p = (char*)buffer;
    int len = 0;

    len += snprintf(p + len, 256 - len, "{\"t\":%lu", (unsigned long)(platformMillis() - m_startTime));

    // Add channels that need sending
    for (int ch = 0; ch < TEL_CH_COUNT; ch++) {
        if (shouldSendChannel((TelemetryChannel)ch)) {
            const char* names[] = {
                "rpm", "spd", "thr", "clt", "iat", "map", "maf", "afr",
                "fp", "op", "ot", "bat", "tim", "ft", "gear", "lat", "lon",
                "ax", "ay", "az", "gx", "gy", "gz", "lap", "best"
            };
            len += snprintf(p + len, 256 - len, ",\"%s\":%ld",
                           names[ch], (long)m_values[ch]);
            m_lastSentValues[ch] = m_values[ch];
            m_lastUpdateTime[ch] = platformMillis();
        }
    }

    len += snprintf(p + len, 256 - len, "}\n");
    *length = len;
}

inline void TelemetryStream::buildBinaryPacket(uint8_t* buffer, uint16_t* length) {
    TelemetryHeader* header = (TelemetryHeader*)buffer;
    header->magic = 0xFE;
    header->version = 1;
    header->sequence = m_sequence;
    header->timestamp = platformMillis() - m_startTime;

    uint8_t numChannels = 0;
    uint16_t offset = sizeof(TelemetryHeader);

    for (int ch = 0; ch < TEL_CH_COUNT; ch++) {
        if (shouldSendChannel((TelemetryChannel)ch)) {
            ChannelData* data = (ChannelData*)(buffer + offset);
            data->channelId = ch;
            data->value = m_values[ch];
            offset += sizeof(ChannelData);
            numChannels++;

            m_lastSentValues[ch] = m_values[ch];
            m_lastUpdateTime[ch] = platformMillis();
        }
    }

    header->numChannels = numChannels;
    *length = offset;
}

inline void TelemetryStream::sendPacket(uint8_t clientId, uint8_t* data, uint16_t length) {
    if (telemetryTransportSend(m_clients[clientId].handle, data, length)) {
        m_bytesSent += length;
    } else {
        disconnectClient(clientId);
    }
}

// Platform-specific functions (to be implemented)
extern bool telemetryTransportInit(TelemetryTransport transport);
extern void* telemetryTransportAccept(TelemetryTransport transport);
extern void telemetryTransportDisconnect(void* handle);
extern bool telemetryTransportSend(void* handle, uint8_t* data, uint16_t length);
extern uint32_t platformMillis();
extern int snprintf(char* str, size_t size, const char* format, ...);

// Global instance
extern TelemetryStream telemetry;

#endif // TELEMETRY_STREAM_H
