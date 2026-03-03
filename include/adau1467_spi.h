// =============================================================================
// adau1467_spi.h - ADAU1467 SPI Communication Driver
// =============================================================================
// Low-level SPI read/write functions for the ADAU1467 slave control port.
// Designed as a reusable driver layer for future DSP control expansion.
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
 * SPI Write Transaction Format:
 *   CS LOW
 *   [0x00] [0x00] [Addr_Hi] [Addr_Lo] [Data_0] [Data_1] ... [Data_N-1]
 *   CS HIGH
 *
 * @param address  16-bit register/parameter address
 * @param data     Pointer to data bytes to write
 * @param length   Number of data bytes (typically 4 for parameters)
 * @return         true on success
 */
bool adau1467_write(uint16_t address, const uint8_t* data, uint16_t length);

/**
 * Read data from an ADAU1467 register or parameter RAM address.
 *
 * SPI Read Transaction Format:
 *   CS LOW
 *   [0x00] [0x01] [Addr_Hi] [Addr_Lo] [Len_Hi] [Len_Lo]
 *   then clock in [length] bytes of data
 *   CS HIGH
 *
 * @param address  16-bit register/parameter address
 * @param data     Pointer to buffer for received data
 * @param length   Number of data bytes to read
 * @return         true on success
 */
bool adau1467_read(uint16_t address, uint8_t* data, uint16_t length);

// =============================================================================
// Parameter Helpers (8.24 Fixed-Point)
// =============================================================================

/**
 * Write a 32-bit value to a parameter address.
 * Handles the 4-byte big-endian formatting.
 *
 * @param address  Parameter RAM address
 * @param value    32-bit value (e.g., 8.24 fixed-point)
 * @return         true on success
 */
bool adau1467_write_param(uint16_t address, uint32_t value);

/**
 * Read a 32-bit value from a parameter address.
 *
 * @param address  Parameter RAM address
 * @param value    Pointer to receive the 32-bit value
 * @return         true on success
 */
bool adau1467_read_param(uint16_t address, uint32_t* value);

/**
 * Convert a float (0.0 to 1.0) to 8.24 fixed-point format.
 *
 * @param value  Float value (clamped to 0.0 - 1.0 range)
 * @return       8.24 fixed-point representation
 */
uint32_t adau1467_float_to_fixed(float value);

/**
 * Convert 8.24 fixed-point to float.
 *
 * @param fixed  8.24 fixed-point value
 * @return       Float representation
 */
float adau1467_fixed_to_float(uint32_t fixed);

// =============================================================================
// Diagnostics
// =============================================================================

/**
 * Check if the ADAU1467 appears to be responding on SPI.
 * Attempts to read a known register and validates the response.
 *
 * @return true if DSP is communicating
 */
bool adau1467_is_alive();

/**
 * Read the DSP Run register to check if program is executing.
 *
 * @return true if DSP core is running
 */
bool adau1467_is_running();

/**
 * Get the total number of SPI transactions since boot (for diagnostics).
 */
uint32_t adau1467_get_transaction_count();

#endif // ADAU1467_SPI_H
