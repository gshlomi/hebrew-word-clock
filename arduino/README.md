# Hebrew Word Clock — Seeed XIAO 7.5-inch ePaper Panel

Targets [this integrated Seeed panel](https://www.seeedstudio.com/XIAO-7-5-ePaper-Panel-p-6416.html): **XIAO ESP32-C3, 4 MB flash, 800×480 monochrome, UC8179**. This is not the separate ePaper breakout board, an ESP32-S3 panel, or an arbitrary 7.5-inch display.

## Build and upload (recommended: PlatformIO)

Install [PlatformIO](https://platformio.org/install) (CLI validated with version 6.2.0), then from the repository root:

1. Copy `arduino/hebrew_clock/secrets.example.h` to `arduino/hebrew_clock/secrets.h`.
2. Set your **2.4 GHz** WiFi SSID and password in `secrets.h` (git-ignored; do not commit credentials).
3. Connect the panel using a USB-C **data** cable and turn its power switch on.
4. Run:

```sh
pio run
pio run --target upload --upload-port /dev/ttyACM0
pio device monitor --port /dev/ttyACM0 --baud 115200
```

Replace the port with your device's port (e.g. `COM5` on Windows). If upload cannot connect, hold BOOT, press/release RESET, then release BOOT and reconnect USB. See [Seeed's recovery instructions](https://wiki.seeedstudio.com/xiao_075inch_epaper_panel/#programming-buttons).

The build pins Espressif32 **6.12.0** (Arduino ESP32 **2.0.17**) and Seeed_GFX commit `0dfdd7135425be82b5bb4b4b58d74dd51ab29a59`. No SPIFFS upload, separate font upload, OpenFontRender or NTPClient is required. A build without `secrets.h` uses placeholders and cannot connect until you configure real credentials.

## Arduino IDE alternative

Install Espressif's **esp32 2.0.17** board package and select **XIAO_ESP32C3** / **XIAO ESP32C3**, 4 MB flash, default partition scheme. Install the [pinned Seeed_GFX ZIP](https://github.com/Seeed-Studio/Seeed_GFX/archive/0dfdd7135425be82b5bb4b4b58d74dd51ab29a59.zip). Avoid a simultaneously installed TFT_eSPI library: both expose `TFT_eSPI.h`. Open `hebrew_clock/hebrew_clock.ino`, configure `secrets.h`, select the USB port and upload. The CLI build is the automated verification path; the IDE flow is not separately tested.

## Exact display configuration and pin mapping

`driver.h` selects Seeed's official configuration:

```cpp
#define BOARD_SCREEN_COMBO 502
#define USE_XIAO_EPAPER_DRIVER_BOARD
```

The library configures SPI and all control pins. **No manual rewiring is needed** on the integrated product. XIAO pin labels are not GPIO numbers:

| Signal | XIAO label | ESP32-C3 GPIO |
|---|---|---|
| SCK | D8 | 8 |
| MOSI / DIN | D10 | 10 |
| CS | D1 | 3 |
| DC | D3 | 5 |
| BUSY | D2 | 4 |
| RESET | D0 | 2 |

D9 / GPIO9 is the library's SPI MISO assignment; this write-only display does not need a MISO data connection. `setRotation(0)` keeps native **800×480** coordinates.

Sources: [Seeed Arduino guide](https://wiki.seeedstudio.com/xiao_075inch_epaper_panel_arduino/), [preset 502](https://github.com/Seeed-Studio/Seeed_GFX/blob/0dfdd7135425be82b5bb4b4b58d74dd51ab29a59/User_Setups/Setup502_Seeed_XIAO_EPaper_7inch5.h), [driver-board pins](https://github.com/Seeed-Studio/Seeed_GFX/blob/0dfdd7135425be82b5bb4b4b58d74dd51ab29a59/User_Setups/EPaper_Board_Pins_Setups.h).

## Hebrew, layout, and timekeeping

- Full Hebrew words, including nikud, are shaped **at asset-generation time** using Pillow/libraqm (HarfBuzz/FriBidi) and bundled Noto Serif Hebrew. Firmware draws the resulting 1-bit word images in RTL order, wrapping and centering lines within 720 pixels. It does not reverse UTF-8 bytes or separate combining marks.
- All **1,440 minute strings** and both status messages are checked for clipping and missing words. The generator selects the largest shared font that fits every phrase within the 40-pixel margins. Current font: **98 px** (previously 64 px). Current assets: **76 words, 255,432 bitmap bytes**, at most three lines. See [layout-preview.png](layout-preview.png).
- The original Hebrew time wording is retained. The browser version is unchanged.
- ESP32 system SNTP supplies UTC; a POSIX Israel timezone rule applies DST automatically. The rule is checked against IANA `Asia/Jerusalem` for 2025–2035; future legal timezone changes may require an update.
- Before the first successful sync, the panel shows “ממתין לשעה” (waiting for time), or “אין חיבור” (no connection). It never treats uptime as wall-clock time. WiFi reconnects and SNTP retries in the background.
- After initial sync, the system clock continues through temporary WiFi outages, with normal oscillator drift. A reboot without network cannot restore accurate time.
- One full refresh per minute; Seeed_GFX sleeps the **display** after each refresh. The **ESP32 remains awake** for WiFi and timekeeping. This is a USB-powered-first build: do not expect Seeed's advertised multi-month deep-sleep battery life at this refresh rate. Partial refresh/deep-sleep optimization is not implemented.

## Regenerate assets and run verification

Normal firmware builds use the committed bitmaps. To change wording/font/layout:

```sh
python -m pip install pillow==13.0.0
python arduino/tools/generate_words.py
python arduino/tools/test_render.py
python arduino/tools/test_timezone.py
pio run
```

The generator needs a host C++ compiler with AddressSanitizer/UndefinedBehaviorSanitizer and Pillow with RAQM. It compiles the **same** `time_words.h` used by firmware, checks all minute layouts, then writes `word_bitmaps.h` and the preview. The bundled [Noto Serif Hebrew](https://github.com/google/fonts/tree/main/ofl/notoserifhebrew) is licensed under [SIL OFL](fonts/OFL.txt). Generated assets retain this license. CI repeats asset checks and the ESP32-C3 build.

## Hardware acceptance test (still required)

Compilation and offline previews cannot prove physical panel behavior. On the actual unit, verify:

1. The boot/status text is black-on-white, upright, and not mirrored; no BUSY timeout appears.
2. After WiFi/NTP sync, the displayed time matches Israel time, with correct RTL and nikud.
3. A minute change refreshes once, without cropped text or persistent ghosting.
4. Booting with WiFi unavailable leaves a status message; restoring WiFi recovers automatically.
5. Review the serial free-heap reading and measure battery runtime if battery operation is required.

## Local build validation

Verified 2026-10-29 using the pinned dependencies:

- ESP32-C3 build: **PASS**; flash **899,710 / 1,310,720 bytes (68.6%)** of the default application partition. The board has 4 MB total flash; no larger partition is needed.
- Static RAM: **39,148 / 327,680 bytes (11.9%)**. The display also allocates a **48,000-byte framebuffer on the heap**; static RAM statistics do not include WiFi/runtime allocations. Physical free-heap testing remains required.
- Actual firmware renderer, executed with a bounds-checking host display stub under ASan/UBSan: **1,440 minute layouts + 2 status messages pass**.
- Israel timezone rule: **192,816 half-hour samples** match IANA for 2025–2035.
- Four representative previews visually inspected for RTL/nikud. Hardware is **not yet tested**.
- Upstream build warnings about unavailable C3 DMA and missing touch CS are expected; neither feature is used.
