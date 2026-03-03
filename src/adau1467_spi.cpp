// =============================================================================
// adau1467_spi.cpp - ADAU1467 SPI Communication Driver Implementation
// =============================================================================
// SPI Protocol (per ADI reference code):
//   Write: [0x00] [Addr_Hi] [Addr_Lo] [Data0] [Data1] ... [DataN]
//   Read:  [0x01] [Addr_Hi] [Addr_Lo] [0x00]  [0x00]  ... [0x00]
//   - 3-byte header only (command + 16-bit address)
//   - Command byte: 0x00 = write, 0x01 = read
//   - No chip address byte, no length bytes
//
// Memory depths:
//   Parameter RAM (0x0000–0x09EB): 4 bytes per word (32-bit, 8.24 fixed-point)
//   Control Registers (0xF000+):   2 bytes per register (16-bit)
// =============================================================================

#include "adau1467_spi.h"
#include "hardware_config.h"

// SPI settings object - reused for all transactions
static SPISettings adauSPISettings(SPI_FREQUENCY, SPI_BIT_ORDER, SPI_MODE);

// Transaction counter for diagnostics
static volatile uint32_t spi_transaction_count = 0;

// =============================================================================
// Initialization
// =============================================================================

void adau1467_spi_init() {
  // Configure chip select pin
  pinMode(ADAU1467_CS, OUTPUT);
  digitalWrite(ADAU1467_CS, HIGH);  // CS idle HIGH (active LOW)

  // Configure reset pin
  pinMode(ADAU1467_RESET, OUTPUT);
  digitalWrite(ADAU1467_RESET, HIGH);  // Not in reset

  // Initialize VSPI bus
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, ADAU1467_CS);

  // Toggle CS three times to switch ADAU1467 from I2C to SPI mode
  // (slave control port defaults to I2C after power-up/reset)
  Serial.println("[SPI] Toggling CS to enter SPI mode...");
  for (uint8_t i = 0; i < 3; i++) {
    digitalWrite(ADAU1467_CS, LOW);
    delayMicroseconds(10);
    digitalWrite(ADAU1467_CS, HIGH);
    delayMicroseconds(10);
  }
  Serial.println("[SPI] SPI mode activated");

  // Configure status LED
  pinMode(LED_SPI_ACTIVITY, OUTPUT);
  digitalWrite(LED_SPI_ACTIVITY, LOW);

  Serial.println("[SPI] ADAU1467 SPI driver initialized");
  Serial.printf("[SPI]   MOSI=%d  MISO=%d  SCK=%d  CS=%d\n",
                SPI_MOSI, SPI_MISO, SPI_SCK, ADAU1467_CS);
  Serial.printf("[SPI]   Frequency: %d Hz, Mode: %d\n",
                SPI_FREQUENCY, SPI_MODE);

  spi_transaction_count = 0;
}

void adau1467_reset() {
  Serial.println("[SPI] Resetting ADAU1467...");

  digitalWrite(ADAU1467_RESET, LOW);
  delay(DSP_RESET_PULSE_MS);
  digitalWrite(ADAU1467_RESET, HIGH);

  // Wait for self-boot from EEPROM
  Serial.printf("[SPI] Waiting %d ms for self-boot...\n", DSP_BOOT_DELAY_MS);
  delay(DSP_BOOT_DELAY_MS);

  Serial.println("[SPI] ADAU1467 reset complete");
}

// =============================================================================
// Low-Level SPI Read/Write
// =============================================================================

bool adau1467_write(uint16_t address, const uint8_t* data, uint16_t length) {
  if (data == nullptr && length > 0) {
    Serial.println("[SPI] ERROR: null data pointer");
    return false;
  }

  // Flash activity LED
  digitalWrite(LED_SPI_ACTIVITY, HIGH);

  SPI.beginTransaction(adauSPISettings);
  delayMicroseconds(SPI_CS_SETUP_US);
  digitalWrite(ADAU1467_CS, LOW);
  delayMicroseconds(SPI_CS_SETUP_US);

  // Send write header: [0x00] [Addr_Hi] [Addr_Lo]  (3 bytes per ADI reference)
  SPI.transfer(ADAU_SPI_WRITE_CMD);
  SPI.transfer((address >> 8) & 0xFF);
  SPI.transfer(address & 0xFF);

  // Send data bytes
  for (uint16_t i = 0; i < length; i++) {
    SPI.transfer(data[i]);
  }

  delayMicroseconds(SPI_CS_HOLD_US);
  digitalWrite(ADAU1467_CS, HIGH);
  SPI.endTransaction();

  spi_transaction_count++;
  digitalWrite(LED_SPI_ACTIVITY, LOW);

  // Debug output
  Serial.printf("[SPI] WRITE: [0x%02X] Addr [0x%04X] Data:",
    ADAU_SPI_WRITE_CMD, address);
  for (uint16_t i = 0; i < length; i++) {
    Serial.printf(" 0x%02X", data[i]);
  }
  Serial.println();

  return true;
}

