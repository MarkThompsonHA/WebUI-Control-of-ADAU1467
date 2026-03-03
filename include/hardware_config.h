// =============================================================================
// hardware_config.h - Pin Definitions & ADAU1467 Register Map
// =============================================================================
// Central hardware configuration for ESP32 DoIt DevKit V1 + ADAU1467
// =============================================================================

#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include <Arduino.h>

// =============================================================================
// ESP32 Pin Assignments (DoIt DevKit V1 - 36 pin)
// =============================================================================

// --- SPI Bus (VSPI) - Connected to ADAU1467 Slave Control Port via J1 ---
#define SPI_MOSI          23    // ESP32 → ADAU1467 (J1 Pin 8)
#define SPI_MISO          19    // ADAU1467 → ESP32 (J1 Pin 5)
#define SPI_SCK           18    // SPI Clock (J1 Pin 7)
#define ADAU1467_CS        5    // Chip Select (J1 Pin 9)

// --- ADAU1467 Control ---
#define ADAU1467_RESET    25    // Reset line (active LOW)

// --- Status LEDs ---
#define LED_WIFI          12    // Wi-Fi connection status
#define LED_SPI_ACTIVITY  13    // SPI transaction indicator
#define LED_DSP_STATUS    14    // DSP running / error indicator

// --- User Interface ---
#define LEVEL_POT_PIN     34    // Potentiometer input (ADC, input-only)
#define BUTTON_PIN        26    // Momentary pushbutton
#define TOGGLE_PIN        27    // Toggle switch

// =============================================================================
// SPI Configuration
// =============================================================================

#define SPI_FREQUENCY     100000    // 100 kHz - per ADI reference code
#define SPI_MODE          SPI_MODE3 // CPOL=1, CPHA=1
#define SPI_BIT_ORDER     MSBFIRST

// =============================================================================
// Wi-Fi Configuration
// =============================================================================

// Access Point mode settings (no router needed)
#define WIFI_AP_SSID      "ADAU1467-Control"
#define WIFI_AP_PASSWORD  "dsp1467ctrl"
#define WIFI_AP_CHANNEL   1
#define WIFI_AP_MAX_CONN  4

// Optional: Station mode (connect to existing network)
// Uncomment and set these to use STA mode instead of AP mode
// #define WIFI_STA_SSID     "YourNetwork"
// #define WIFI_STA_PASSWORD "YourPassword"

// Web server port
#define WEB_SERVER_PORT   80

// =============================================================================
// ADAU1467 SPI Protocol Constants
// =============================================================================

// SPI transaction header bytes (3-byte header per ADI reference)
#define ADAU_SPI_WRITE_CMD    0x00  // Single command byte: 0x00 = WRITE
#define ADAU_SPI_READ_CMD     0x01  // Single command byte: 0x01 = READ

// =============================================================================
// ADAU1467 Parameter Addresses (from SigmaStudio export)
// =============================================================================

// --- Mute Block Addresses ---
#define MUTE_OUTL_ADDR        0x0275  // MuteL - address 629
#define MUTE_OUTR_ADDR        0x0276  // MuteR - address 630

// --- Gain Control Addresses (8.24 fixed-point, single param controls stereo pair) ---
#define GAIN_SPEAKERS_ADDR    0x0267  // PC_GainCtrl - loudspeaker output (addr 615)
#define GAIN_HEADPHONES_ADDR  0x0268  // PC_GainCtrl_2 - headphone output (addr 616)

// --- Gain Control Limits ---
#define GAIN_MIN_DB          -40.0f   // Minimum gain in dBFS
#define GAIN_MAX_DB            0.0f   // Maximum gain in dBFS (unity)
#define GAIN_DEFAULT_DB        0.0f   // Default gain at startup

// =============================================================================
// ADAU1467 Control Registers (from datasheet Rev. A)
// All control registers are 16-bit (2 bytes)
// =============================================================================

