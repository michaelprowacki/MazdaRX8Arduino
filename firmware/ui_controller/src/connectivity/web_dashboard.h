/**
 * @file web_dashboard.h
 * @brief Web dashboard for real-time vehicle monitoring
 *
 * Serves a web page on port 80 showing:
 * - Real-time gauges (RPM, speed, temperature)
 * - Warning lights status
 * - Historical graphs
 * - Configuration interface
 * - OTA update interface
 *
 * Access via: http://rx8-ecu.local or http://[ESP32_IP]
 *
 * @author Created for Phase 5 unified architecture
 * @date 2025-11-16
 */

#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include <stdint.h>
#include "../communication/uart_bridge.h"

namespace WebDashboard {

/**
 * @brief Initialize web dashboard
 *
 * Starts web server on port 80
 */
void init();

/**
 * @brief Handle web server requests (call from loop or FreeRTOS task)
 */
void handle();

/**
 * @brief Update vehicle state for dashboard
 *
 * @param state Current vehicle state
 */
void updateState(const UARTBridge::VehicleState& state);

/**
 * @brief Get number of active clients
 * @return Number of connected web clients
 */
uint8_t getClientCount();

/**
 * @brief Get total page views
 * @return Total number of page requests
 */
uint32_t getPageViews();

/**
 * @brief Enable/disable dashboard
 * @param enabled true to enable dashboard
 */
void setEnabled(bool enabled);

/**
 * @brief Set dashboard password (optional)
 *
 * If set, users must enter password to access dashboard
 *
 * @param password Password string (nullptr to disable)
 */
void setPassword(const char* password);

/**
 * @brief Enable WebSocket for real-time updates
 *
 * WebSocket provides faster updates than HTTP polling
 *
 * @param enabled true to enable WebSocket
 */
void setWebSocketEnabled(bool enabled);

} // namespace WebDashboard

#endif // WEB_DASHBOARD_H