bool adau1467_read(uint16_t address, uint8_t* data, uint16_t length) {
  if (data == nullptr || length == 0) {
    Serial.println("[SPI] ERROR: invalid read parameters");
    return false;
  }

  // Flash activity LED
  digitalWrite(LED_SPI_ACTIVITY, HIGH);

  SPI.beginTransaction(adauSPISettings);
  delayMicroseconds(SPI_CS_SETUP_US);
  digitalWrite(ADAU1467_CS, LOW);
  delayMicroseconds(SPI_CS_SETUP_US);

  // Send read header: [0x01] [Addr_Hi] [Addr_Lo]  (3 bytes per ADI reference)
  SPI.transfer(ADAU_SPI_READ_CMD);
  SPI.transfer((address >> 8) & 0xFF);
  SPI.transfer(address & 0xFF);

  // Clock in data bytes
  for (uint16_t i = 0; i < length; i++) {
    data[i] = SPI.transfer(0x00);
  }

  delayMicroseconds(SPI_CS_HOLD_US);
  digitalWrite(ADAU1467_CS, HIGH);
  SPI.endTransaction();

  spi_transaction_count++;
  digitalWrite(LED_SPI_ACTIVITY, LOW);

  // Debug output
  Serial.printf("[SPI] READ:  [0x%02X] Addr [0x%04X] Got:",
    ADAU_SPI_READ_CMD, address);
  for (uint16_t i = 0; i < length; i++) {
    Serial.printf(" 0x%02X", data[i]);
  }
  Serial.println();

  return true;
}

// =============================================================================
// Parameter RAM Helpers (32-bit, 8.24 Fixed-Point)
// =============================================================================

bool adau1467_write_param(uint16_t address, uint32_t value) {
  uint8_t data[4];
  data[0] = (value >> 24) & 0xFF;
  data[1] = (value >> 16) & 0xFF;
  data[2] = (value >> 8) & 0xFF;
  data[3] = value & 0xFF;

  return adau1467_write(address, data, 4);
}

bool adau1467_read_param(uint16_t address, uint32_t* value) {
  uint8_t data[4];

  if (!adau1467_read(address, data, 4)) {
    return false;
  }

  *value = ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) |
           (uint32_t)data[3];

  return true;
}

// =============================================================================
// Control Register Helpers (16-bit)
// =============================================================================

bool adau1467_write_register(uint16_t address, uint16_t value) {
  uint8_t data[2];
  data[0] = (value >> 8) & 0xFF;
  data[1] = value & 0xFF;

  return adau1467_write(address, data, 2);
}

bool adau1467_read_register(uint16_t address, uint16_t* value) {
  uint8_t data[2];

  // whilst the second input arguemnt is a pointer (reference to memory) the & is not
  // implicitely used here. in C/C++ an array name already is a pointer to the first element
  // BUT, if you wanted to explicitely define the pointer you could use:  &data[0] (address of the first element)
  if (!adau1467_read(address, data, 2)) {
    return false;
  }

  // Note to self:
  // *value dereferences the pointer - meaning "go to that address and write to it"
  // data[0] is first SPI byte (high byte), data[1] is the second byte (low byte)
  // (uint16_t)data[0] casts it from 8-bit to 16-bit value
  // << 8 shifts it left 8 bits so it leaves the lower 8 bits for data[1]
  // now the two bytes are reassembled into the original 16-bit register value
  *value = ((uint16_t)data[0] << 8) | (uint16_t)data[1];

  return true;
}

// =============================================================================
// Fixed-Point Conversion Utilities
// =============================================================================

uint32_t adau1467_float_to_fixed(float value) {
  // Clamp to valid range
  if (value > 1.0f) value = 1.0f;
  if (value < 0.0f) value = 0.0f;

  // Convert to 8.24 fixed-point: multiply by 2^24
  return (uint32_t)(value * 16777216.0f);  // 2^24 = 16777216
}

float adau1467_fixed_to_float(uint32_t fixed) {
  return (float)fixed / 16777216.0f;
}

// =============================================================================
// Diagnostics
// =============================================================================

bool adau1467_is_alive() {
  // Read the PLL Lock register (0xF004) - 16-bit control register
  uint16_t pll_lock = 0;
  bool success = adau1467_read_register(ADAU1467_REG_PLL_LOCK, &pll_lock);

  if (success) {
    Serial.printf("[SPI] ADAU1467 alive check: PLL Lock = 0x%04X\n", pll_lock);
  } else {
    Serial.println("[SPI] ADAU1467 alive check: FAILED");
  }

  return success;
}

bool adau1467_is_running() {
  // Read the CORE_STATUS register (0xF405) - 16-bit, read-only
  // Bit 0: 1 = core is running, 0 = core is stopped
  uint16_t core_status = 0;

  // &core_status gives the function a pointer to the variables location in memory so it can write to it
  if (!adau1467_read_register(ADAU1467_REG_CORE_STATUS, &core_status)) {
    return false;
  }

  Serial.printf("[SPI] Core status: 0x%04X\n", core_status);
  return (core_status & 0x01) != 0;
}

uint32_t adau1467_get_transaction_count() {
  return spi_transaction_count;
}