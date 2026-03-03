// =============================================================================
// adau1467_spi.h - ADAU1467 SPI Communication Driver
// =============================================================================
// Low-level SPI read/write functions for the ADAU1467 slave control port.
//
// SPI Protocol (per ADI reference code):
//   Write: [0x00] [Addr_Hi] [Addr_Lo] [Data0] ... [DataN]
//   Read:  [0x01] [Addr_Hi] [Addr_Lo] [clk in data bytes]
//   - 3-byte header (command + 16-bit address)
//   - Command byte: 0x00 = write, 0x01 = read
//
// Memory map:
//   0x0000–0x09EB  Parameter RAM (4 bytes per word, 8.24 fixed-point)
//   0xF000–0xF8FF  Control Registers (2 bytes per register)
// =============================================================================

#ifndef ADAU1467_SPI_H
#define ADAU1467_SPI_H

#include <Arduino.h>
#include <SPI.h>

// =============================================================================
// Initialization
// =============================================================================

/**
 * Initialize the SPI bus and ADAU1467 control pins.
 * Toggles CS three times to switch slave control port from I2C to SPI mode.
 * Call this once during setup().
 */
void adau1467_spi_init();

/**
 * Reset the ADAU1467 by pulsing the reset pin LOW.
 * After reset, the DSP will self-boot from EEPROM if SELFBOOT is enabled.
 */
void adau1467_reset();

// =============================================================================
// Low-Level SPI Read/Write
// =============================================================================

/**
 * Write data to an ADAU1467 register or parameter RAM address.
 *
 * @param address  16-bit register/parameter address
 * @param data     Pointer to data bytes to write
 * @param length   Number of data bytes (4 for params, 2 for control regs)
 * @return         true on success
 */
bool adau1467_write(uint16_t address, const uint8_t* data, uint16_t length);

/**
 * Read data from an ADAU1467 register or parameter RAM address.
 *
 * @param address  16-bit register/parameter address
 * @param data     Pointer to buffer for received data
 * @param length   Number of data bytes to read
 * @return         true on success
 */
bool adau1467_read(uint16_t address, uint8_t* data, uint16_t length);

// =============================================================================
// Parameter RAM Helpers (32-bit, 8.24 Fixed-Point)
// =============================================================================

/**
 * Write a 32-bit value to a parameter RAM address.
 * Parameter RAM words are 4 bytes, big-endian.
 *
 * @param address  Parameter RAM address (0x0000–0x09EB)
 * @param value    32-bit value (e.g., 8.24 fixed-point)
 * @return         true on success
 */
bool adau1467_write_param(uint16_t address, uint32_t value);

/**
 * Read a 32-bit value from a parameter RAM address.
 *
 * @param address  Parameter RAM address (0x0000–0x09EB)
 * @param value    Pointer to receive the 32-bit value
 * @return         true on success
 */
bool adau1467_read_param(uint16_t address, uint32_t* value);

// =============================================================================
// Control Register Helpers (16-bit)
// =============================================================================

/**
 * Write a 16-bit value to a control register.
 * Control registers are 2 bytes, big-endian.
 *
 * @param address  Control register address (0xF000–0xF8FF)
 * @param value    16-bit register value
 * @return         true on success
 */
bool adau1467_write_register(uint16_t address, uint16_t value);

/**
 * Read a 16-bit value from a control register.
 *
 * @param address  Control register address (0xF000–0xF8FF)
 * @param value    Pointer to receive the 16-bit value
 * @return         true on success
 */
bool adau1467_read_register(uint16_t address, uint16_t* value);

// =============================================================================
// Fixed-Point Conversion Utilities
// =============================================================================

uint32_t adau1467_float_to_fixed(float value);
float adau1467_fixed_to_float(uint32_t fixed);

// =============================================================================
// Diagnostics
// =============================================================================

/**
 * Check if the ADAU1467 is responding on SPI.
 * Reads the PLL Lock register and validates the response.
 */
bool adau1467_is_alive();

/**
 * Check if the DSP core is running.
 * Reads the CORE_STATUS register (0xF405).
 */
bool adau1467_is_running();

/**
 * Get the total number of SPI transactions since boot.
 */
uint32_t adau1467_get_transaction_count();

#endif // ADAU1467_SPI_H