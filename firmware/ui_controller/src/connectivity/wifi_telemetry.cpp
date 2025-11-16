/**
 * @file wifi_telemetry.cpp
 * @brief WiFi telemetry implementation
 *
 * @author Created for Phase 5 unified architecture
 * @date 2025-11-16
 */

#include "wifi_telemetry.h"
#include "../../config/features.h"
#include <Arduino.h>

#if ENABLE_WIFI

#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>  // MQTT library

namespace WiFiTelemetry {

// Configuration
static Backend current_backend = BACKEND_THINGSPEAK;
static uint32_t update_interval_ms = 15000;  // 15 seconds (ThingSpeak minimum)
static uint32_t last_update_time = 0;
static uint32_t update_count = 0;
static uint32_t error_count = 0;

// ThingSpeak config
static String thingspeak_api_key = "";
static uint32_t thingspeak_channel_id = 0;

// MQTT config
static String mqtt_broker = "";
static uint16_t mqtt_port = 1883;
static String mqtt_username = "";
static String mqtt_password = "";
static String mqtt_topic = "rx8/telemetry";
static WiFiClient wifi_client;
static PubSubClient* mqtt_client = nullptr;

// InfluxDB config
static String influxdb_url = "";
static String influxdb_database = "";
static String influxdb_measurement = "vehicle_data";
static String influxdb_username = "";
static String influxdb_password = "";

// HTTP config
static String http_url = "";
static String http_auth_header = "";

// ============================================================================
// INITIALIZATION
// ============================================================================

void init(Backend backend) {
    current_backend = backend;

    // Initialize MQTT client if needed
    if (backend == BACKEND_MQTT && mqtt_client == nullptr) {
        mqtt_client = new PubSubClient(wifi_client);
    }

    Serial.printf("[WIFI TELEMETRY] Initialized with backend: %d\n", backend);
}

// ============================================================================
// SEND UPDATE
// ============================================================================

bool sendUpdate(const UARTBridge::VehicleState& state) {
    // Check if enough time has passed
    if (millis() - last_update_time < update_interval_ms) {
        return false;
    }

    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        error_count++;
        return false;
    }

    bool success = false;

    switch (current_backend) {
        case BACKEND_THINGSPEAK:
            success = sendToThingSpeak(state);
            break;

        case BACKEND_MQTT:
            success = sendToMQTT(state);
            break;

        case BACKEND_INFLUXDB:
            success = sendToInfluxDB(state);
            break;

        case BACKEND_HTTP_POST:
            success = sendToHTTP(state);
            break;
    }

    if (success) {
        update_count++;
        last_update_time = millis();
    } else {
        error_count++;
    }

    return success;
}

// ============================================================================
// BACKEND IMPLEMENTATIONS
// ============================================================================

static bool sendToThingSpeak(const UARTBridge::VehicleState& state) {
    if (thingspeak_api_key.length() == 0) {
        Serial.println("[WIFI TELEMETRY] ThingSpeak API key not configured!");
        return false;
    }

    HTTPClient http;

    // Build ThingSpeak URL
    String url = "http://api.thingspeak.com/update?api_key=" + thingspeak_api_key;
    url += "&field1=" + String(state.rpm);
    url += "&field2=" + String(state.speed_kmh / 10.0f);
    url += "&field3=" + String(state.throttle_percent);
    url += "&field4=" + String(state.coolant_temp / 10.0f);
    url += "&field5=" + String(state.battery_voltage / 100.0f);
    url += "&field6=" + String(state.oil_pressure);
    url += "&field7=" + String(state.boost_pressure);
    url += "&field8=" + String(state.fuel_level);

    http.begin(url);
    int httpCode = http.GET();

    bool success = (httpCode == 200);

    #if ENABLE_SERIAL_DEBUG >= 2
        Serial.printf("[WIFI TELEMETRY] ThingSpeak: %s (code %d)\n",
                     success ? "OK" : "FAILED", httpCode);
    #endif

    http.end();
    return success;
}

static bool sendToMQTT(const UARTBridge::VehicleState& state) {
    if (!mqtt_client) return false;

    // Connect to MQTT broker if not connected
    if (!mqtt_client->connected()) {
        mqtt_client->setServer(mqtt_broker.c_str(), mqtt_port);

        if (mqtt_username.length() > 0) {
            mqtt_client->connect("ESP32_RX8", mqtt_username.c_str(), mqtt_password.c_str());
        } else {
            mqtt_client->connect("ESP32_RX8");
        }

        if (!mqtt_client->connected()) {
            return false;
        }
    }

    // Build JSON payload
    String payload = "{";
    payload += "\"rpm\":" + String(state.rpm) + ",";
    payload += "\"speed\":" + String(state.speed_kmh / 10.0f) + ",";
    payload += "\"throttle\":" + String(state.throttle_percent) + ",";
    payload += "\"coolant_temp\":" + String(state.coolant_temp / 10.0f) + ",";
    payload += "\"battery_voltage\":" + String(state.battery_voltage / 100.0f) + ",";
    payload += "\"oil_pressure\":" + String(state.oil_pressure) + ",";
    payload += "\"boost\":" + String(state.boost_pressure) + ",";
    payload += "\"fuel\":" + String(state.fuel_level);
    payload += "}";

    bool success = mqtt_client->publish(mqtt_topic.c_str(), payload.c_str());

    #if ENABLE_SERIAL_DEBUG >= 2
        Serial.printf("[WIFI TELEMETRY] MQTT: %s\n", success ? "OK" : "FAILED");
    #endif

    return success;
}

