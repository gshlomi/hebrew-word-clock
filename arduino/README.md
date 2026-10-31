# Hebrew Word Clock — Seeed XIAO 7.5-inch ePaper Panel

Targets [this integrated Seeed panel](https://www.seeedstudio.com/XIAO-7-5-ePaper-Panel-p-6416.html): **XIAO ESP32-C3, 4 MB flash, 800×480 monochrome, UC8179**. This is not the separate ePaper breakout board, an ESP32-S3 panel, or an arbitrary 7.5-inch display.

## Build and upload (recommended: PlatformIO)

Install [PlatformIO](https://platformio.org/install) (CLI validated with version 6.2.0), then from the repository root:

1. Copy `arduino/hebrew_clock/secrets.example.h` to `arduino/hebrew_clock/secrets.h`.
2. Set your **2.4 GHz** WiFi SSID and password in `secrets.h` (git-ignored; do not commit credentials). Optionally set `CLOCK_TIMEZONE` there as described below.
3. Connect the panel using a USB-C **data** cable and turn its power switch on.
4. Run:

```sh
pio run
pio run --target upload --upload-port /dev/ttyACM0
pio device monitor --port /dev/ttyACM0 --baud 115200
```

Replace the port with your device's port (e.g. `COM5` on Windows). If upload cannot connect, hold BOOT, press/release RESET, then release BOOT and reconnect USB. See [Seeed's recovery instructions](https://wiki.seeedstudio.com/xiao_075inch_epaper_panel/#programming-buttons).

The build pins Espressif32 **6.12.0** (Arduino ESP32 **2.0.17**) and Seeed_GFX commit `0dfdd7135425be82b5bb4b4b58d74dd51ab29a59`. No SPIFFS upload, separate font upload, OpenFontRender or NTPClient is required. A build without `secrets.h` uses placeholders and cannot connect until you configure real credentials.

## Timezone configuration

The default is **Israel time**, with automatic daylight saving. To choose another
timezone, add a `CLOCK_TIMEZONE` setting to `arduino/hebrew_clock/secrets.h`, for example:

```cpp
#define CLOCK_TIMEZONE "UTC0"
```

Use a **POSIX TZ rule**, not an IANA name such as `Asia/Jerusalem`:

| Location | `CLOCK_TIMEZONE` value |
|---|---|
| Israel (default) | `"IST-2IDT,M3.4.4/26,M10.5.0"` |
| UTC | `"UTC0"` |
| New York | `"EST5EDT,M3.2.0/2,M11.1.0/2"` |
| Berlin | `"CET-1CEST,M3.5.0/2,M10.5.0/3"` |

POSIX offset signs are reversed: `CET-1` means UTC+1. Rules with DST apply it
automatically. Legal timezone changes may require updating the rule.
`CLOCK_TIMEZONE` is the initial/default POSIX rule. Changing that build default
requires rebuilding and uploading. For everyday changes, use the webpage below;
saved runtime settings take precedence and persist across reboot and ordinary uploads.
Hebrew wording is unchanged; only the local hour and minute change.

### Runtime settings webpage

After the initial USB firmware upload, open `http://<device-ip>/`:

1. Choose an IANA location such as **Asia/Jerusalem** from the timezone dropdown.
2. Set **Enable remote syslog**, server IPv4 and UDP port as desired.
3. Click **Save settings**. The complete settings record is stored in ESP32 NVS
   flash, checked for write success, and applied without reboot or rebuild.

Timezone changes trigger a display refresh when the display task is available;
an already-running or stuck ePaper update cannot be interrupted. Syslog changes
apply to subsequent queue processing. Disabling forwarding keeps the address and
port for later re-enabling, and does not disable local diagnostics or logs.
Status polling does not overwrite unsaved form edits. Invalid settings are rejected.

The offline IANA dropdown uses the bundled MIT-licensed
[nayarsystems/posix_tz_db](https://github.com/nayarsystems/posix_tz_db) mapping.
**Firmware default** selects the compiled `CLOCK_TIMEZONE` rule. IANA selections
are translated to POSIX on the device, and both the name and rule are saved.
This is not a full historical timezone database. Future law changes require a
mapping/firmware update and re-saving the location; there is no automatic online
mapping update. See `third_party/posix_tz_db/README.md` for regeneration.

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
- ESP32 system SNTP supplies UTC; the configured POSIX timezone rule supplies local time and DST (Israel by default). The default rule is checked against IANA `Asia/Jerusalem` for 2025–2035; future legal timezone changes may require an update.
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
python arduino/tools/test_settings.py
pio run
```

The generator needs a host C++ compiler with AddressSanitizer/UndefinedBehaviorSanitizer and Pillow with RAQM. It compiles the **same** `time_words.h` used by firmware, checks all minute layouts, then writes `word_bitmaps.h` and the preview. The bundled [Noto Serif Hebrew](https://github.com/google/fonts/tree/main/ofl/notoserifhebrew) is licensed under [SIL OFL](fonts/OFL.txt). Generated assets retain this license. CI repeats asset checks and the ESP32-C3 build.

## Hardware acceptance test (still required)

Compilation and offline previews cannot prove physical panel behavior. On the actual unit, verify:

1. The boot/status text is black-on-white, upright, and not mirrored; no BUSY timeout appears.
2. After WiFi/NTP sync, the displayed time matches your configured timezone, with correct RTL and nikud.
3. A minute change refreshes once, without cropped text or persistent ghosting.
4. Booting with WiFi unavailable leaves a status message; restoring WiFi recovers automatically.
5. Review the serial free-heap reading and measure battery runtime if battery operation is required.

## Local build validation

Build/render/timezone checks verified 2026-10-31 using the pinned dependencies:

- ESP32-C3 build: **PASS**; flash **1,094,706 / 1,310,720 bytes (83.5%)** of the default application partition. The board has 4 MB total flash; no larger partition is needed.
- Static RAM: **48,500 / 327,680 bytes (14.8%)**. The display also allocates a **48,000-byte framebuffer on the heap**; static RAM statistics do not include WiFi/runtime allocations, the 6 KB diagnostic task stack, or its 24-record queue. Physical free-heap testing remains required.
- Actual firmware renderer, executed with a bounds-checking host display stub under ASan/UBSan: **1,440 minute layouts + 2 status messages pass**.
- Israel timezone rule: **192,816 half-hour samples** match IANA for 2025–2035.
- Four representative previews visually inspected for RTL/nikud. Hardware is **not yet tested**.
- Upstream build warnings about unavailable C3 DMA and missing touch CS are expected; neither feature is used.

## Wi-Fi diagnostics and remote syslog

After uploading this firmware by USB, find **hebrew-clock** in your router's DHCP
clients and open `http://<device-ip>/`. The hostname is a DHCP label, not a promise
that `hebrew-clock.local` resolves. The status area refreshes every five seconds;
`/status` returns plain-text diagnostics and `/logs` downloads the last 48 log
records. It shows uptime, numeric ESP-IDF reset reason, local time/sync validity,
Wi-Fi/IP/RSSI, current/minimum heap, display-update start/return times, and syslog
counters. Update return means the driver returned, not that the panel physically
changed. Logs include boot, connection transitions, initial time synchronization,
refresh boundaries, application errors, and a heartbeat every minute.

HTTP, Wi-Fi reconnection and UDP forwarding run in a separate FreeRTOS task,
started **before display initialization**. Display code only appends bounded log
records and queues them without waiting for network I/O. This can keep diagnostics
alive during a blocked display call; it cannot survive a whole-chip hang, disabled
interrupts/scheduler, power loss, or loss of Wi-Fi. The C3 is single-core. HTTP is
single-client and a slow client can delay syslog in the diagnostic task, but not
block the display task. A stale-data warning appears if browser polling fails.

### Send logs to a syslog server

Use the webpage toggle, IPv4 address and port, then Save. Alternatively, set initial
defaults in your local, git-ignored `arduino/hebrew_clock/secrets.h`:

```cpp
#define CLOCK_HOSTNAME "hebrew-clock"       // short ASCII hostname, no spaces
#define CLOCK_SYSLOG_IP "192.168.1.50"      // replace with server's IPv4 address
#define CLOCK_SYSLOG_PORT 514               // UDP listener port
```

Rebuild and upload to change compile-time defaults; saved webpage settings take
precedence. Syslog is disabled initially (empty address); only numeric
IPv4 is supported. Configure the server to accept **UDP**, with RFC5424 messages,
facility **local0**, application **hebrew-clock**. Packets use a NILVALUE timestamp
and include device uptime in milliseconds; use the receiver's arrival timestamp
for wall-clock correlation, including during NTP failures. For rsyslog, the usual
UDP input is `module(load="imudp")` and `input(type="imudp" port="514")`; merge
with existing server configuration rather than adding duplicate listeners.

Forwarding is best effort: UDP has no acknowledgement, and a locally sent packet
is not proof of receipt. Offline records stay only in the recent RAM ring; they
are not replayed after reconnect. A 24-record transmit queue bounds memory and
reports overflow. Logs and refresh markers reset at reboot; reset reason is
reported again on network connection. Library-internal Serial output is **not**
captured/forwarded; use USB serial for driver-specific BUSY messages. No flash
logging is performed; only settings are persisted to flash.

This is an unauthenticated HTTP page and unencrypted syslog for a trusted LAN;
do not port-forward it to the Internet. No Wi-Fi password is exposed by the page. Settings POSTs require a per-boot token
and custom header to reject cross-origin form submissions; this is not login authentication.

### Hardware verification after upload

1. Save a timezone and syslog configuration, reboot and confirm persistence.
   Disable/re-enable syslog and verify receiver traffic stops/resumes while local logs continue.
   Confirm the page is reachable and uptime advances while the display refreshes.
2. Confirm the server receives connection/heartbeat and refresh start/return logs.
3. Leave running beyond the reported ~90-minute failure window. If the panel
   freezes, save `/status` and `/logs` before resetting it; compare the last
   refresh start/return and continuing heartbeats in remote logs.
4. Temporarily interrupt Wi-Fi, restore it and verify reconnection/page/syslog
   recovery. The display should continue using system time after initial sync.
5. A controlled simulated display-task stall (with scheduler still running)
   should leave HTTP/heartbeats responsive; whole-chip hangs cannot be covered.

These on-device/network checks remain required; a successful compile and offline
renderer tests do not prove runtime behavior or syslog delivery.
