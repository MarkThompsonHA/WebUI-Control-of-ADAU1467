# ADAU1467 Web Control — SPI Test Framework

Real-time web-based control of Analog Devices ADAU1467 DSP parameters via an ESP32 DoIt DevKit V1. Designed as an extensible test framework for developing SPI-based DSP control.

## What This Does

Your ESP32 creates a Wi-Fi access point, serves a browser-based control UI, and communicates with the ADAU1467 eval board over SPI to toggle mute blocks on digital inputs 32 and 33. The web UI updates in real-time via WebSocket — when you press a mute button in the browser, the ESP32 writes the corresponding parameter to the ADAU1467's parameter RAM over SPI within milliseconds.

## Architecture

```
┌─────────────────────────────────────────────────────┐
│  Web Browser (phone / laptop)                       │
│  ┌─────────────────────────────────────────────┐    │
│  │  ADAU1467 Control UI                        │    │
│  │  [MUTE DIN32] [MUTE DIN33]                  │    │
│  │  Developer Console (raw register R/W)        │    │
│  └──────────────────┬──────────────────────────┘    │
│                     │ WebSocket (JSON)               │
└─────────────────────┼───────────────────────────────┘
                      │
┌─────────────────────┼───────────────────────────────┐
│  ESP32 DoIt DevKit  │                               │
│  ┌──────────────────┴──────────────────────────┐    │
│  │  Wi-Fi AP: "ADAU1467-Control"               │    │
│  │  Async Web Server + WebSocket               │    │
│  │  DSP Control Layer (mute, future: vol/EQ)   │    │
│  │  ADAU1467 SPI Driver                        │    │
│  └──────────────────┬──────────────────────────┘    │
│        GPIO18 SCK   │  GPIO23 MOSI                  │
│        GPIO19 MISO  │  GPIO5  CS                    │
│        GPIO25 RESET │                               │
└─────────────────────┼───────────────────────────────┘
                      │ SPI (1 MHz, Mode 3)
┌─────────────────────┼───────────────────────────────┐
│  EVAL-ADAU1467Z     │                               │
│  ┌──────────────────┴──────────────────────────┐    │
│  │  J1 Header (10-pin SPI Slave Control Port)  │    │
│  │  ADAU1467 SigmaDSP                          │    │
│  │  Running self-booted program from EEPROM    │    │
│  │  Parameter RAM ← mute values written here   │    │
│  └─────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────┘
```

## Hardware Wiring

Connect ESP32 to the **EVAL-ADAU1467Z J1 10-pin header** (the same header the USBi ribbon cable uses). **Disconnect the USBi ribbon cable first.**

| ESP32 Pin | Signal | J1 Pin | Notes |
|-----------|--------|--------|-------|
| GPIO 23   | MOSI   | 3      | ESP32 → ADAU1467 |
| GPIO 19   | MISO   | 4      | ADAU1467 → ESP32 |
| GPIO 18   | SCK    | 5      | SPI Clock |
| GPIO 5    | CS     | 7      | Chip Select (active LOW) |
| GND       | GND    | 2, 10  | Common ground |
| GPIO 25   | RESET  | 9      | Optional: reset ADAU1467 |

**Also connect:**
- GPIO 12 → LED + resistor → GND (Wi-Fi status)
- GPIO 13 → LED + resistor → GND (SPI activity)
- GPIO 14 → LED + resistor → GND (DSP status)
- GPIO 26 → Momentary button → GND (toggles DIN32 mute)

## Prerequisites

1. **ADAU1467 must be self-booted** with a SigmaStudio program that includes mute blocks on DIN32 and DIN33
2. The SELFBOOT switch (S3 position 1) on the eval board must be ON
3. PlatformIO IDE installed in VS Code

## ⚠️ CRITICAL: Get Your SigmaStudio Parameter Addresses

The placeholder addresses `0x0000` and `0x0001` in `hardware_config.h` **must be replaced** with the actual parameter RAM addresses from your SigmaStudio project. Here's how:

### Method 1: SigmaStudio Export (Recommended)

1. In SigmaStudio, right-click your mute block → **Export System Files**
2. Open the generated `_IC_1_PARAM.h` file
3. Find the `#define` for your mute block — it will look something like:
   ```c
   #define MOD_MUTE_DIN32_ALG0_MUTEONOFF_ADDR  0x0018
   ```
4. Copy these addresses into `hardware_config.h`:
   ```c
   #define MUTE_DIN32_ADDR  0x0018  // From SigmaStudio export
   #define MUTE_DIN33_ADDR  0x0019  // From SigmaStudio export
   ```

### Method 2: SigmaStudio Capture Window