// --- PLL & Clock Registers ---
#define ADAU1467_REG_PLL_CTRL0        0xF000  // PLL Control Register 0
#define ADAU1467_REG_PLL_CTRL1        0xF001  // PLL Control Register 1
#define ADAU1467_REG_PLL_CLK_SRC      0xF002  // selects the source of the clock used for inputs. 0 - direct in from MCLK pin. 1 - PLL clock.
#define ADAU1467_REG_PLL_ENABLE       0xF003  // enables or disables the PLL. PLL won't clock until bit 0 is enabled.
#define ADAU1467_REG_PLL_LOCK         0xF004  // a read-only flag representing status of PLL. 1 - locked, 0 - unlocked
#define ADAU1467_REG_MCLK_OUT         0xF005  // MCLK Output Control
#define ADAU1467_REG_PLL_WATCHDOG     0xF006  // PLL Watchdog
#define ADAU1467_REG_CLK_GEN1_M       0xF020  // Clock Gen 1 M value
#define ADAU1467_REG_CLK_GEN1_N       0xF021  // Clock Gen 1 N value

// --- DSP Core Control Registers ---
#define ADAU1467_REG_HIBERNATE        0xF400  // Hibernate control
#define ADAU1467_REG_START_PULSE      0xF401  // Start pulse
#define ADAU1467_REG_START_CORE       0xF402  // Start core
#define ADAU1467_REG_KILL_CORE        0xF403  // Kill core
#define ADAU1467_REG_START_ADDRESS    0xF404  // Program start address
#define ADAU1467_REG_CORE_STATUS      0xF405  // Core status (read-only: bit 0 = running)

// --- Panic Registers ---
#define ADAU1467_REG_PANIC_CLEAR      0xF421  // Clear panic flags
#define ADAU1467_REG_PANIC_PARITY_MASK 0xF422 // Panic parity mask
#define ADAU1467_REG_PANIC_SOFTWARE   0xF423  // Software panic
#define ADAU1467_REG_PANIC_WD         0xF424  // Watchdog panic
#define ADAU1467_REG_PANIC_STACK      0xF425  // Stack overflow panic
#define ADAU1467_REG_PANIC_LOOP       0xF426  // Loop stack panic
#define ADAU1467_REG_PANIC_FLAG       0xF427  // Panic flag (read-only)
#define ADAU1467_REG_PANIC_CODE       0xF428  // Panic code (read-only)

// --- Misc Registers ---
#define ADAU1467_REG_SOFT_RESET       0xF890  // Software reset

// =============================================================================
// Safeload Registers (for atomic coefficient updates)
// =============================================================================

#define ADAU1467_REG_SAFELOAD_DATA0   0x6000  // Safeload data slot 0
#define ADAU1467_REG_SAFELOAD_DATA1   0x6001  // Safeload data slot 1
#define ADAU1467_REG_SAFELOAD_DATA2   0x6002  // Safeload data slot 2
#define ADAU1467_REG_SAFELOAD_DATA3   0x6003  // Safeload data slot 3
#define ADAU1467_REG_SAFELOAD_DATA4   0x6004  // Safeload data slot 4
#define ADAU1467_REG_SAFELOAD_ADDR    0x6005  // Target parameter RAM address
#define ADAU1467_REG_SAFELOAD_COUNT_L 0x6006  // Number of words (lower)
#define ADAU1467_REG_SAFELOAD_COUNT_U 0x6007  // Number of words (upper)

// --- 8.24 Fixed-Point Values ---
#define ADAU_FIXEDPOINT_ZERO  0x00000000  // 0.0 = MUTED
#define ADAU_FIXEDPOINT_UNITY 0x00800000  // 1.0 = UNMUTED / 0 dBFS

// =============================================================================
// Timing Constants
// =============================================================================

#define DSP_RESET_PULSE_MS    20
#define DSP_BOOT_DELAY_MS     500
#define SPI_CS_SETUP_US       1
#define SPI_CS_HOLD_US        1
#define STATUS_REPORT_INTERVAL_MS 2000
#define LED_BLINK_INTERVAL_MS    500

#endif // HARDWARE_CONFIG_H