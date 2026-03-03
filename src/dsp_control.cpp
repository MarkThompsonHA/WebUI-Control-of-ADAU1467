// =============================================================================
// dsp_control.cpp - High-Level DSP Parameter Control Implementation
// =============================================================================

#include "dsp_control.h"
#include "adau1467_spi.h"
#include "hardware_config.h"

// =============================================================================
// Address Lookup Table
// =============================================================================
// Maps (channel, block_type) → ADAU1467 parameter RAM address
// Update these addresses after exporting from SigmaStudio!

static const uint16_t param_address_table[DSP_CHANNEL_COUNT][DSP_BLOCK_TYPE_COUNT] = {
  // [channel][block_type]
  // Columns: MUTE           (add more columns for VOLUME, EQ, etc.)
  { MUTE_OUTL_ADDR },       // DIN32
  { MUTE_OUTR_ADDR },       // DIN33
};

// =============================================================================
// Internal State
// =============================================================================

static dsp_system_status_t system_status = {};
static unsigned long boot_time = 0;

// Channel name strings
static const char* channel_names[] = {
  "Output Left",
  "Output Right",
};

// =============================================================================
// Initialization
// =============================================================================

void dsp_control_init(bool reset_dsp) {
  Serial.println("[DSP] Initializing DSP control layer...");

  // Initialize the SPI driver
  adau1467_spi_init();

  // Reset DSP if requested
  if (reset_dsp) {
    adau1467_reset();
  }

  // Set initial state: all channels unmuted
  for (int i = 0; i < DSP_CHANNEL_COUNT; i++) {
    system_status.channels[i].muted = false;
  }

  // Record boot time
  boot_time = millis();

  // Check if DSP is communicating
  system_status.dsp_alive = adau1467_is_alive();
  if (system_status.dsp_alive) {
    Serial.println("[DSP] ADAU1467 is responding on SPI");

    // Sync mute states - write UNMUTED to all channels
    for (int i = 0; i < DSP_CHANNEL_COUNT; i++) {
      dsp_set_mute((dsp_channel_t)i, false);
    }

    system_status.dsp_running = adau1467_is_running();
    Serial.printf("[DSP] DSP core is %s\n",
                  system_status.dsp_running ? "RUNNING" : "STOPPED");
  } else {
    Serial.println("[DSP] WARNING: ADAU1467 not responding on SPI!");
    Serial.println("[DSP]   Check: SPI wiring, SELFBOOT enabled, power supply");
  }

  // Configure DSP status LED
  pinMode(LED_DSP_STATUS, OUTPUT);
  digitalWrite(LED_DSP_STATUS, system_status.dsp_alive ? HIGH : LOW);

  Serial.println("[DSP] Initialization complete");
}

// =============================================================================
// Mute Control
// =============================================================================

bool dsp_set_mute(dsp_channel_t channel, bool muted) {
  if (channel >= DSP_CHANNEL_COUNT) {
    Serial.printf("[DSP] ERROR: Invalid channel %d\n", channel);
    return false;
  }

  // Get the mute parameter address for this channel
  uint16_t address = param_address_table[channel][DSP_BLOCK_MUTE];

  // SigmaStudio mute block:
  //   0x00000000 = muted (gain = 0.0)
  //   0x00800000 = unmuted (gain = 1.0 in 8.24 fixed-point)
  uint32_t value = muted ? ADAU_FIXEDPOINT_ZERO : ADAU_FIXEDPOINT_UNITY;

  bool success = adau1467_write_param(address, value);

  if (success) {
    system_status.channels[channel].muted = muted;
    Serial.printf("[DSP] %s: %s (addr=0x%04X, val=0x%08X)\n",
                  channel_names[channel],
                  muted ? "MUTED" : "UNMUTED",
                  address, value);
  } else {
    Serial.printf("[DSP] ERROR: Failed to set mute on %s\n",
                  channel_names[channel]);
    system_status.last_error_code = 1;  // SPI write failure
  }

  return success;
}

bool dsp_get_mute(dsp_channel_t channel) {
  if (channel >= DSP_CHANNEL_COUNT) return false;
  return system_status.channels[channel].muted;
}

bool dsp_toggle_mute(dsp_channel_t channel) {
  if (channel >= DSP_CHANNEL_COUNT) return false;
  return dsp_set_mute(channel, !system_status.channels[channel].muted);
}

// =============================================================================
// Status & Diagnostics
// =============================================================================

void dsp_refresh_status() {
  system_status.dsp_alive = adau1467_is_alive();
  system_status.spi_transactions = adau1467_get_transaction_count();
  system_status.uptime_seconds = (millis() - boot_time) / 1000;

  // Only check running state if DSP is communicating
  if (system_status.dsp_alive) {
    system_status.dsp_running = adau1467_is_running();
    digitalWrite(LED_DSP_STATUS, HIGH);
  } else {
    system_status.dsp_running = false;
    // Blink LED to indicate communication failure
    digitalWrite(LED_DSP_STATUS,
                 (millis() / LED_BLINK_INTERVAL_MS) % 2 ? HIGH : LOW);
  }
}

dsp_system_status_t dsp_get_status() {
  return system_status;
}

const char* dsp_channel_name(dsp_channel_t channel) {
  if (channel >= DSP_CHANNEL_COUNT) return "Unknown";
  return channel_names[channel];
}