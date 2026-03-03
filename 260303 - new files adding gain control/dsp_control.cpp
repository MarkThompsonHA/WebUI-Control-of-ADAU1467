// =============================================================================
// dsp_control.cpp - High-Level DSP Parameter Control Implementation
// =============================================================================

#include "dsp_control.h"
#include "adau1467_spi.h"
#include "hardware_config.h"
#include <math.h>

// =============================================================================
// Address Lookup Tables
// =============================================================================

// Mute addresses: maps channel → parameter RAM address
static const uint16_t mute_address_table[DSP_CHANNEL_COUNT] = {
  MUTE_OUTL_ADDR,        // Output Left
  MUTE_OUTR_ADDR,        // Output Right
};

// Gain addresses: maps gain_id → parameter RAM address
static const uint16_t gain_address_table[DSP_GAIN_COUNT] = {
  GAIN_SPEAKERS_ADDR,    // PC → Loudspeakers
  GAIN_HEADPHONES_ADDR,  // PC → Headphones
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

// Gain control name strings
static const char* gain_names[] = {
  "Speakers",
  "Headphones",
};

// =============================================================================
// dBFS ↔ 8.24 Fixed-Point Conversion
// =============================================================================

/**
 * Convert dBFS to ADAU1467 8.24 fixed-point value.
 *   0 dBFS   → 1.0 linear → 0x00800000
 *  -6 dBFS   → 0.5 linear → 0x00400000
 * -40 dBFS   → 0.01 linear → 0x000028F5
 * -inf dBFS  → 0.0 linear → 0x00000000
 */
static uint32_t db_to_fixed824(float db) {
  if (db <= GAIN_MIN_DB) {
    return 0x00000000;  // Below minimum, full attenuation
  }
  if (db >= GAIN_MAX_DB) {
    return ADAU_FIXEDPOINT_UNITY;  // 0 dBFS = unity
  }

  // dBFS to linear: linear = 10^(dB/20)
  float linear = powf(10.0f, db / 20.0f);

  // Linear to 8.24 fixed-point: multiply by 2^24
  uint32_t fixed = (uint32_t)(linear * 16777216.0f);

  // Clamp to maximum (shouldn't exceed unity but safety check)
  if (fixed > ADAU_FIXEDPOINT_UNITY) {
    fixed = ADAU_FIXEDPOINT_UNITY;
  }

  return fixed;
}

/**
 * Convert 8.24 fixed-point to dBFS.
 */
static float fixed824_to_db(uint32_t fixed) {
  if (fixed == 0) {
    return GAIN_MIN_DB;  // Represent silence as minimum
  }

  float linear = (float)fixed / 16777216.0f;
  float db = 20.0f * log10f(linear);

  // Clamp to range
  if (db < GAIN_MIN_DB) db = GAIN_MIN_DB;
  if (db > GAIN_MAX_DB) db = GAIN_MAX_DB;

  return db;
}

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

  // Set initial gain state: all at default (0 dBFS)
  for (int i = 0; i < DSP_GAIN_COUNT; i++) {
    system_status.gains[i].gain_db = GAIN_DEFAULT_DB;
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

    // Sync gain states - write default gain to all controls
    for (int i = 0; i < DSP_GAIN_COUNT; i++) {
      dsp_set_gain((dsp_gain_t)i, GAIN_DEFAULT_DB);
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

  uint16_t address = mute_address_table[channel];
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
    system_status.last_error_code = 1;
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
// Gain Control
// =============================================================================

bool dsp_set_gain(dsp_gain_t gain_id, float gain_db) {
  if (gain_id >= DSP_GAIN_COUNT) {
    Serial.printf("[DSP] ERROR: Invalid gain ID %d\n", gain_id);
    return false;
  }

  // Clamp to valid range
  if (gain_db < GAIN_MIN_DB) gain_db = GAIN_MIN_DB;
  if (gain_db > GAIN_MAX_DB) gain_db = GAIN_MAX_DB;

  uint16_t address = gain_address_table[gain_id];
  uint32_t fixed_value = db_to_fixed824(gain_db);

  bool success = adau1467_write_param(address, fixed_value);

  if (success) {
    system_status.gains[gain_id].gain_db = gain_db;
    Serial.printf("[DSP] %s gain: %.1f dBFS (addr=0x%04X, val=0x%08X)\n",
                  gain_names[gain_id], gain_db, address, fixed_value);
  } else {
    Serial.printf("[DSP] ERROR: Failed to set gain on %s\n",
                  gain_names[gain_id]);
    system_status.last_error_code = 2;
  }

  return success;
}

float dsp_get_gain(dsp_gain_t gain_id) {
  if (gain_id >= DSP_GAIN_COUNT) return GAIN_MIN_DB;
  return system_status.gains[gain_id].gain_db;
}

const char* dsp_gain_name(dsp_gain_t gain_id) {
  if (gain_id >= DSP_GAIN_COUNT) return "Unknown";
  return gain_names[gain_id];
}

// =============================================================================
// Status & Diagnostics
// =============================================================================

void dsp_refresh_status() {
  system_status.dsp_alive = adau1467_is_alive();
  system_status.spi_transactions = adau1467_get_transaction_count();
  system_status.uptime_seconds = (millis() - boot_time) / 1000;

  if (system_status.dsp_alive) {
    system_status.dsp_running = adau1467_is_running();
    digitalWrite(LED_DSP_STATUS, HIGH);
  } else {
    system_status.dsp_running = false;
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
