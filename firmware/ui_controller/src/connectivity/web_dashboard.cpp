/**
 * @file web_dashboard.cpp
 * @brief Web dashboard implementation
 *
 * @author Created for Phase 5 unified architecture
 * @date 2025-11-16
 */

#include "web_dashboard.h"
#include "../../config/features.h"
#include <Arduino.h>

#if ENABLE_WEB_DASHBOARD

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

namespace WebDashboard {

// Web server
static WebServer* server = nullptr;
static UARTBridge::VehicleState current_state = {0};
static uint32_t page_views = 0;
static String dashboard_password = "";
static bool websocket_enabled = false;

// Forward declarations
static void handleRoot();
static void handleAPI();
static void handleNotFound();
static String generateHTML();
static String generateJSON();

// ============================================================================
// INITIALIZATION
// ============================================================================

void init() {
    if (!server) {
        server = new WebServer(80);
    }

    // Configure routes
    server->on("/", handleRoot);
    server->on("/api/data", handleAPI);
    server->onNotFound(handleNotFound);

    // Start server
    server->begin();

    // Start mDNS responder
    if (MDNS.begin("rx8-ecu")) {
        MDNS.addService("http", "tcp", 80);
        Serial.println("[WEB] mDNS responder started: http://rx8-ecu.local");
    }

    Serial.printf("[WEB] Web dashboard started on port 80\n");
    Serial.printf("[WEB] Access at: http://%s\n", WiFi.localIP().toString().c_str());
}

// ============================================================================
// HANDLER
// ============================================================================

void handle() {
    if (server) {
        server->handleClient();
    }
}

void updateState(const UARTBridge::VehicleState& state) {
    current_state = state;
}

// ============================================================================
// HTTP HANDLERS
// ============================================================================

static void handleRoot() {
    page_views++;

    // Check password if set
    if (dashboard_password.length() > 0) {
        if (!server->authenticate("admin", dashboard_password.c_str())) {
            return server->requestAuthentication();
        }
    }

    String html = generateHTML();
    server->send(200, "text/html", html);
}

static void handleAPI() {
    String json = generateJSON();
    server->send(200, "application/json", json);
}

static void handleNotFound() {
    server->send(404, "text/plain", "404: Not Found");
}

// ============================================================================
// HTML GENERATION
// ============================================================================

static String generateHTML() {
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>RX8 ECU Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 20px;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        h1 {
            text-align: center;
            margin-bottom: 30px;
            font-size: 2.5em;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }
        .gauges {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        .gauge {
            background: rgba(255, 255, 255, 0.1);
            backdrop-filter: blur(10px);
            border-radius: 15px;
            padding: 20px;
            text-align: center;
            box-shadow: 0 8px 32px 0 rgba(31, 38, 135, 0.37);
        }
        .gauge-label {
            font-size: 0.9em;
            opacity: 0.8;
            margin-bottom: 10px;
        }
        .gauge-value {
            font-size: 3em;
            font-weight: bold;
            margin: 10px 0;
        }
        .gauge-unit {
            font-size: 0.8em;
            opacity: 0.7;
        }
        .warnings {
            background: rgba(255, 255, 255, 0.1);
            backdrop-filter: blur(10px);
            border-radius: 15px;
            padding: 20px;
            margin-bottom: 30px;
        }
        .warning-light {
            display: inline-block;
            padding: 8px 16px;
            margin: 5px;
            border-radius: 20px;
            font-size: 0.9em;
        }
        .warning-off {
            background: rgba(0, 255, 0, 0.2);
        }
        .warning-on {
            background: rgba(255, 0, 0, 0.6);
            animation: blink 1s infinite;
        }
        @keyframes blink {
            0%, 50% { opacity: 1; }
            51%, 100% { opacity: 0.5; }
        }
        .footer {
            text-align: center;
            opacity: 0.7;
            font-size: 0.9em;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🏎️ RX8 ECU Dashboard</h1>

        <div class="gauges">
            <div class="gauge">
                <div class="gauge-label">ENGINE RPM</div>
                <div class="gauge-value" id="rpm">0</div>
                <div class="gauge-unit">rpm</div>
            </div>

            <div class="gauge">
                <div class="gauge-label">SPEED</div>
                <div class="gauge-value" id="speed">0</div>
                <div class="gauge-unit">km/h</div>
            </div>

            <div class="gauge">
                <div class="gauge-label">COOLANT TEMP</div>
                <div class="gauge-value" id="coolant">0</div>
                <div class="gauge-unit">°C</div>
            </div>

            <div class="gauge">
                <div class="gauge-label">BATTERY</div>
                <div class="gauge-value" id="battery">0.0</div>
                <div class="gauge-unit">V</div>
            </div>

            <div class="gauge">
                <div class="gauge-label">THROTTLE</div>
                <div class="gauge-value" id="throttle">0</div>
                <div class="gauge-unit">%</div>
            </div>

            <div class="gauge">
                <div class="gauge-label">OIL PRESSURE</div>
                <div class="gauge-value" id="oil">0</div>
                <div class="gauge-unit">PSI</div>
            </div>
        </div>

        <div class="warnings">
            <h2 style="margin-bottom: 15px;">Warning Lights</h2>
            <span class="warning-light warning-off" id="check-engine">CHECK ENGINE</span>
            <span class="warning-light warning-off" id="abs">ABS</span>
            <span class="warning-light warning-off" id="oil-pressure">OIL PRESSURE</span>
            <span class="warning-light warning-off" id="coolant-level">COOLANT</span>
            <span class="warning-light warning-off" id="battery-charge">BATTERY</span>
        </div>

        <div class="footer">
            <p>Mazda RX8 Unified ECU - Phase 5</p>
            <p>Updates every second • <span id="uptime">0</span>s uptime</p>
        </div>
    </div>

    <script>
        function updateDashboard() {
            fetch('/api/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('rpm').textContent = data.rpm;
                    document.getElementById('speed').textContent = data.speed.toFixed(1);
                    document.getElementById('coolant').textContent = data.coolant_temp.toFixed(1);
                    document.getElementById('battery').textContent = data.battery_voltage.toFixed(2);
                    document.getElementById('throttle').textContent = data.throttle;
                    document.getElementById('oil').textContent = data.oil_pressure;
                    document.getElementById('uptime').textContent = Math.floor(data.uptime / 1000);

                    // Update warnings
                    document.getElementById('check-engine').className =
                        'warning-light ' + (data.warnings.check_engine ? 'warning-on' : 'warning-off');
                    document.getElementById('abs').className =
                        'warning-light ' + (data.warnings.abs ? 'warning-on' : 'warning-off');
                    document.getElementById('oil-pressure').className =
                        'warning-light ' + (data.warnings.oil_pressure ? 'warning-on' : 'warning-off');
                    document.getElementById('coolant-level').className =
                        'warning-light ' + (data.warnings.coolant_level ? 'warning-on' : 'warning-off');
                    document.getElementById('battery-charge').className =
                        'warning-light ' + (data.warnings.battery_charge ? 'warning-on' : 'warning-off');
                })
                .catch(err => console.error('Update failed:', err));
        }

        // Update every second
        setInterval(updateDashboard, 1000);
        updateDashboard();
    </script>
</body>
</html>
)";

    return html;
}

static String generateJSON() {
    String json = "{";
    json += "\"rpm\":" + String(current_state.rpm) + ",";
    json += "\"speed\":" + String(current_state.speed_kmh / 10.0f) + ",";
    json += "\"throttle\":" + String(current_state.throttle_percent) + ",";
    json += "\"coolant_temp\":" + String(current_state.coolant_temp / 10.0f) + ",";
    json += "\"battery_voltage\":" + String(current_state.battery_voltage / 100.0f) + ",";
    json += "\"oil_pressure\":" + String(current_state.oil_pressure) + ",";
    json += "\"boost_pressure\":" + String(current_state.boost_pressure) + ",";
    json += "\"uptime\":" + String(millis()) + ",";

    json += "\"warnings\":{";
    json += "\"check_engine\":" + String(current_state.warnings.check_engine ? "true" : "false") + ",";
    json += "\"abs\":" + String(current_state.warnings.abs_warning ? "true" : "false") + ",";
    json += "\"oil_pressure\":" + String(current_state.warnings.oil_pressure ? "true" : "false") + ",";
    json += "\"coolant_level\":" + String(current_state.warnings.coolant_level ? "true" : "false") + ",";
    json += "\"battery_charge\":" + String(current_state.warnings.battery_charge ? "true" : "false");
    json += "}";

    json += "}";

    return json;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

uint8_t getClientCount() {
    return 0;  // WebServer doesn't track this easily
}

uint32_t getPageViews() {
    return page_views;
}

void setEnabled(bool enabled) {
    // Not implemented - server always runs once started
}

void setPassword(const char* password) {
    if (password) {
        dashboard_password = String(password);
        Serial.printf("[WEB] Dashboard password protection enabled\n");
    } else {
        dashboard_password = "";
        Serial.printf("[WEB] Dashboard password protection disabled\n");
    }
}

void setWebSocketEnabled(bool enabled) {
    websocket_enabled = enabled;
}

} // namespace WebDashboard

#endif // ENABLE_WEB_DASHBOARD