1. Enable **Tools → Capture Window** in SigmaStudio
2. Click your mute block's on/off button in SigmaStudio
3. Observe the SPI write in the capture window — note the address and data values
4. A mute-on will write `0x00000000`, mute-off will write `0x00800000`

### Method 3: Use the Developer Console

The web UI includes a Developer Console that lets you read/write arbitrary registers. You can use this to experiment and find the correct addresses.

## Building & Uploading

```bash
# In VS Code with PlatformIO:
# 1. Open this folder as a project
# 2. Build: Ctrl+Alt+B  (or PlatformIO sidebar → Build)
# 3. Upload firmware: Ctrl+Alt+U
# 4. Upload filesystem (web UI): PlatformIO sidebar → "Upload Filesystem Image"
```

**Important:** You must upload the filesystem separately to get the web UI onto the ESP32's flash. In the PlatformIO sidebar, expand your environment and click **"Upload Filesystem Image"**.

## Using the Web UI

1. Power up the ESP32 and ADAU1467 eval board
2. On your phone/laptop, connect to Wi-Fi network **"ADAU1467-Control"** (password: `dsp1467ctrl`)
3. Open a browser and navigate to **http://192.168.4.1**
4. You'll see the control UI with mute buttons for DIN32 and DIN33
5. Click a mute button — the ESP32 writes to the ADAU1467 via SPI in real-time

## Project Structure

```
adau1467-web-control/
├── platformio.ini              # PlatformIO configuration
├── include/
│   ├── hardware_config.h       # Pin definitions, register addresses, constants
│   ├── adau1467_spi.h          # SPI driver API
│   ├── dsp_control.h           # High-level DSP control API
│   └── web_server.h            # Web server API
├── src/
│   ├── main.cpp                # Entry point, setup/loop
│   ├── adau1467_spi.cpp        # SPI read/write implementation
│   ├── dsp_control.cpp         # Mute/volume/EQ control logic
│   └── web_server.cpp          # Wi-Fi, HTTP, WebSocket server
├── data/
│   └── index.html              # Web UI (uploaded to LittleFS)
└── README.md
```

## Extending the Framework

### Adding a Volume Control

1. **SigmaStudio**: Add a volume block in your signal path, export to get the parameter address
2. **hardware_config.h**: Add `#define VOLUME_DIN32_ADDR 0xXXXX`
3. **dsp_control.h/cpp**: Uncomment and implement `dsp_set_volume()`
4. **web_server.cpp**: Add a `"set_volume"` action handler in `handle_ws_message()`
5. **index.html**: Add a slider control that sends `{ action: "set_volume", channel: 0, volume: 0.75 }`

### Adding EQ, Compressor, Delay, etc.

Follow the same pattern. The architecture is designed so each new parameter type requires:
- An address constant in `hardware_config.h`
- A new entry in `dsp_block_type_t` enum
- A control function in `dsp_control.cpp`
- A WebSocket action handler in `web_server.cpp`
- A UI control in `index.html`

### Switching to Station Mode (connect to your router)

In `hardware_config.h`, uncomment and set:
```c
#define WIFI_STA_SSID     "YourNetwork"
#define WIFI_STA_PASSWORD "YourPassword"
```
The ESP32 will try to join your network first, and fall back to AP mode if that fails.

## WebSocket API Reference

All communication between the browser and ESP32 uses JSON over WebSocket at `ws://<ip>/ws`.

### Commands (Browser → ESP32)

| Action | Fields | Description |
|--------|--------|-------------|
| `set_mute` | `channel`, `muted` | Set mute state (true/false) |
| `toggle_mute` | `channel` | Toggle mute on a channel |
| `get_status` | — | Request full status update |
| `reset_dsp` | — | Pulse ADAU1467 reset line |
| `write_param` | `address`, `value` | Raw 32-bit parameter write |
| `read_param` | `address` | Raw 32-bit parameter read |

### Status Message (ESP32 → Browser)

```json
{
  "type": "status",
  "dsp_alive": true,
  "dsp_running": true,
  "spi_transactions": 42,
  "uptime": 3600,
  "wifi_clients": 1,
  "error": 0,
  "channels": [
    { "id": 0, "name": "Digital In 32", "muted": false },
    { "id": 1, "name": "Digital In 33", "muted": true }
  ]
}
```

## Troubleshooting

| Symptom | Check |
|---------|-------|
| "DSP: No Response" in UI | SPI wiring, CS pin, ADAU1467 power, SELFBOOT enabled |
| Mute button has no effect on audio | Parameter addresses need updating from SigmaStudio export |
| Can't connect to Wi-Fi AP | ESP32 may not have booted — check serial monitor at 115200 |
| Web page won't load | Did you upload the filesystem image? (separate from firmware upload) |
| SPI transactions increment but no audio change | Address mismatch — use Developer Console to find correct addresses |
