# ADAU1467 Web Control — SPI Test Framework

Real-time web-based control of Analog Devices ADAU1467 DSP parameters via an ESP32 DoIt DevKit V1. Designed as an extensible test framework for developing SPI-based DSP control.

## What This Does

Your ESP32 creates a Wi-Fi access point, serves a browser-based control UI, and communicates with the ADAU1467 eval board over SPI. The web UI provides:

- **Mute controls** — toggle output mutes on Left and Right channels
- **Level controls** — adjust gain (−40 dBFS to 0 dBFS) for Speaker and Headphone outputs via sliders or direct numeric entry

All controls update in real-time via WebSocket. When you move a slider or press a mute button, the ESP32 writes the corresponding parameter to the ADAU1467's parameter RAM over SPI within milliseconds.

## Architecture

```
┌─────────────────────────────────────────────────────┐
│  Web Browser (phone / laptop)                       │
│  ┌─────────────────────────────────────────────┐    │
│  │  ADAU1467 Control UI                        │    │
│  │  [MUTE OUT L] [MUTE OUT R]                  │    │
│  │  [─────── Speaker Level ───────]            │    │
│  │  [───── Headphone Level ───────]            │    │
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
│  │  DSP Control Layer (mute, gain)             │    │
│  │  ADAU1467 SPI Driver                        │    │
│  └──────────────────┬──────────────────────────┘    │
│        GPIO18 SCK   │  GPIO23 MOSI                  │
│        GPIO19 MISO  │  GPIO5  CS                    │
│        GPIO25 RESET │                               │
└─────────────────────┼───────────────────────────────┘
                      │ SPI (100 kHz, Mode 3)
┌─────────────────────┼───────────────────────────────┐
│  EVAL-ADAU1467Z     │                               │
│  ┌──────────────────┴──────────────────────────┐    │
│  │  J1 Header (10-pin SPI Slave Control Port)  │    │
│  │  ADAU1467 SigmaDSP                          │    │
│  │  Running self-booted program from EEPROM    │    │
│  │  Parameter RAM ← mute & gain values here    │    │
│  └─────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────┘
```

## Hardware Wiring

Connect ESP32 to the **EVAL-ADAU1467Z J1 10-pin header** (the same header the USBi ribbon cable uses). **Disconnect the USBi ribbon cable first.**

| ESP32 Pin | Signal | J1 Pin | Notes |
|-----------|--------|--------|-------|
| GPIO 23   | MOSI   | 8      | ESP32 → ADAU1467 |
| GPIO 19   | MISO   | 5      | ADAU1467 → ESP32 |
| GPIO 18   | SCK    | 7      | SPI Clock |
| GPIO 5    | CS     | 9      | Chip Select (active LOW) |
| GND       | GND    | 2, 10  | Common ground |
| GPIO 25   | RESET  | —      | Optional: reset ADAU1467 |

**Also connect:**
- GPIO 12 → LED + resistor → GND (Wi-Fi status)
- GPIO 13 → LED + resistor → GND (SPI activity)
- GPIO 14 → LED + resistor → GND (DSP status)
- GPIO 26 → Momentary button → GND
- GPIO 27 → Toggle switch → GND
- GPIO 34 → Potentiometer wiper (ADC, 0–3.3 V, input-only pin)

## Prerequisites

1. **ADAU1467 must be self-booted** with a SigmaStudio program that includes:
   - Output mute blocks for Left and Right channels
   - Gain (volume) control blocks for the Speaker and Headphone paths
2. The SELFBOOT switch (S3 position 1) on the eval board must be ON
3. PlatformIO IDE installed in VS Code

## ⚠️ CRITICAL: Get Your SigmaStudio Parameter Addresses

The addresses in `hardware_config.h` **must match your SigmaStudio project**. The currently configured addresses are:

