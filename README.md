# VentureD Board C shortcut demo

This repository contains a deliberately small, offline-first hardware hackathon
demo. Board C firmware sends one versioned physical-input event per serial line; the
Companion maps the three fixed inputs to locally configured keyboard chords.

The project is not a flashing guide and does not claim physical hardware or
Windows input-injection acceptance from its offline checks.

## Three-stage workflow

### 1. Development Debug: browser fixture only

This is the ordinary software-development path. It starts a typed browser
fixture instead of the native host, so it does not build firmware or a native
program and cannot enumerate/open a COM port, capture a foreground target,
write an application profile, or call `SendInput`.

```powershell
pnpm --dir companion dev:fixture
pnpm --dir companion check:software
pnpm --dir companion test:interaction
```

`dev:fixture` launches Vite with `VITE_COMPANION_FIXTURE=true` from the local
fixture environment. `check:software` runs only lint, TypeScript type-checking,
and Vitest. `test:interaction` starts that fixture and runs the Playwright
interaction/visual regression test. `pnpm --dir companion dev` is an alias for
the same fixture workflow.

### 2. Native Integration: explicitly later

Only after an explicit integration decision may an operator use native Tauri
commands such as `pnpm --dir companion tauri dev` or
`pnpm --dir companion tauri build`. Native compilation/liveness is distinct
from, and does not authorize, real application-config writes, foreground
capture, COM enumeration/open, or `SendInput`.
Tauri's development command uses the internal `dev:native` Vite bridge; the
ordinary `dev` command remains an alias of the browser fixture.

### 3. Hardware Acceptance: separately authorized

Install ESP-IDF **5.5.4** and export its environment. An ESP-IDF build is a
separate hardware-acceptance preparation step, not part of Development Debug:

```powershell
idf.py -C firmware/board-c-demo set-target esp32s3
idf.py -C firmware/board-c-demo -B <fresh-build-dir> -D SDKCONFIG=<fresh-sdkconfig> build
```

Flash, reset, monitor, COM selection, and HIL each require a separate fresh
authorization for the actual board, selected port, candidate image/commit,
write regions, and reset/monitor permissions. They are intentionally not part
of the commands above.

## Confirmed Board C wiring

- Rotary encoder: `ENA`/`CLK` -> GPIO6 and `ENB`/`DT` -> GPIO7.
- The encoder `SW` lead is **unconnected** in this demo.
- `ENCODER_PRESS` is retained as the protocol ID for an independent external
  three-pin pull-up confirm/action button: `SIG` -> GPIO8, with 3.3 V and a
  common GND. The expected signal is idle HIGH and pressed LOW.
- GPIO6/GPIO7 conflict with camera use, and GPIO8 conflicts with DHT11 use;
  those peripherals cannot be enabled alongside this wiring without a new pin
  allocation decision.

After selecting the intended serial port and saving the profile, start the
Companion in dry-run mode. The production dry-run reads that selected port and
resolves mappings but never constructs an input dispatcher. The browser fixture
tests the same UI flow with fakes and never opens a real port.

In live mode, each encoder rotation or GPIO8 press first restores the captured
foreground window (already running, exact executable path), then sends one of
the three configured shortcuts. Codex session/Hook inspection is not part of
this first demo.

## Scope and future seam

The window contains a serial selection, a collapsed **设置** panel for Wi-Fi
and the SiliconFlow API key, a captured foreground target, and the three fixed
physical-input rows. Secrets stay in `device.json`, not the shortcut profile.
ESP32-S3 station mode is 2.4 GHz only; a 5 GHz SSID is rejected with a visible
warning instead of staying on 连接中.
Live dispatch is process-local, defaults to off after every restart, and must
be explicitly enabled only after a valid profile is loaded.

Future voice work remains a documented boundary only:
`microphone capture -> audio transport -> speech recognizer -> text/Agent ->
guarded text insertion`. No ASR provider, credentials, microphone transport,
or text insertion belongs to this demo.

See [the Board C guide](docs/board-c-demo.md), [the event contract](protocol/input-event-v1.md),
[the device-link contract](protocol/device-link-v1.md),
and [source provenance](docs/source-provenance.md).
