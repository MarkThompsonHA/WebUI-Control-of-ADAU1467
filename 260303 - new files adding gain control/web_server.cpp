// =============================================================================
// web_server.cpp - Async Web Server & WebSocket Implementation
// =============================================================================

#include "web_server.h"
#include "dsp_control.h"
#include "adau1467_spi.h"
#include "hardware_config.h"

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// =============================================================================
// Server & WebSocket Objects
// =============================================================================

static AsyncWebServer server(WEB_SERVER_PORT);
static AsyncWebSocket ws("/ws");

// Timing
static unsigned long last_status_push = 0;

// =============================================================================
// WebSocket Message Handlers
// =============================================================================

static String build_status_json() {
  JsonDocument doc;

  dsp_system_status_t status = dsp_get_status();

  doc["type"] = "status";
  doc["dsp_alive"] = status.dsp_alive;
  doc["dsp_running"] = status.dsp_running;
  doc["spi_transactions"] = status.spi_transactions;
  doc["uptime"] = status.uptime_seconds;
  doc["wifi_clients"] = web_server_client_count();
  doc["error"] = status.last_error_code;

  // Channel states (mute)
  JsonArray channels = doc["channels"].to<JsonArray>();
  for (int i = 0; i < DSP_CHANNEL_COUNT; i++) {
    JsonObject ch = channels.add<JsonObject>();
    ch["id"] = i;
    ch["name"] = dsp_channel_name((dsp_channel_t)i);
    ch["muted"] = status.channels[i].muted;
  }

  // Gain control states
  JsonArray gains = doc["gains"].to<JsonArray>();
  for (int i = 0; i < DSP_GAIN_COUNT; i++) {
    JsonObject g = gains.add<JsonObject>();
    g["id"] = i;
    g["name"] = dsp_gain_name((dsp_gain_t)i);
    g["gain_db"] = serialized(String(status.gains[i].gain_db, 1));
  }

  String output;
  serializeJson(doc, output);
  return output;
}

static void handle_ws_message(AsyncWebSocketClient* client, const String& msg) {
  Serial.printf("[WS] Received from client %u: %s\n", client->id(), msg.c_str());

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, msg);

  if (err) {
    Serial.printf("[WS] JSON parse error: %s\n", err.c_str());
    client->text("{\"type\":\"error\",\"message\":\"Invalid JSON\"}");
    return;
  }

  const char* action = doc["action"];
  if (!action) {
    client->text("{\"type\":\"error\",\"message\":\"Missing action field\"}");
    return;
  }

  // ----- Action: Set Mute -----
  if (strcmp(action, "set_mute") == 0) {
    int channel = doc["channel"] | -1;
    bool muted = doc["muted"] | false;

    if (channel < 0 || channel >= DSP_CHANNEL_COUNT) {
      client->text("{\"type\":\"error\",\"message\":\"Invalid channel\"}");
      return;
    }

    bool success = dsp_set_mute((dsp_channel_t)channel, muted);
    if (success) {
      ws.textAll(build_status_json());
    } else {
      client->text("{\"type\":\"error\",\"message\":\"SPI write failed\"}");
    }
  }
  // ----- Action: Toggle Mute -----
  else if (strcmp(action, "toggle_mute") == 0) {
    int channel = doc["channel"] | -1;

    if (channel < 0 || channel >= DSP_CHANNEL_COUNT) {
      client->text("{\"type\":\"error\",\"message\":\"Invalid channel\"}");
      return;
    }

    bool success = dsp_toggle_mute((dsp_channel_t)channel);
    if (success) {
      ws.textAll(build_status_json());
    } else {
      client->text("{\"type\":\"error\",\"message\":\"SPI write failed\"}");
    }
  }
  // ----- Action: Set Gain -----
  else if (strcmp(action, "set_gain") == 0) {
    int gain_id = doc["gain_id"] | -1;
    float gain_db = doc["gain_db"] | -999.0f;

    if (gain_id < 0 || gain_id >= DSP_GAIN_COUNT) {
      client->text("{\"type\":\"error\",\"message\":\"Invalid gain ID\"}");
      return;
    }

    if (gain_db < -990.0f) {
      client->text("{\"type\":\"error\",\"message\":\"Missing gain_db value\"}");
      return;
    }

    bool success = dsp_set_gain((dsp_gain_t)gain_id, gain_db);
    if (success) {
      ws.textAll(build_status_json());
    } else {
      client->text("{\"type\":\"error\",\"message\":\"Gain SPI write failed\"}");
    }
  }
  // ----- Action: Get Status -----
  else if (strcmp(action, "get_status") == 0) {
    dsp_refresh_status();
    client->text(build_status_json());
  }
  // ----- Action: Reset DSP -----
  else if (strcmp(action, "reset_dsp") == 0) {
    adau1467_reset();
    dsp_control_init(false);
    dsp_refresh_status();
    ws.textAll(build_status_json());
  }
  // ----- Action: Write Raw Parameter -----
  else if (strcmp(action, "write_param") == 0) {
    uint16_t address = doc["address"] | 0;
    uint32_t value = doc["value"] | 0;

    bool success = adau1467_write_param(address, value);

    JsonDocument resp;
    resp["type"] = "write_result";
    resp["address"] = address;
    resp["value"] = value;
    resp["success"] = success;

    String output;
    serializeJson(resp, output);
    client->text(output);
  }
  // ----- Action: Read Raw Parameter -----
  else if (strcmp(action, "read_param") == 0) {
    uint16_t address = doc["address"] | 0;
    uint32_t value = 0;

    bool success = adau1467_read_param(address, &value);

    JsonDocument resp;
    resp["type"] = "read_result";
    resp["address"] = address;
    resp["value"] = value;
    resp["success"] = success;

    String output;
    serializeJson(resp, output);
    client->text(output);
  }
  // ----- Unknown Action -----
  else {
    String errMsg = "{\"type\":\"error\",\"message\":\"Unknown action: ";
    errMsg += action;
    errMsg += "\"}";
    client->text(errMsg);
  }
}