```c
// Mute blocks
#define MUTE_OUTL_ADDR        0x0275  // Output Left mute
#define MUTE_OUTR_ADDR        0x0276  // Output Right mute

// Gain control blocks (8.24 fixed-point, single param controls stereo pair)
#define GAIN_SPEAKERS_ADDR    0x0267  // Loudspeaker output gain
#define GAIN_HEADPHONES_ADDR  0x0268  // Headphone output gain
```

If your SigmaStudio program assigns different addresses, update these constants before flashing.

### Finding Parameter Addresses — Method 1: SigmaStudio Export (Recommended)

1. In SigmaStudio, right-click your project → **Export System Files**
2. Open the generated `_IC_1_PARAM.h` file
3. Find the `#define` for each block — for example:
   ```c
   #define MOD_MUTECTL_OUTL_ALG0_MUTEONOFF_ADDR  0x0275
   #define MOD_PCGAINCTRL_ALG0_TARGET_ADDR        0x0267
   ```
4. Copy the addresses into `hardware_config.h`

### Finding Parameter Addresses — Method 2: Capture Window

1. Enable **Tools → Capture Window** in SigmaStudio
2. Click the mute or volume control in the SigmaStudio UI
3. Observe the SPI write in the capture window — note the address and data
4. Mute-on writes `0x00000000`; mute-off writes `0x00800000`
5. Gain values are 8.24 fixed-point: `0 dBFS = 0x00800000`, `−6 dBFS ≈ 0x00400000`

### Finding Parameter Addresses — Method 3: Developer Console

The web UI includes a Developer Console for reading and writing arbitrary registers. Use this to probe addresses interactively.

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
4. The UI shows two sections:
   - **Mute Control · Outputs** — click MUTE buttons to toggle output mutes
   - **Level Control · PC Input** — drag sliders or type a value (−40 to 0 dBFS) and click Apply

Gain changes are throttled to one SPI write every 50 ms while dragging, then a final write on release, keeping the interface responsive without flooding the bus.

## Project Structure

```
adau1467-web-control/
├── platformio.ini              # PlatformIO configuration
├── include/
│   ├── hardware_config.h       # Pin definitions, register addresses, constants
│   ├── adau1467_spi.h          # SPI driver API
│   ├── dsp_control.h           # High-level DSP control API (mute + gain)
│   └── web_server.h            # Web server API
├── src/
│   ├── main.cpp                # Entry point, setup/loop
│   ├── adau1467_spi.cpp        # SPI read/write implementation
│   ├── dsp_control.cpp         # Mute and gain control logic
│   └── web_server.cpp          # Wi-Fi, HTTP, WebSocket server
├── data/
│   └── index.html              # Web UI (uploaded to LittleFS)
└── README.md
```

## Extending the Framework

### Adding EQ, Compressor, Delay, etc.

Follow the same pattern used for gain controls. Each new parameter type requires:

- An address constant in `hardware_config.h`
- A new identifier in the appropriate enum in `dsp_control.h`
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
| `set_gain` | `gain_id`, `gain_db` | Set gain in dBFS (−40.0 to 0.0) |
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
    { "id": 0, "name": "Output Left",  "muted": false },
    { "id": 1, "name": "Output Right", "muted": true  }
  ],
  "gains": [
    { "id": 0, "name": "Speakers",   "gain_db": "-6.0" },
    { "id": 1, "name": "Headphones", "gain_db": "0.0"  }
  ]
}
```

## Troubleshooting

| Symptom | Check |
|---------|-------|
| "DSP: No Response" in UI | SPI wiring, CS pin, ADAU1467 power, SELFBOOT enabled |
| Mute button has no effect on audio | Parameter addresses need updating from SigmaStudio export |
| Slider moves but gain doesn't change | Gain parameter addresses need updating from SigmaStudio export |
| Can't connect to Wi-Fi AP | ESP32 may not have booted — check serial monitor at 115200 baud |
| Web page won't load | Did you upload the filesystem image? (separate from firmware upload) |
| SPI transactions increment but no audio change | Address mismatch — use Developer Console to find correct addresses |
