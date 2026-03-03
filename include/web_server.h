// =============================================================================
// web_server.h - Async Web Server & WebSocket Interface
// =============================================================================
// Serves the control UI and handles real-time bidirectional communication
// via WebSocket for parameter changes and status updates.
// =============================================================================

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>

/**
 * Initialize Wi-Fi (AP or STA mode) and start the async web server.
 * Serves static files from LittleFS and sets up WebSocket endpoint.
 */
void web_server_init();

/**
 * Called from loop() to handle periodic tasks:
 *   - Push status updates to connected WebSocket clients
 *   - Clean up stale WebSocket connections
 */
void web_server_loop();

/**
 * Broadcast a JSON status update to all connected WebSocket clients.
 */
void web_server_push_status();

/**
 * Get the number of currently connected WebSocket clients.
 */
uint8_t web_server_client_count();

#endif // WEB_SERVER_H
