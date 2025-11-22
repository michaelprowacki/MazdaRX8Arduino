/**
 * @file wifi_telemetry.h
 * @brief WiFi telemetry for cloud data upload
 *
 * Sends vehicle telemetry data to cloud services:
 * - ThingSpeak (free, easy to use)
 * - MQTT broker (flexible, real-time)
 * - InfluxDB (time-series database)
 * - Custom HTTP endpoint
 *
 * @author Created for Phase 5 unified architecture
 * @date 2025-11-16
 */

#ifndef WIFI_TELEMETRY_H
#define WIFI_TELEMETRY_H

#include <stdint.h>
#include "../communication/uart_bridge.h"

namespace WiFiTelemetry {

/**
 * @brief Telemetry backend enumeration
 */
enum Backend {
    BACKEND_THINGSPEAK,  // ThingSpeak cloud
    BACKEND_MQTT,        // MQTT broker
    BACKEND_INFLUXDB,    // InfluxDB time-series DB
    BACKEND_HTTP_POST    // Custom HTTP POST endpoint
};

/**
 * @brief Initialize WiFi telemetry
 *
 * @param backend Telemetry backend to use
 */
void init(Backend backend = BACKEND_THINGSPEAK);

/**
 * @brief Send vehicle state update
 *
 * @param state Vehicle state structure
 * @return true if sent successfully
 */
bool sendUpdate(const UARTBridge::VehicleState& state);

/**
 * @brief Set update interval
 *
 * Controls how often data is sent to cloud.
 * Longer intervals = less data usage, less real-time.
 *
 * @param interval_ms Update interval in milliseconds (min: 15000ms for ThingSpeak)
 */
void setUpdateInterval(uint32_t interval_ms);

/**
 * @brief Get time since last update
 * @return Milliseconds since last successful update
 */
uint32_t getTimeSinceLastUpdate();

/**
 * @brief Get total updates sent
 * @return Number of successful updates
 */
uint32_t getUpdateCount();

/**
 * @brief Get error count
 * @return Number of failed updates
 */
uint32_t getErrorCount();

/**
 * @brief Configure ThingSpeak
 *
 * @param api_key Write API key from ThingSpeak channel
 * @param channel_id Channel ID (optional, can be in API key URL)
 */
void configureThingSpeak(const char* api_key, uint32_t channel_id = 0);

/**
 * @brief Configure MQTT
 *
 * @param broker MQTT broker address (e.g., "mqtt.example.com")
 * @param port MQTT port (default: 1883)
 * @param username Username for authentication (optional)
 * @param password Password for authentication (optional)
 * @param topic Topic to publish to (e.g., "rx8/telemetry")
 */
void configureMQTT(const char* broker, uint16_t port = 1883,
                   const char* username = nullptr,
                   const char* password = nullptr,
                   const char* topic = "rx8/telemetry");

/**
 * @brief Configure InfluxDB
 *
 * @param url InfluxDB URL (e.g., "http://influxdb.example.com:8086")
 * @param database Database name
 * @param measurement Measurement name (e.g., "vehicle_data")
 * @param username Username (optional)
 * @param password Password (optional)
 */
void configureInfluxDB(const char* url, const char* database,
                       const char* measurement = "vehicle_data",
                       const char* username = nullptr,
                       const char* password = nullptr);

/**
 * @brief Configure HTTP POST
 *
 * @param url Endpoint URL (e.g., "https://api.example.com/telemetry")
 * @param auth_header Authorization header (optional, e.g., "Bearer token123")
 */
void configureHTTP(const char* url, const char* auth_header = nullptr);

/**
 * @brief Check if WiFi is connected
 * @return true if WiFi connected
 */
bool isConnected();

/**
 * @brief Get WiFi RSSI (signal strength)
 * @return RSSI in dBm (-100 to 0, higher is better)
 */
int8_t getRSSI();

} // namespace WiFiTelemetry

#endif // WIFI_TELEMETRY_H
