/**
 * @file main.cpp
 * @brief Unified UI Controller Firmware - ALL non-critical features in ONE codebase
 *
 * This replaces ALL separate display/UI modules with a single unified firmware:
 * - AC_Display_Module (factory AC display)
 * - Aftermarket_Display_Module (OLED gauges)
 * - Coolant_Monitor_Module (coolant temp/pressure)
 * - Wipers_Module (speed-sensitive wipers)
 * - ESP8266_Companion (WiFi/Bluetooth)
 *
 * Configure via config/features.h
 *
 * Runs on ESP32 with FreeRTOS multitasking
 *
 * @author Unified Architecture (Phase 5)
 * @date 2025-11-16
 */

#include <Arduino.h>
#include "config/features.h"

// FreeRTOS
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// Display libraries
#if ENABLE_AC_DISPLAY
    #include "displays/ac_display.h"
#endif

#if ENABLE_OLED_GAUGES
    #include "displays/oled_gauges.h"
#endif

#if ENABLE_COOLANT_MONITOR
    #include "displays/coolant_monitor.h"
#endif

// Control modules
#if ENABLE_WIPERS
    #include "controls/wipers.h"
#endif

#if ENABLE_BUTTONS
    #include "controls/buttons.h"
#endif

// Connectivity
#if ENABLE_WIFI
    #include <WiFi.h>
    #include "connectivity/wifi_telemetry.h"
#endif

#if ENABLE_BLUETOOTH
    #include "connectivity/bluetooth.h"
#endif

#if ENABLE_OTA_UPDATES
    #include "connectivity/ota_update.h"
#endif

#if ENABLE_WEB_DASHBOARD
    #include "connectivity/web_server.h"
#endif

// Communication with automotive ECU
#if USE_UART_BRIDGE
    #include "communication/uart_bridge.h"
#elif USE_CAN_BRIDGE
    #include "communication/can_listener.h"
#endif

// Shared vehicle state structure
#include "../shared/vehicle_state.h"

// ============================================================================
// GLOBAL STATE
// ============================================================================
VehicleState g_vehicle_state = {0};
SemaphoreHandle_t g_state_mutex;  // Protect vehicle state from concurrent access

// Task handles
TaskHandle_t g_display_task_handle = NULL;
TaskHandle_t g_network_task_handle = NULL;
TaskHandle_t g_can_task_handle = NULL;
TaskHandle_t g_wiper_task_handle = NULL;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================
void printBanner();
void printConfig();

