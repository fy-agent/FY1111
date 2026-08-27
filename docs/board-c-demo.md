# Board C demo guide

## Pin ownership

The pin manifest in `main/board_c_pins.h` is the single source of truth.

| Resource | Pins | State |
| --- | --- | --- |
| Rotary encoder | ENA/CLK 6, ENB/DT 7; SW unconnected | enabled |
| External confirm/action button | pull-up SIG 8; 3.3 V and common GND | enabled |
| Digital microphone | WS 42, SD 2, SCK 41 | reserved, disabled |
| ST7789 LCD | SCL 21, SDA 47, DC 43, CS 44 | status screen: Chinese network state |
| WS2812 | 48 | reserved, disabled |
| Optional PTT button | 12 | reserved, disabled |
| Camera and SD-card resources | Board-documented resources | reserved; not allocated by this demo |

The exact module flash and PSRAM configuration has not been identified, so the
defaults intentionally do not assume a VibeKey N16R8-like module.

The external button is active-low: GPIO8 is expected to be HIGH while idle and
LOW while pressed. It produces the fixed `ENCODER_PRESS` protocol ID, but it is
not an integrated encoder switch. GPIO6/GPIO7 conflict with camera use and
GPIO8 conflicts with DHT11 use, so camera or DHT11 enablement needs a separate
pin-allocation decision.

## Firmware

Use ESP-IDF 5.5.4. The adapter configures GPIO6/GPIO7 for rotary samples without enabling
GPIO8 interrupts, queues ISR samples, scans the independent GPIO8 button on a
10 ms cadence, emits only semantic `VKEY_INPUT/1` records on the ESP32-S3
USB-Serial-JTAG console, and owns no shortcut mapping.
The `VKEY_INPUT/1` record is documented in `protocol/input-event-v1.md`.
Host-to-device Wi-Fi/API configuration and device-to-host network status are
documented in `protocol/device-link-v1.md`. The Companion stores those secrets
in `device.json`, not in the shortcut profile. ESP32-S3 station mode is
2.4 GHz only: a 5 GHz SSID must fail closed with `reason=BAND` instead of
staying on `CONNECTING`.

The saved Companion profile contract is versioned and nested: `{"version":1,
"revision":"sha256:...","serial":{"port":"<selected port>","baud":115200},
"target":...,"mappings":[...]}`. Flat `serialPort`/`baud` fields and unknown
profile fields are rejected.

`idf.py ... build` is offline build evidence. Do not combine it with `flash`,
`erase-flash`, or `monitor`. Those operations require a new explicit approval
for the actual board, candidate image, port, write region, reset, and monitor.

## Companion setup

Choose a port from the host list, capture the intended foreground program after
the visible three-second delay, configure the three fixed mapping rows, and
save. The Chinese UI labels `ENCODER_PRESS` as the independent **GPIO8
external confirm/action button**. Duplicate/incomplete chords block save.
Start with **dry-run**, which reports a resolved event without creating a
dispatcher. Live permission is visibly separate, session-only, and starts
disabled after every application restart. In live mode, each encoder rotation
or GPIO8 press first restores the already-running captured window, rechecks
exact foreground identity, then sends the mapped shortcut. Codex Hook/Observer
inspection is out of scope for this first demo.

## Development and acceptance boundaries

For ordinary Development Debug, run `pnpm --dir companion dev:fixture`,
`pnpm --dir companion check:software`, and
`pnpm --dir companion test:interaction`. These run the browser fixture only:
they do not build native/firmware programs or perform real I/O.

Native Integration is explicitly later and uses Tauri commands only after that
decision; it still does not itself authorize app-config writes, foreground
capture, COM access, or `SendInput`. Hardware Acceptance is a separately
authorized ESP-IDF build/flash/monitor/HIL stage. See the repository README
for the exact three-stage commands and authorization boundaries.

## Evidence boundary

Host/Cargo/React/browser checks prove parsing, validation, persistence against
temporary files, layout, and dry-run behaviour only. They do not prove a COM
port, physical encoder, flash, foreground capture, `SendInput`, native
shortcut effect, or any future microphone/Agent workflow.
