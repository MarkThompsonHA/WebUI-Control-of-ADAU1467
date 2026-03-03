// =============================================================================
// main.cpp - ADAU1467 Web Control Firmware
// =============================================================================
// Entry point for the ESP32 firmware. Initializes all subsystems and runs
// the main loop handling web server tasks and local UI.
//
// System Architecture:
//   ESP32 (this firmware)
//     ├── Wi-Fi AP/STA → Web Browser (control UI)
//     ├── VSPI Bus → ADAU1467 Slave Control Port (DSP parameters)
//     ├── GPIOs → LEDs, buttons, pot (local UI)
//     └── Serial → Debug console (115200 baud)
//
// Boot Sequence:
//   1. Serial + GPIO initialization
//   2. SPI bus initialization
//   3. ADAU1467 reset (optional) and communication check
//   4. Wi-Fi + Web Server startup
//   5. Main loop: web server tasks + local UI polling
// =============================================================================

#include <Arduino.h>
#include "hardware_config.h"
#include "adau1467_spi.h"
#include "dsp_control.h"
#include "web_server.h"

// =============================================================================
// Local UI State
// =============================================================================

static unsigned long last_button_check = 0;
static bool last_button_state = HIGH;

// =============================================================================
// Setup
// =============================================================================

void setup() {
  // --- Serial Debug ---
  Serial.begin(115200);
  delay(500);  // Allow serial to stabilize
  Serial.println();
  Serial.println("============================================");
  Serial.println("  ADAU1467 Web Control - Test Framework");
  Serial.println("  ESP32 DoIt DevKit V1");
  Serial.println("============================================");

  // --- Local UI pins ---
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_SPI_ACTIVITY, OUTPUT);
  pinMode(LED_DSP_STATUS, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(TOGGLE_PIN, INPUT_PULLUP);

  // Flash all LEDs at startup
  digitalWrite(LED_WIFI, HIGH);
  digitalWrite(LED_SPI_ACTIVITY, HIGH);
  digitalWrite(LED_DSP_STATUS, HIGH);
  delay(300);
  digitalWrite(LED_WIFI, LOW);
  digitalWrite(LED_SPI_ACTIVITY, LOW);
  digitalWrite(LED_DSP_STATUS, LOW);

  // --- DSP Control Layer ---
  // Set to 'true' to reset ADAU1467 at boot.
  // Set to 'false' if ADAU1467 is already self-booted and you don't want
  // to interrupt its operation.
  bool reset_at_boot = false;
  dsp_control_init(reset_at_boot);

  // --- Web Server ---
  web_server_init();

  // --- Ready ---
  Serial.println();
  Serial.println("============================================");
  Serial.println("  System ready!");
  Serial.printf("  Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.println("============================================");
  Serial.println();
}

// =============================================================================
// Main Loop
// =============================================================================

void loop() {
  // --- Web Server Tasks ---
  web_server_loop();

  // --- Local Button Handling ---
  // Momentary button on GPIO26 toggles mute on DIN32
  if (millis() - last_button_check > 50) {  // 50ms debounce
    last_button_check = millis();
    bool button_state = digitalRead(BUTTON_PIN);

    if (button_state == LOW && last_button_state == HIGH) {
      // Button pressed (falling edge) - toggle Output Left mute
      Serial.println("[UI] Button press -> Toggle Output Left mute");
      dsp_toggle_mute(DSP_CHANNEL_OUTL);
      web_server_push_status();  // Notify connected web clients
    }

    last_button_state = button_state;
  }

  // Small yield to keep watchdog happy
  yield();
}