// FreeRTOS tasks
void displayTask(void* param);
void networkTask(void* param);
void canTask(void* param);
void wiperTask(void* param);

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    // Initialize serial console
    #if ENABLE_SERIAL_DEBUG
        Serial.begin(SERIAL_BAUDRATE);
        delay(1000);
        printBanner();
        printConfig();
    #endif

    // Create mutex for vehicle state protection
    g_state_mutex = xSemaphoreCreateMutex();

    // Initialize WiFi (if enabled)
    #if ENABLE_WIFI
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        WiFi.setHostname(WIFI_HOSTNAME);

        #if ENABLE_SERIAL_DEBUG
            Serial.print("[WIFI] Connecting to ");
            Serial.print(WIFI_SSID);
        #endif

        int retries = 0;
        while (WiFi.status() != WL_CONNECTED && retries < 20) {
            delay(500);
            #if ENABLE_SERIAL_DEBUG
                Serial.print(".");
            #endif
            retries++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            #if ENABLE_SERIAL_DEBUG
                Serial.println(" Connected!");
                Serial.print("[WIFI] IP: ");
                Serial.println(WiFi.localIP());
            #endif
        } else {
            #if ENABLE_SERIAL_DEBUG
                Serial.println(" Failed!");
            #endif
        }
    #endif

    // Initialize Bluetooth (if enabled)
    #if ENABLE_BLUETOOTH
        Bluetooth::init("RX8-ECU");
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[BT] Bluetooth initialized");
        #endif
    #endif

    // Initialize OTA updates (if enabled)
    #if ENABLE_OTA_UPDATES
        OTAUpdate::init();
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[OTA] OTA updates enabled");
        #endif
    #endif

    // Initialize web dashboard (if enabled)
    #if ENABLE_WEB_DASHBOARD
        WebServer::init();
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[WEB] Web dashboard started on port 80");
        #endif
    #endif

    // Initialize displays
    #if ENABLE_AC_DISPLAY
        ACDisplay::init();
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[DISPLAY] AC Display initialized");
        #endif
    #endif

    #if ENABLE_OLED_GAUGES
        OLEDGauges::init();
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[DISPLAY] OLED Gauges initialized");
        #endif
    #endif

    #if ENABLE_COOLANT_MONITOR
        CoolantMonitor::init();
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[DISPLAY] Coolant Monitor initialized");
        #endif
    #endif

    // Initialize controls
    #if ENABLE_WIPERS
        Wipers::init();
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[WIPER] Speed-sensitive wipers initialized");
        #endif
    #endif

    #if ENABLE_BUTTONS
        Buttons::init();
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[INPUT] Button input initialized");
        #endif
    #endif

    // Initialize communication with automotive ECU
    #if USE_UART_BRIDGE
        UARTBridge::init(UART_BAUDRATE, UART_RX_PIN, UART_TX_PIN);
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[UART] Bridge to automotive ECU initialized");
        #endif
    #elif USE_CAN_BRIDGE
        CANListener::init(CAN_RX_PIN, CAN_TX_PIN, CAN_SPEED);
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[CAN] CAN listener initialized @ 500 kbps");
        #endif
    #endif

    // Create FreeRTOS tasks
    #if ENABLE_AC_DISPLAY || ENABLE_OLED_GAUGES || ENABLE_COOLANT_MONITOR
        xTaskCreatePinnedToCore(
            displayTask,                // Task function
            "DisplayTask",              // Task name
            TASK_DISPLAY_STACK,         // Stack size
            NULL,                       // Parameters
            TASK_DISPLAY_PRIORITY,      // Priority
            &g_display_task_handle,     // Task handle
            0                           // Core (0 or 1)
        );
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[TASK] Display task created on core 0");
        #endif
    #endif

    #if ENABLE_WIFI || ENABLE_BLUETOOTH || ENABLE_WEB_DASHBOARD
        xTaskCreatePinnedToCore(
            networkTask,
            "NetworkTask",
            TASK_NETWORK_STACK,
            NULL,
            TASK_NETWORK_PRIORITY,
            &g_network_task_handle,
            1                           // Core 1 (separate from display)
        );
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[TASK] Network task created on core 1");
        #endif
    #endif

    xTaskCreatePinnedToCore(
        canTask,
        "CANTask",
        TASK_CAN_STACK,
        NULL,
        TASK_CAN_PRIORITY,             // Higher priority for CAN
        &g_can_task_handle,
        1                               // Core 1
    );
    #if ENABLE_SERIAL_DEBUG
        Serial.println("[TASK] CAN/UART listener task created");
    #endif

    #if ENABLE_WIPERS
        xTaskCreatePinnedToCore(
            wiperTask,
            "WiperTask",
            TASK_WIPER_STACK,
            NULL,
            TASK_WIPER_PRIORITY,
            &g_wiper_task_handle,
            0                           // Core 0
        );
        #if ENABLE_SERIAL_DEBUG
            Serial.println("[TASK] Wiper control task created");
        #endif
    #endif

    #if ENABLE_SERIAL_DEBUG
        Serial.println("\n=== UI CONTROLLER READY ===\n");
        Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
        Serial.printf("Features enabled: %d\n", FEATURE_COUNT);
        Serial.println();
    #endif
}

// ============================================================================
// MAIN LOOP (minimal - FreeRTOS tasks do the work)
// ============================================================================
void loop() {
    // OTA updates check (if enabled)
    #if ENABLE_OTA_UPDATES
        OTAUpdate::handle();
    #endif

    // Let FreeRTOS scheduler run
    vTaskDelay(pdMS_TO_TICKS(100));
}

// ============================================================================
// DISPLAY TASK - Updates all displays
// ============================================================================
void displayTask(void* param) {
    TickType_t last_wake_time = xTaskGetTickCount();

    while (true) {
        // Acquire vehicle state mutex
        if (xSemaphoreTake(g_state_mutex, portMAX_DELAY)) {
            VehicleState state_copy = g_vehicle_state;  // Make local copy
            xSemaphoreGive(g_state_mutex);

            // Update AC display
            #if ENABLE_AC_DISPLAY
                ACDisplay::update(state_copy);
            #endif

            // Update OLED gauges
            #if ENABLE_OLED_GAUGES
                OLEDGauges::update(state_copy);
            #endif

            // Update coolant monitor
            #if ENABLE_COOLANT_MONITOR
                CoolantMonitor::update(state_copy);
            #endif
        }

        // Run at fixed rate (100ms = 10 Hz)
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(100));
    }
}

