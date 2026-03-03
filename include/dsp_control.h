// =============================================================================
// dsp_control.h - High-Level DSP Parameter Control
// =============================================================================
// Abstraction layer for controlling DSP blocks (mute, volume, etc.)
// This is the module you'll expand as you add more SigmaStudio blocks.
// =============================================================================

#ifndef DSP_CONTROL_H
#define DSP_CONTROL_H

#include <Arduino.h>

// =============================================================================
// Channel Identifiers
// =============================================================================

// Extensible channel enum - add more as your signal path grows
typedef enum {
  DSP_CHANNEL_OUTL = 0,   // Digital Input 32
  DSP_CHANNEL_OUTR = 1,   // Digital Input 33
  // --- Add future channels here ---
  // DSP_CHANNEL_DIN34 = 2,
  // DSP_CHANNEL_AIN_L = 3,
  // DSP_CHANNEL_AIN_R = 4,
  DSP_CHANNEL_COUNT         // Always last - gives us the count
} dsp_channel_t;

// =============================================================================
// Parameter Block Types (expandable)
// =============================================================================

typedef enum {
  DSP_BLOCK_MUTE = 0,
  // --- Add future block types here ---
  // DSP_BLOCK_VOLUME,
  // DSP_BLOCK_EQ_BAND1,
  // DSP_BLOCK_COMPRESSOR,
  // DSP_BLOCK_DELAY,
  DSP_BLOCK_TYPE_COUNT
} dsp_block_type_t;

// =============================================================================
// Channel State Structure
// =============================================================================

typedef struct {
  bool     muted;           // Current mute state
  // --- Add future per-channel state here ---
  // float    volume;       // 0.0 to 1.0
  // bool     solo;
  // bool     phase_invert;
} dsp_channel_state_t;

// =============================================================================
// System Status
// =============================================================================

typedef struct {
  bool     dsp_alive;           // SPI communication OK
  bool     dsp_running;         // DSP core executing
  uint32_t spi_transactions;    // Total SPI transaction count
  uint32_t uptime_seconds;      // Seconds since boot
  uint32_t last_error_code;     // Last error (0 = none)
  dsp_channel_state_t channels[DSP_CHANNEL_COUNT];
} dsp_system_status_t;

// =============================================================================
// Initialization
// =============================================================================

/**
 * Initialize the DSP control layer.
 * Calls adau1467_spi_init() internally.
 * Optionally resets the ADAU1467.
 *
 * @param reset_dsp  If true, pulse the reset line and wait for self-boot
 */
void dsp_control_init(bool reset_dsp);

// =============================================================================
// Mute Control
// =============================================================================

/**
 * Set the mute state for a channel.
 *
 * @param channel  Which channel to mute/unmute
 * @param muted    true = mute (silence), false = unmute (pass-through)
 * @return         true on successful SPI write
 */
bool dsp_set_mute(dsp_channel_t channel, bool muted);

/**
 * Get the current mute state for a channel (from cached state).
 *
 * @param channel  Which channel to query
 * @return         true if muted
 */
bool dsp_get_mute(dsp_channel_t channel);

/**
 * Toggle the mute state for a channel.
 *
 * @param channel  Which channel to toggle
 * @return         true on successful SPI write
 */
bool dsp_toggle_mute(dsp_channel_t channel);

// =============================================================================
// Status & Diagnostics
// =============================================================================

/**
 * Refresh system status (reads from DSP via SPI).
 * Call this periodically to keep status up to date.
 */
void dsp_refresh_status();

/**
 * Get a copy of the current system status.
 */
dsp_system_status_t dsp_get_status();

/**
 * Get a human-readable name for a channel.
 */
const char* dsp_channel_name(dsp_channel_t channel);

// =============================================================================
// Future Expansion Stubs
// =============================================================================

// Uncomment and implement these as you add SigmaStudio blocks:

// bool dsp_set_volume(dsp_channel_t channel, float volume);
// float dsp_get_volume(dsp_channel_t channel);

// bool dsp_set_eq(dsp_channel_t channel, uint8_t band, float gain_db,
//                 float freq_hz, float q);

// bool dsp_set_delay(dsp_channel_t channel, float delay_ms);

// bool dsp_set_compressor(dsp_channel_t channel, float threshold_db,
//                         float ratio, float attack_ms, float release_ms);

// bool dsp_load_preset(uint8_t preset_index);

#endif // DSP_CONTROL_H