// =============================================================================
// WebSocket Event Handler
// =============================================================================

static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                       AwsEventType type, void* arg, uint8_t* data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("[WS] Client %u connected from %s\n",
                    client->id(), client->remoteIP().toString().c_str());
      dsp_refresh_status();
      client->text(build_status_json());
      break;

    case WS_EVT_DISCONNECT:
      Serial.printf("[WS] Client %u disconnected\n", client->id());
      break;

    case WS_EVT_DATA: {
      AwsFrameInfo* info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len) {
        if (info->opcode == WS_TEXT) {
          String msg((char*)data, len);
          handle_ws_message(client, msg);
        }
      }
      break;
    }

    case WS_EVT_PONG:
      break;

    case WS_EVT_ERROR:
      Serial.printf("[WS] Error from client %u: %u\n", client->id(), *((uint16_t*)arg));
      break;
  }
}

// =============================================================================
// Wi-Fi Initialization
// =============================================================================

static void init_wifi() {
  pinMode(LED_WIFI, OUTPUT);

#ifdef WIFI_STA_SSID
  Serial.printf("[WiFi] Connecting to %s...\n", WIFI_STA_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_WIFI, !digitalRead(LED_WIFI));
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    digitalWrite(LED_WIFI, HIGH);
  } else {
    Serial.println("\n[WiFi] STA connection failed, falling back to AP mode");
    WiFi.disconnect();
  }
#endif

  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf("[WiFi] Starting AP: %s\n", WIFI_AP_SSID);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL, false, WIFI_AP_MAX_CONN);

    IPAddress ip = WiFi.softAPIP();
    Serial.printf("[WiFi] AP started. Connect to '%s' and browse to http://%s\n",
                  WIFI_AP_SSID, ip.toString().c_str());
    digitalWrite(LED_WIFI, HIGH);
  }
}

// =============================================================================
// Web Server Setup
// =============================================================================

void web_server_init() {
  init_wifi();

  if (!LittleFS.begin(true)) {
    Serial.println("[FS] ERROR: LittleFS mount failed!");
    return;
  }
  Serial.println("[FS] LittleFS mounted successfully");

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    dsp_refresh_status();
    request->send(200, "application/json", build_status_json());
  });

  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.printf("[HTTP] Web server started on port %d\n", WEB_SERVER_PORT);
}

// =============================================================================
// Periodic Tasks
// =============================================================================

void web_server_loop() {
  ws.cleanupClients();

  if (millis() - last_status_push >= STATUS_REPORT_INTERVAL_MS) {
    last_status_push = millis();

    if (ws.count() > 0) {
      dsp_refresh_status();
      ws.textAll(build_status_json());
    }
  }
}

void web_server_push_status() {
  if (ws.count() > 0) {
    ws.textAll(build_status_json());
  }
}

uint8_t web_server_client_count() {
  return ws.count();
}