// ============================================================================
// NETWORK TASK - Handles WiFi/Bluetooth/Web dashboard
// ============================================================================
void networkTask(void* param) {
    TickType_t last_wake_time = xTaskGetTickCount();

    while (true) {
        // WiFi telemetry
        #if ENABLE_WIFI
            if (WiFi.status() == WL_CONNECTED) {
                // Acquire vehicle state
                if (xSemaphoreTake(g_state_mutex, portMAX_DELAY)) {
                    VehicleState state_copy = g_vehicle_state;
                    xSemaphoreGive(g_state_mutex);

                    // Send telemetry
                    WiFiTelemetry::sendUpdate(state_copy);
                }
            }
        #endif

        // Bluetooth updates
        #if ENABLE_BLUETOOTH
            if (xSemaphoreTake(g_state_mutex, portMAX_DELAY)) {
                VehicleState state_copy = g_vehicle_state;
                xSemaphoreGive(g_state_mutex);

                Bluetooth::sendUpdate(state_copy);
            }
        #endif

        // Web dashboard updates
        #if ENABLE_WEB_DASHBOARD
            WebServer::handle();  // Process HTTP requests
        #endif

        // Run at fixed rate (100ms)
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(WEB_UPDATE_RATE_MS));
    }
}

// ============================================================================
// CAN/UART TASK - Receives vehicle data from automotive ECU
// ============================================================================
void canTask(void* param) {
    TickType_t last_wake_time = xTaskGetTickCount();

    while (true) {
        VehicleState new_state;

        // Read vehicle state from automotive ECU
        #if USE_UART_BRIDGE
            if (UARTBridge::receiveVehicleState(&new_state)) {
                // Update global state (thread-safe)
                if (xSemaphoreTake(g_state_mutex, portMAX_DELAY)) {
                    g_vehicle_state = new_state;
                    xSemaphoreGive(g_state_mutex);
                }
            }
        #elif USE_CAN_BRIDGE
            if (CANListener::receiveVehicleState(&new_state)) {
                if (xSemaphoreTake(g_state_mutex, portMAX_DELAY)) {
                    g_vehicle_state = new_state;
                    xSemaphoreGive(g_state_mutex);
                }
            }
        #endif

        // Run at high frequency (20ms = 50 Hz)
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(20));
    }
}

// ============================================================================
// WIPER TASK - Controls speed-sensitive wipers
// ============================================================================
void wiperTask(void* param) {
    TickType_t last_wake_time = xTaskGetTickCount();

    while (true) {
        #if ENABLE_WIPERS
            // Get current vehicle speed
            uint16_t speed_kmh = 0;
            if (xSemaphoreTake(g_state_mutex, portMAX_DELAY)) {
                speed_kmh = g_vehicle_state.speed_kmh / 10;
                xSemaphoreGive(g_state_mutex);
            }

            // Update wiper timing based on speed
            Wipers::update(speed_kmh);
        #endif

        // Run at moderate frequency (100ms)
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(100));
    }
}

// ============================================================================
// PRINT BANNER
// ============================================================================
void printBanner() {
    Serial.println("\n");
    Serial.println("╔══════════════════════════════════════════════════════════════╗");
    Serial.println("║     RX8 UNIFIED UI CONTROLLER - Phase 5 Architecture        ║");
    Serial.println("║     ALL non-critical features in ONE ESP32 firmware          ║");
    Serial.println("╚══════════════════════════════════════════════════════════════╝");
    Serial.println();
}

// ============================================================================
// PRINT CONFIGURATION
// ============================================================================
void printConfig() {
    Serial.println("Configuration:");
    Serial.printf("  WiFi SSID:     %s\n", WIFI_SSID);
    Serial.printf("  Communication: %s\n", USE_UART_BRIDGE ? "UART" : "CAN");
    Serial.println();

    Serial.println("Enabled Features:");
    Serial.printf("  [%c] AC Display\n", ENABLE_AC_DISPLAY ? 'X' : ' ');
    Serial.printf("  [%c] OLED Gauges\n", ENABLE_OLED_GAUGES ? 'X' : ' ');
    Serial.printf("  [%c] Coolant Monitor\n", ENABLE_COOLANT_MONITOR ? 'X' : ' ');
    Serial.printf("  [%c] Wipers\n", ENABLE_WIPERS ? 'X' : ' ');
    Serial.printf("  [%c] WiFi Telemetry\n", ENABLE_WIFI ? 'X' : ' ');
    Serial.printf("  [%c] Bluetooth\n", ENABLE_BLUETOOTH ? 'X' : ' ');
    Serial.printf("  [%c] OTA Updates\n", ENABLE_OTA_UPDATES ? 'X' : ' ');
    Serial.printf("  [%c] Web Dashboard\n", ENABLE_WEB_DASHBOARD ? 'X' : ' ');
    Serial.println();

    Serial.printf("Total features enabled: %d\n", FEATURE_COUNT);
    Serial.println();
}