static bool sendToInfluxDB(const UARTBridge::VehicleState& state) {
    HTTPClient http;

    // Build InfluxDB write URL
    String url = influxdb_url + "/write?db=" + influxdb_database;

    // Build line protocol payload
    String payload = influxdb_measurement;
    payload += " rpm=" + String(state.rpm);
    payload += ",speed=" + String(state.speed_kmh / 10.0f);
    payload += ",throttle=" + String(state.throttle_percent);
    payload += ",coolant_temp=" + String(state.coolant_temp / 10.0f);
    payload += ",battery_voltage=" + String(state.battery_voltage / 100.0f);
    payload += ",oil_pressure=" + String(state.oil_pressure);
    payload += ",boost_pressure=" + String(state.boost_pressure);
    payload += ",fuel_level=" + String(state.fuel_level);

    http.begin(url);

    // Add authentication if configured
    if (influxdb_username.length() > 0) {
        http.setAuthorization(influxdb_username.c_str(), influxdb_password.c_str());
    }

    int httpCode = http.POST(payload);
    bool success = (httpCode == 204);  // InfluxDB returns 204 on success

    #if ENABLE_SERIAL_DEBUG >= 2
        Serial.printf("[WIFI TELEMETRY] InfluxDB: %s (code %d)\n",
                     success ? "OK" : "FAILED", httpCode);
    #endif

    http.end();
    return success;
}

static bool sendToHTTP(const UARTBridge::VehicleState& state) {
    HTTPClient http;

    // Build JSON payload
    String payload = "{";
    payload += "\"rpm\":" + String(state.rpm) + ",";
    payload += "\"speed\":" + String(state.speed_kmh / 10.0f) + ",";
    payload += "\"throttle\":" + String(state.throttle_percent) + ",";
    payload += "\"coolant_temp\":" + String(state.coolant_temp / 10.0f) + ",";
    payload += "\"battery_voltage\":" + String(state.battery_voltage / 100.0f) + ",";
    payload += "\"timestamp\":" + String(millis());
    payload += "}";

    http.begin(http_url);
    http.addHeader("Content-Type", "application/json");

    if (http_auth_header.length() > 0) {
        http.addHeader("Authorization", http_auth_header);
    }

    int httpCode = http.POST(payload);
    bool success = (httpCode == 200 || httpCode == 201);

    #if ENABLE_SERIAL_DEBUG >= 2
        Serial.printf("[WIFI TELEMETRY] HTTP POST: %s (code %d)\n",
                     success ? "OK" : "FAILED", httpCode);
    #endif

    http.end();
    return success;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void setUpdateInterval(uint32_t interval_ms) {
    if (interval_ms < 15000) {
        Serial.println("[WIFI TELEMETRY] Warning: Interval < 15s not recommended for ThingSpeak");
    }
    update_interval_ms = interval_ms;
}

void configureThingSpeak(const char* api_key, uint32_t channel_id) {
    thingspeak_api_key = String(api_key);
    thingspeak_channel_id = channel_id;
    Serial.printf("[WIFI TELEMETRY] ThingSpeak configured (channel: %d)\n", channel_id);
}

void configureMQTT(const char* broker, uint16_t port,
                   const char* username, const char* password,
                   const char* topic) {
    mqtt_broker = String(broker);
    mqtt_port = port;
    mqtt_topic = String(topic);

    if (username) mqtt_username = String(username);
    if (password) mqtt_password = String(password);

    Serial.printf("[WIFI TELEMETRY] MQTT configured (%s:%d)\n", broker, port);
}

void configureInfluxDB(const char* url, const char* database,
                       const char* measurement,
                       const char* username, const char* password) {
    influxdb_url = String(url);
    influxdb_database = String(database);
    influxdb_measurement = String(measurement);

    if (username) influxdb_username = String(username);
    if (password) influxdb_password = String(password);

    Serial.printf("[WIFI TELEMETRY] InfluxDB configured (%s/%s)\n", url, database);
}

void configureHTTP(const char* url, const char* auth_header) {
    http_url = String(url);
    if (auth_header) http_auth_header = String(auth_header);

    Serial.printf("[WIFI TELEMETRY] HTTP POST configured (%s)\n", url);
}

// ============================================================================
// STATUS
// ============================================================================

uint32_t getTimeSinceLastUpdate() {
    return millis() - last_update_time;
}

uint32_t getUpdateCount() {
    return update_count;
}

uint32_t getErrorCount() {
    return error_count;
}

bool isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

int8_t getRSSI() {
    return WiFi.RSSI();
}

} // namespace WiFiTelemetry

#endif // ENABLE_WIFI
