# artdmx-bridge32

ESP32 Art-Net to DMX512 gateway for the Arduino IDE. Receives DMX over WiFi, drives one or two RS-485 outputs, and optionally accepts wired DMX input as a fallback when Art-Net goes idle. Settings are stored in flash and edited through a built-in web UI.

**Firmware version:** `2.3.0-web-dmx`

> **Inspired by** [Connotron DMX Gateway](https://github.com/chaosloth/Connotron_DMX_Gateway) by [Christopher Connolly](https://github.com/chaosloth) — an ESP32 Art-Net to DMX512 project with WiFi configuration, OTA updates, and MAX485 wiring. **artdmx-bridge32** carries forward that idea with a modular codebase, NVS-backed web configuration, dual DMX outputs, and a FreeRTOS dual-core task layout.

---

## Features

- **Art-Net input** — receives DMX universes over WiFi
- **DMX1 output** — primary DMX512 output (always active)
- **DMX2 output** — mirrors Art-Net data to a second port (when DMX1 input is disabled)
- **DMX1 input fallback** — wired DMX on port 2 forwards to DMX1 output when Art-Net is idle (optional, requires reboot to enable)
- **Web dashboard** — live status, traffic indicators, and rolling log at `http://<hostname>.local`
- **Web configuration** — WiFi, hostname, Art-Net/DMX timing, input mode, and debug options saved to NVS
- **Config access point** — if WiFi fails, device opens `<hostname>-setup` so you can configure it in a browser
- **Dual-core tasks** — network/Art-Net/OTA on core 0, DMX on core 1
- **Queue-based handoff** — Art-Net callbacks never block on DMX transmission
- **Traffic LEDs** — GPIO 13 = DMX1 output, GPIO 14 = DMX2 output or DMX1 input
- **Factory reset** — hold GPIO 15 LOW for 3 seconds to erase all saved settings
- **ArduinoOTA** — firmware updates over WiFi

---

## Hardware

| Component | Notes |
|-----------|--------|
| **ESP32** | Any common dev board (e.g. ESP32-WROOM-32) |
| **MAX485 × 1–2** | RS-485 transceivers for DMX ports |
| **Traffic LEDs** | GPIO 13 (DMX1 out), GPIO 14 (DMX2 out or DMX1 in) |
| **Factory reset** | GPIO 15 to GND (hold 3 s) |
| **5 V / 3.3 V** | ESP32 logic; DMX line is separate |

### Port layout

The firmware uses one UART-backed DMX driver (`DMX_NUM_1`) for **DMX1 output**. **DMX2 output** reuses that same driver and switches GPIO to 19/18/21 for each frame (no second esp_dmx port). Port 2 wiring is either **DMX2 output** or **DMX1 input** — never both at once. **DMX1 input** still uses `DMX_NUM_2` and may require the [esp_dmx UART2 patch](#esp_dmx-library-patch-esp32-core-33x). Mode is chosen at boot from the web config.

| Port | Role | ESP32 GPIO (TX / RX / EN) |
|------|------|---------------------------|
| **DMX1 output** | Art-Net → fixtures (always) | 17 / 16 / 4 |
| **DMX2 output** | Art-Net mirror (input disabled) | 19 / 18 / 21 |
| **DMX1 input** | Wired fallback (input enabled) | 19 / 18 / 21 |

Pin assignments are defined in `config.cpp`. Change them there if your board uses different GPIOs.

### Wiring (DMX1 output)

| ESP32 GPIO | MAX485 pin | Function |
|------------|------------|----------|
| GPIO 17 | DI | UART TX → DMX data |
| GPIO 16 | RO | UART RX |
| GPIO 4 | DE + /RE | Driver enable (tied together) |
| GND | GND | Common ground |
| — | A / B | DMX+ / DMX− to fixtures |

### Wiring (second port — DMX2 output or DMX1 input)

Wire a second MAX485 to GPIO 19 (TX), 18 (RX), and 21 (enable). When DMX1 input is enabled in the web UI and the device is rebooted, this port listens for wired DMX instead of outputting.

### Traffic LEDs

| GPIO | Indicates |
|------|-----------|
| **13** | DMX1 output activity |
| **14** | DMX2 output activity, or DMX1 input activity when input mode is enabled |

LEDs blink on each new frame; the web dashboard shows traffic for 2 seconds after the last packet.

---

## Software requirements

Install these libraries through the **Arduino Library Manager** or their GitHub repos:

| Library | Purpose |
|---------|---------|
| [esp_dmx](https://github.com/someweisguy/esp_dmx) **4.x** | DMX512 driver for ESP32 |
| [ArtnetWifi](https://github.com/rstephan/ArtnetWifi) | Art-Net over WiFi |
| **ArduinoOTA** | Included with ESP32 Arduino core |
| **WiFi / WebServer / Preferences** | Included with ESP32 Arduino core |

**Board:** ESP32 Dev Module (or your exact module)  
**ESP32 Arduino core:** 2.x or 3.x (see [esp_dmx patch](#esp_dmx-library-patch-esp32-core-33x) for 3.3.x)

---

## Quick start

### 1. Clone the repository

```bash
git clone https://github.com/your-user/artdmx-bridge32.git
cd artdmx-bridge32
```

### 2. Configure first-boot secrets

Credentials are used as **defaults on first boot** and can later be changed in the web UI. Copy the template and edit your local file:

```bash
cp secrets.h.example secrets.h
```

Edit `secrets.h`:

```cpp
#define SECRETS_WIFI_SSID     "MyNetwork"
#define SECRETS_WIFI_PASS     "MyPassword"
#define SECRETS_OTA_PASSWORD  "MyOtaPassword"
```

`secrets.h` is listed in `.gitignore` and must never be committed.

### 3. Apply esp_dmx patch (if needed)

If compilation fails with `uart_periph_signal[...].module` errors on ESP32 core **3.3.x**, follow [esp_dmx library patch](#esp_dmx-library-patch-esp32-core-33x).

### 4. Upload

1. Open `artdmx-bridge32.ino` in the Arduino IDE.
2. Select your ESP32 board and port.
3. Upload.

On success, Serial Monitor (115200 baud) shows the boot log and WiFi IP.

### 5. Open the web UI

When connected to your network:

- **http://&lt;hostname&gt;.local** (default hostname: `artdmx-bridge32`)
- or **http://&lt;device-ip&gt;**

If WiFi is unavailable for 15 seconds, the device starts a config access point:

- **SSID:** `artdmx-bridge32-setup` (or `<hostname>-setup`)
- Open the AP IP shown in Serial / the dashboard banner

### 6. Send Art-Net

Point your lighting software (QLC+, grandMA onPC, etc.) at the ESP32 IP. Universe must match the value set in **Configuration** (default: universe 0).

---

## Web interface

| Page | URL | Description |
|------|-----|-------------|
| Dashboard | `/` | Status, DMX traffic dots, Art-Net state, log |
| Configuration | `/config` | Edit and save all settings |

### API endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/api/status` | JSON status (traffic, Art-Net, config summary) |
| GET | `/api/log` | Rolling text log |
| GET | `/api/config` | Full configuration JSON |
| POST | `/api/config` | Save configuration (form-urlencoded) |
| POST | `/api/reboot` | Reboot device |

### Configurable settings (stored in NVS)

| Setting | Default | Notes |
|---------|---------|-------|
| WiFi SSID / password | from `secrets.h` | Reboot recommended after change |
| Hostname | `artdmx-bridge32` | Used for mDNS, OTA, and optional web login username |
| OTA password | from `secrets.h` | Empty disables OTA password protection |
| Web password | *(empty)* | When set, HTTP basic auth is required (username = hostname) |
| Art-Net universe | `0` | |
| Art-Net timeout | `5000` ms | Time without Art-Net before input fallback can take over |
| DMX refresh interval | `30` ms | Re-send interval for hold-style dimmers |
| Send full 512-channel packet | on | Off sends only channels up to the active range |
| Enable DMX2 output | off | Mirrors Art-Net to port 2 (GPIO 19/18/21); reboot required |
| Enable DMX1 wired input | off | Disables DMX2 output; **reboot required** |
| Channel filter start/end | `1`–`512` | DMX1 in: forward range to DMX1 out. DMX2 out: send range only when values change |
| Debug Art-Net | off | Logs packets to the web log (verbose at high rates) |

### DMX traffic indicators

The dashboard shows three indicators:

- **DMX1 output** — always visible (green)
- **DMX2 output** — visible when DMX1 input is disabled (blue)
- **DMX1 input** — visible only when DMX1 input is enabled (amber)

### Factory reset

Hold **GPIO 15** LOW for **3 seconds**. All NVS settings are erased and defaults from `config.h` / `secrets.h` are restored on next boot.

---

## Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                            ESP32                                  │
│  ┌──────────────────── Core 0 ──────────────────────────────┐  │
│  │  NetworkTask                                                │  │
│  │    WiFi · ArtNet.read() · WebServer · OTA · reset pin     │  │
│  └──────────────────────────┬─────────────────────────────────┘  │
│                             │ xQueue (depth 1)                     │
│  ┌──────────────────── Core 1 ──────────────────────────────┐  │
│  │  DmxTask                                                    │  │
│  │    DMX1 out (always)                                        │  │
│  │    DMX2 out (when input disabled)                           │  │
│  │    DMX1 in → DMX1 out (when input enabled, Art-Net idle)   │  │
│  └──────────────────────────┬─────────────────────────────────┘  │
└─────────────────────────────┼────────────────────────────────────┘
                              │ RS-485
                              ▼
                         DMX fixtures
```

**Why two tasks?** DMX transmission blocks for roughly 1–23 ms per frame. Handling Art-Net, the web server, and OTA on the same thread would cause dropped packets or failed updates. The Art-Net callback only copies data into a single-slot queue; `DmxTask` owns all timing-critical output.

**DMX2 output:** Mirrors Art-Net to port 2, but only channels inside the filter range (default 1–512). A DMX2 frame is sent only when at least one channel in that range changes; periodic refresh does not re-send DMX2 if the filtered channels are unchanged. DMX1 output always receives the full Art-Net frame.

**DMX1 input behavior:** When enabled, port 2 receives wired DMX only while Art-Net is inactive (past the configured timeout). Channels outside the filter range are zeroed; filtered channels are sent to DMX1 output only.

---

## Project structure

```
artdmx-bridge32/
├── artdmx-bridge32.ino      # setup() / loop()
├── config.h / config.cpp    # Pin map, task layout, compile-time defaults
├── device_config.h / .cpp   # NVS-backed runtime configuration
├── secrets.h                # First-boot WiFi + OTA defaults (local, gitignored)
├── secrets.h.example        # Template for secrets.h
├── types.h                  # DmxFrame struct
├── globals.h / globals.cpp  # ArtNet instance, queue, traffic timestamps
├── wifi_manager.*           # WiFi connect, AP fallback, mDNS
├── web_server.*             # Dashboard, config UI, REST API
├── ota_manager.*            # ArduinoOTA setup
├── device_log.*             # Rolling log buffer for web UI
├── dmx_output.*             # DMX1 output (DMX_NUM_1)
├── dmx_output2.*            # DMX2 output (DMX_NUM_2)
├── dmx_input.*              # DMX1 input on port 2
├── artnet_handler.*         # Art-Net callback and init
├── led.*                    # Traffic LEDs and activity tracking
├── tasks.*                  # networkTask, dmxTask
├── uart.c.txt               # Reference patch notes for esp_dmx
├── .gitignore
└── README.md
```

---

## OTA updates

After the device is on WiFi:

1. In the Arduino IDE: **Tools → Port** → network port `artdmx-bridge32 at …`
2. Upload as usual; enter the OTA password when prompted (from web config or `secrets.h`).

Hostname matches the configured `hostname` field (default `artdmx-bridge32`).

---

## esp_dmx library patch (ESP32 Core 3.3.x)

Recent ESP32 Arduino cores removed `uart_periph_signal[n].module`. The stock **esp_dmx 4.1.0** `uart.c` may not compile until patched. The **DMX1 wired input** mode uses `DMX_NUM_2` and needs the UART2 context fix below. **DMX2 output does not** — it shares `DMX_NUM_1` with pin switching.

**File to edit** (adjust path to your machine):

```
~/Arduino/libraries/esp_dmx/src/dmx/hal/uart.c
```

**Steps** (summary; see `uart.c.txt` in this repo for the full reference):

1. After the existing `#include` block, add:

```c
#include "esp_idf_version.h"
#include "soc/periph_defs.h"
#include "soc/soc_caps.h"
#include "esp_private/periph_ctrl.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
static periph_module_t dmx_uart_get_module(int dmx_num) {
  switch (dmx_num) {
    case 0: return PERIPH_UART0_MODULE;
    case 1: return PERIPH_UART1_MODULE;
    case 2: return PERIPH_UART2_MODULE;
    default: return PERIPH_UART0_MODULE;
  }
}
#define DMX_UART_MODULE(num) dmx_uart_get_module(num)
#else
#define DMX_UART_MODULE(num) uart_periph_signal[num].module
#endif
```

2. Replace all `periph_module_enable/disable/reset(uart_periph_signal[...].module)` calls with `DMX_UART_MODULE(...)`.

3. **UART2 context (required for DMX2 output):** in the `dmx_uart_context` initializer, the third UART entry must use `#if SOC_UART_NUM > 2` — **not** `#if DMX_NUM_MAX > 2` (`DMX_NUM_MAX` is an enum and does not work in `#if`, so UART2 is silently omitted and `DMX_NUM_2` crashes at runtime):

```c
} dmx_uart_context[DMX_NUM_MAX] = {
    {.num = 0, .dev = UART_LL_GET_HW(0)},
    {.num = 1, .dev = UART_LL_GET_HW(1)},
#if SOC_UART_NUM > 2
    {.num = 2, .dev = UART_LL_GET_HW(2)},
#endif
};
```

4. Clear the Arduino build cache after patching (otherwise old object files may be reused).

**Alternatives:** use an older ESP32 core (2.x) or a fork of esp_dmx that already includes both fixes.

---

## Troubleshooting

| Symptom | Things to check |
|---------|------------------|
| Reboot loop with **DMX1 input** enabled | **esp_dmx UART2 bug** — patch `uart.c` (`#if SOC_UART_NUM > 2`, not `DMX_NUM_MAX`) ([patch section](#esp_dmx-library-patch-esp32-core-33x)) |
| No DMX output | Wiring, DE pin, Art-Net universe, lighting software output enabled |
| Only one output works | DMX1 input mode disables DMX2 output — check **Configuration** |
| Wired input ignored | Enable DMX1 input, reboot, wait for Art-Net timeout, check filter range |
| Compile error in `uart.c` | Apply [esp_dmx patch](#esp_dmx-library-patch-esp32-core-33x) |
| WiFi connects but no Art-Net | Firewall, same subnet, correct universe |
| Cannot open web UI | Try device IP; if offline, join `<hostname>-setup` AP |
| Flickering fixtures | Lower DMX refresh interval; enable full 512-channel packets |
| OTA fails | OTA password, device and PC on same network |
| Log floods quickly | Disable **Debug Art-Net** unless diagnosing |
| Settings won't stick | Check save confirmation; factory reset clears NVS |

---

## Configuration reference

### `secrets.h` (private, first-boot only)

| Macro | Description |
|-------|-------------|
| `SECRETS_WIFI_SSID` | Default WiFi network name |
| `SECRETS_WIFI_PASS` | Default WiFi password |
| `SECRETS_OTA_PASSWORD` | Default OTA password (`""` disables protection) |

After first save via the web UI, values live in NVS. Editing `secrets.h` alone does not change a device that already has stored config unless you factory-reset.

### `config.h` (compile-time defaults)

| Symbol | Default | Description |
|--------|---------|-------------|
| `DEVICE_NAME` | `artdmx-bridge32` | Firmware identity string |
| `VERSION` | `2.3.0-web-dmx` | Version string |
| `DEFAULT_HOSTNAME` | `artdmx-bridge32` | First-boot hostname |
| `DEFAULT_ARTNET_UNIVERSE` | `0` | First-boot Art-Net universe |
| `DEFAULT_ARTNET_TIMEOUT_MS` | `5000` | Art-Net idle timeout |
| `DEFAULT_DMX_REFRESH_MS` | `30` | DMX re-send period |
| `DEFAULT_SEND_FULL_PACKET` | `1` | Send full 512-slot frames |
| `DEFAULT_ENABLE_DMX_INPUT` | `0` | DMX1 wired input off at first boot |
| `DEFAULT_ENABLE_DMX2_OUTPUT` | `0` | DMX2 output off at first boot (safe without esp_dmx UART2 patch) |
| `DEFAULT_DMX2_FILTER_START/END` | `1` / `512` | DMX1 input channel filter |
| `DEFAULT_DEBUG_ARTNET` | `0` | Art-Net debug logging off |

### `config.cpp` (hardware pins)

| Symbol | GPIO | Description |
|--------|------|-------------|
| `LED_DMX1_PIN` | 13 | DMX1 output traffic LED |
| `LED_DMX2_PIN` | 14 | DMX2 output / DMX1 input traffic LED |
| `RESET_PIN` | 15 | Factory reset (hold LOW 3 s) |
| `DMX_OUT_TX/RX/EN` | 17/16/4 | DMX1 output |
| `DMX2_OUT_TX/RX/EN` | 19/18/21 | DMX2 output (or DMX1 input) |

### NVS namespace

All runtime settings are stored under the Preferences namespace `artdmx-bridge32`.

---

## License

Copyright © 2026 Richard Smetana.

Licensed under the **GNU General Public License v3 or later**. See [LICENSE](LICENSE) for the full text.

---

## Contributing

Contributions are welcome — bug reports, documentation fixes, hardware notes, and pull requests.

1. **Fork** the repository and create a feature branch from `main`.
2. **Keep secrets local** — do not commit `secrets.h` or other credentials.
3. **Match the style** — follow existing naming, file layout, and Arduino/ESP32 conventions in this project.
4. **Test on hardware** when changing DMX, Art-Net, or WiFi behaviour; note your board and ESP32 core version in the PR.
5. **Open a pull request** with a short description of what changed and why.

If you are porting ideas from other Art-Net/DMX projects (including the original Connotron gateway), please credit the source in your PR or commit message where appropriate.

---

## Acknowledgments

### Inspiration

This project was inspired by **[Connotron DMX Gateway](https://github.com/chaosloth/Connotron_DMX_Gateway)** ([chaosloth](https://github.com/chaosloth) / Christopher Connolly) — an ESP32 Art-Net to DMX512 gateway with WiFi setup portal, web OTA, PCB/enclosure designs, and proven MAX485 pinouts. Several hardware choices in **artdmx-bridge32** follow that lineage, including:

- DMX1 output on GPIO **17** (TX), **16** (RX), **4** (enable)
- DMX input / second port on GPIO **19** (TX), **18** (RX), **21** (enable)
- Configuration / factory-reset pin on GPIO **15**

**artdmx-bridge32** is not a direct fork; it is a separate rewrite that adds NVS-backed web configuration, dual DMX outputs, traffic LEDs, a dashboard log, and a dual-core FreeRTOS architecture.

### Libraries

- [esp_dmx](https://github.com/someweisguy/esp_dmx) by someweisguy (successor to the esp_dmx library used in the original Connotron project)
- [ArtnetWifi](https://github.com/rstephan/ArtnetWifi) by rstephan
