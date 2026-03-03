// =============================================================================
// dsp_control.h - High-Level DSP Parameter Control
// =============================================================================
// Abstraction layer for controlling DSP blocks (mute, volume, gain, etc.)
// =============================================================================

#ifndef DSP_CONTROL_H
#define DSP_CONTROL_H

#include <Arduino.h>

// =============================================================================
// Channel Identifiers
// =============================================================================

typedef enum {
  DSP_CHANNEL_OUT_L = 0,     // Output Left (mute)
  DSP_CHANNEL_OUT_R = 1,     // Output Right (mute)
  DSP_CHANNEL_COUNT
} dsp_channel_t;

// =============================================================================
// Gain Control Identifiers
// =============================================================================

typedef enum {
  DSP_GAIN_SPEAKERS = 0,     // PC → Loudspeaker stereo gain
  DSP_GAIN_HEADPHONES = 1,   // PC → Headphone stereo gain
  DSP_GAIN_COUNT
} dsp_gain_t;

// =============================================================================
// Parameter Block Types
// =============================================================================

typedef enum {
  DSP_BLOCK_MUTE = 0,
  DSP_BLOCK_TYPE_COUNT
} dsp_block_type_t;

// =============================================================================
// Channel State Structure
// =============================================================================

typedef struct {
  bool     muted;
} dsp_channel_state_t;

// =============================================================================
// Gain State Structure
// =============================================================================

typedef struct {
  float    gain_db;           // Current gain in dBFS (-40.0 to 0.0)
} dsp_gain_state_t;

// =============================================================================
// System Status
// =============================================================================

typedef struct {
  bool     dsp_alive;
  bool     dsp_running;
  uint32_t spi_transactions;
  uint32_t uptime_seconds;
  uint32_t last_error_code;
  dsp_channel_state_t channels[DSP_CHANNEL_COUNT];
  dsp_gain_state_t    gains[DSP_GAIN_COUNT];
} dsp_system_status_t;

// =============================================================================
// Initialization
// =============================================================================

void dsp_control_init(bool reset_dsp);

// =============================================================================
// Mute Control
// =============================================================================

bool dsp_set_mute(dsp_channel_t channel, bool muted);
bool dsp_get_mute(dsp_channel_t channel);
bool dsp_toggle_mute(dsp_channel_t channel);

// =============================================================================
// Gain Control
// =============================================================================

/**
 * Set gain for a stereo gain control in dBFS.
 * Range: GAIN_MIN_DB (-40) to GAIN_MAX_DB (0)
 *
 * @param gain_id   Which gain control (speakers or headphones)
 * @param gain_db   Gain in dBFS (0.0 = unity, -40.0 = minimum)
 * @return          true on successful SPI write
 */
bool dsp_set_gain(dsp_gain_t gain_id, float gain_db);

/**
 * Get the current gain value in dBFS (from cached state).
 */
float dsp_get_gain(dsp_gain_t gain_id);

/**
 * Get a human-readable name for a gain control.
 */
const char* dsp_gain_name(dsp_gain_t gain_id);

// =============================================================================
// Status & Diagnostics
// =============================================================================

void dsp_refresh_status();
dsp_system_status_t dsp_get_status();
const char* dsp_channel_name(dsp_channel_t channel);

#endif // DSP_CONTROL_H
