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
#define SPI_MOSI          23    // ESP32 → ADAU1467 (J1 Pin 3)
#define SPI_MISO          19    // ADAU1467 → ESP32 (J1 Pin 4)
#define SPI_SCK           18    // SPI Clock (J1 Pin 5)
#define ADAU1467_CS        5    // Chip Select (J1 Pin 7)

// --- ADAU1467 Control ---
#define ADAU1467_RESET    25    // Reset line (J1 Pin 9, active LOW)

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

#define SPI_FREQUENCY     100000    // 100 kHz - matches ADI example code
                                     // Can increase to 10 MHz once stable
#define SPI_MODE          SPI_MODE3  // CPOL=1, CPHA=1 (ADAU1467 supports Mode 0 & 3)
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

// SPI transaction header bytes
#define ADAU_SPI_WRITE_CMD    0x00  // Single command byte: 0x00 = WRITE
#define ADAU_SPI_READ_CMD     0x01  // Single command byte: 0x01 = READ

// =============================================================================
// ADAU1467 Register / Parameter Addresses
// =============================================================================
// IMPORTANT: These addresses are PLACEHOLDERS. You MUST update them with
// the actual addresses from your SigmaStudio project.
//
// How to find your addresses:
//   1. In SigmaStudio, right-click your Mute block → Export
//   2. Or use the Capture Window to observe SPI writes when toggling mute
//   3. The exported header file will contain the parameter addresses
//
// Mute blocks in SigmaStudio use a single 4-byte parameter:
//   0x00000000 = MUTED   (signal multiplied by 0.0)
//   0x00800000 = UNMUTED (signal multiplied by 1.0 in 8.24 fixed-point)
// =============================================================================

// --- Mute Block Addresses (UPDATE THESE from SigmaStudio export) ---
// Digital Input 32 mute block parameter address
#define MUTE_OUTL_ADDR       0x0275  // MuteL - address 629

// Digital Input 33 mute block parameter address
#define MUTE_OUTR_ADDR       0x0276  // MuteR - address 630

// --- Core ADAU1467 Registers (from datasheet, for status/diagnostics) ---
#define ADAU1467_REG_PLL_CTRL0        0xF000  // PLL Control Register 0
#define ADAU1467_REG_PLL_CTRL1        0xF001  // PLL Control Register 1
#define ADAU1467_REG_PLL_CLK_SRC      0xF002  // PLL Clock Source
#define ADAU1467_REG_PLL_ENABLE       0xF003  // PLL Enable
#define ADAU1467_REG_PLL_LOCK         0xF004  // PLL Lock Status
#define ADAU1467_REG_MCLK_OUT         0xF005  // MCLK Output Control
#define ADAU1467_REG_PLL_WATCHDOG     0xF006  // PLL Watchdog
#define ADAU1467_REG_CLK_GEN1_M       0xF020  // Clock Gen 1 M value
#define ADAU1467_REG_CLK_GEN1_N       0xF021  // Clock Gen 1 N value
#define ADAU1467_REG_DSP_RUN          0xF089  // DSP Run Register
#define ADAU1467_REG_DSP_SR_SETTING   0xF009  // Sample Rate Setting
#define ADAU1467_REG_KILL_CORE        0xF08B  // Kill Core Register
#define ADAU1467_REG_PANIC_CLEAR      0xF021  // Panic Clear
#define ADAU1467_REG_PANIC_PARITY_MASK 0xF022 // Panic Parity Mask
#define ADAU1467_REG_PANIC_SOFTWARE   0xF023  // Panic Software
#define ADAU1467_REG_PANIC_WD         0xF024  // Panic Watchdog
#define ADAU1467_REG_PANIC_STACK      0xF025  // Panic Stack Overflow
#define ADAU1467_REG_PANIC_LOOP       0xF026  // Panic Loop Stack

// --- 8.24 Fixed-Point Values for Mute Control ---
// In SigmaStudio's 8.24 format: 8 bits integer, 24 bits fractional
#define ADAU_FIXEDPOINT_ZERO  0x00000000  // 0.0 = MUTED
#define ADAU_FIXEDPOINT_UNITY 0x00000001  // UNMUTED (integer mute block)

// =============================================================================
// Timing Constants
// =============================================================================

#define DSP_RESET_PULSE_MS    20    // Reset pulse width
#define DSP_BOOT_DELAY_MS     500   // Wait for self-boot from EEPROM
#define SPI_CS_SETUP_US       1     // CS setup time before clock
#define SPI_CS_HOLD_US        1     // CS hold time after clock
#define STATUS_REPORT_INTERVAL_MS 2000  // How often to push status to web UI
#define LED_BLINK_INTERVAL_MS    500    // LED blink rate for status indication

#endif // HARDWARE_CONFIG_H
