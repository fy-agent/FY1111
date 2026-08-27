# Technical Design

## 1. Scope Decision

Build a new minimal competition application instead of copying the complete
VibeKey product stack. Reuse only behavior that directly supports the first
demo:

```text
Board C GPIO6/GPIO7 encoder + GPIO8 action button
  -> versioned serial event
  -> compact Tauri Companion
  -> profile lookup
  -> foreground guard
  -> dry-run or guarded Windows shortcut dispatch
```

The Companion includes a purpose-built single-window configuration interface.
It does not include VibeKey Setup, Codex Hook/Observer, WorkBuddy discovery,
application catalogs, provisioning, MSC, BLE, or cloud/Agent screens.

## 2. Repository Boundaries

```text
VentureD/
├── firmware/
│   └── board-c-demo/
│       ├── CMakeLists.txt
│       ├── sdkconfig.defaults
│       ├── sdkconfig.board-c.defaults
│       ├── main/
│       │   ├── CMakeLists.txt
│       │   ├── Kconfig.projbuild
│       │   ├── app_main.c
│       │   ├── board_c_pins.h
│       │   ├── input/
│       │   └── protocol/
│       └── host_tests/
├── companion/
│   ├── package.json
│   ├── pnpm-lock.yaml
│   ├── src/                 # React single-window UI
│   ├── tests/               # Vitest and browser visual checks
│   └── src-tauri/
│       ├── Cargo.toml
│       └── src/             # profile, serial, target, runtime, SendInput
├── protocol/
│   └── input-event-v1.md
├── docs/
│   ├── source-provenance.md
│   └── board-c-demo.md
└── README.md
```

The current repository owns every runtime file. No Cargo path dependency,
frontend import, script, documentation command, or runtime configuration may
refer back to `vibekey_new`.

## 3. Source Reuse and Provenance

Reuse is selective and adapted; source files are never modified in place.
`docs/source-provenance.md` will record source commit, source path, pre-copy
SHA-256, destination path, adaptation summary, and verification status.

Initial candidates from source commit
`a2ef7cb00af8cb47460921741bbf13e80119df1a`:

| Source responsibility | Source SHA-256 | Destination responsibility |
| --- | --- | --- |
| EC11 pure core `.c` | `CB08D8838F317C2633B3739FFD61E443DCA29CCE299512D627780292C81B66B6` | Preserve Gray-code and debounce semantics |
| EC11 pure core `.h` | `E993748178E881236351273191232B9E89B7D5A3ABFB794E31226715CE035FFA` | Preserve pure API with local naming only where required |
| EC11 event enum | `07E882212BDF6066F8BB92F315257F2CD47B27C2E30F0BF8FC7E0EE518B1597B` | Three semantic encoder inputs |
| Host input parser | `2DA72EB96956F5BB35EE158FBB26951C514B5CC1D435D7EF0F4BC99D4791DA0C` | Replace log scraping with v1 machine event decoding |
| Bounded serial reader | `4102FCCA72555A7FA9E253C5BE0DD64BDA656C2B3970028AE1E2B80E662E9F01` | Preserve bounded UTF-8 line handling |
| Guarded shortcut dispatcher | `B95063FD36C3F5B3FE372240F11E650F01D75D8ECB80802F4C24B43DC599887E` | Preserve clean-key-state and SendInput boundary |
| Foreground probe | `9F93B9EE54B02801148541C35EA09DDF82152D65EFB0347028A8751E8292DA34` | Strict target process identity |
| Foreground decision core | `F517D4186256B74800EC671A509B08DE79D337F880B1497993C99585345DD868` | Fail-closed name/path comparison |

The source UI supplies interaction lessons, not a copied application. The new
UI may reuse the bounded name and chord-validation semantics, but it owns a new
three-row DTO and visual layout.

## 4. Board C Firmware

### 4.1 Toolchain and target

- ESP-IDF baseline: 5.5.4.
- IDF target: `esp32s3`.
- Flash size and PSRAM mode remain unset until the exact supplied core module
  is identified. The normal ESP-IDF detection/default path is retained.
- The build directory and generated `sdkconfig` live outside source or in an
  ignored task-specific directory.
- Build, flash, and monitor are distinct operator commands.

### 4.2 Pin ownership

`board_c_pins.h` is the single source of truth for documented Board C pins:

| Owner | Pins | MVP state |
| --- | --- | --- |
| Rotary encoder | ENA/CLK 6, ENB/DT 7; SW unconnected | enabled |
| External confirm/action button | pull-up SIG 8; 3.3 V and common GND | enabled |
| Digital microphone | WS 42, SD 2, SCK 41 | reserved, disabled |
| ST7789 LCD | SCL 21, SDA 47, DC 43, CS 44 | reserved, disabled |
| WS2812 | 48 | reserved, disabled |
| Optional pull-up button | 12 | reserved for future PTT |

Camera, SD-card, and other documented module pins are recorded in the pin
manifest and excluded from ad hoc allocation. GPIO6/GPIO7 conflict with camera
use, and GPIO8 conflicts with DHT11 use. Compile-time/static checks cover
duplicate active pins, while host tests validate the full manifest for
collisions relevant to this demo.

### 4.3 Input implementation

- Preserve the source EC11 16-entry Gray transition table, four-quarter-step
  detent requirement, invalid two-bit-jump reset, and the 10 ms scan / 25 ms
  debounce safeguards. GPIO8 is an independent active-low pull-up button
  (idle HIGH, pressed LOW), not an encoder-integrated switch.
- The ESP32 adapter owns GPIO configuration, ISR queueing, overflow counters,
  and a worker task.
- The adapter emits only semantic events; it does not know shortcut mappings.

### 4.4 Serial event contract

One UTF-8 JSON record per line, prefixed for resynchronization:

```text
VKEY_INPUT/1 {"seq":1,"input":"ENCODER_CW"}
```

Allowed inputs are exactly:

- `ENCODER_CW`
- `ENCODER_CCW`
- `ENCODER_PRESS`

`ENCODER_PRESS` remains the stable event ID for the independent GPIO8
confirm/action button.

The firmware increments an unsigned sequence number for every emitted event.
A new Companion serial session resets its sequence tracker. Duplicate or
backward records are rejected; a forward gap is reported and the current valid
event may continue. Lines over 1024 bytes, invalid UTF-8/JSON, unknown fields,
unknown versions, and unknown input IDs are ignored without dispatch.

The future PTT/audio contract receives a new record kind or protocol version;
it is not represented as an encoder shortcut.

## 5. Compact Companion

### 5.1 Technology

Use a small Tauri 2 + React/Vite application because the source checkout has a
known Windows-native build path and the native Rust host can own serial access,
profile persistence, foreground inspection, and `SendInput`. Pin compatible
versions from the existing source baseline, but create an independent manifest
and lockfile.

The frontend talks only to typed Tauri command wrappers. It does not parse raw
serial records or call Windows APIs.

### 5.2 Single-window UI

The entire normal workflow fits in one compact window:

```text
┌─ VentureD Companion ───────────────────────────────┐
│ 设备串口 [COM… ▼] [刷新]       状态：已停止        │
│ 前台目标 Codex.exe · C:\…     [3 秒后捕获]       │
│                                                   │
│ 顺时针旋转          [上一项] [CTRL] [TAB ▼]       │
│ 逆时针旋转          [下一项] [CTRL] [TAB ▼]       │
│ GPIO8 外接确认/动作按钮 [确认动作]      [ENTER ▼] │
│                                                   │
│ [保存配置] [启动演练模式] [为本次运行启用实时模式] │
│ 最后事件：ENCODER_CW → CTRL+TAB · 演练模式        │
└───────────────────────────────────────────────────┘
```

UI rules:

- Physical input rows are fixed and cannot be added, deleted, or reordered.
- Names accept 1–40 trimmed Unicode characters and reject control characters.
- Chords use a bounded allowlist: modifiers `CTRL`, `ALT`, `SHIFT` plus one or
  more primary keys from `A-Z`, `0-9`, `F1-F24`, `ENTER`, `TAB`, `ESC`,
  `SPACE`, `[`, or `]`; total one to four keys. The mapping control records a real
  key-down chord after click, for example `CTRL+TAB+1`.
- Canonical modifier ordering makes permutations equivalent; duplicate chords
  across the three rows block save.
- Serial port selection comes from the native host's current enumeration.
- Target capture uses a three-second delay so the user can focus the intended
  app, then stores exact process name and normalized executable path.
- Browser visual fixtures use a typed fake host. They cannot open COM ports,
  write a profile, capture a process, or dispatch input.

### 5.3 Profile contract

The native host owns one versioned JSON profile in the Tauri application config
directory:

```json
{
  "version": 1,
  "revision": "sha256:...",
  "serial": { "port": "COM8", "baud": 115200 },
  "target": {
    "process_name": "Codex.exe",
    "process_path": "C:\\Program Files\\Codex\\Codex.exe"
  },
  "mappings": [
    {
      "input": "ENCODER_CW",
      "display_name": "Previous item",
      "keys": ["CTRL", "TAB"]
    }
  ]
}
```

The frontend submits a draft plus expected revision. The Rust host validates
the complete profile, refuses stale revisions, writes a private temporary file,
atomically replaces the profile, and retains a previous-byte backup only when
contents change. Raw native errors and full paths are not written to routine
logs or projected into generic status messages.

`live dispatch enabled` is runtime-only state. It is always false after launch,
never serialized, and cannot be enabled until serial, target, and all mappings
are valid.

### 5.4 Runtime state machine

```text
Stopped
  -> DryRun: open serial, decode, resolve, report only
  -> Live: explicit per-process enable, then open serial

DryRun/Live
  -> event decoded
  -> mapping resolved
  -> if DryRun: report resolved chord; do not construct restorer or dispatcher
  -> if Live: if saved target is not already foreground, copy-adapted
     restorer may focus the already-running exact-path window
  -> exact foreground process name/path rechecked after restore
  -> keyboard modifier state checked
  -> SendInput called once or event rejected with zero dispatch
  -> status projected to UI
```

Stopping closes the serial handle and clears live permission. Disconnects do
not silently switch ports or automatically enable live behavior on reconnect.

## 6. Security and Failure Behavior

- The serial stream can select only one of three physical IDs. It cannot supply
  a chord, process path, prompt, file path, or command.
- Only the locally validated profile can define shortcut chords.
- Wrong foreground process name/path fails closed.
- An unavailable foreground identity, dirty modifier state, invalid serial
  record, stale profile revision, duplicate chord, missing port, or disconnect
  produces a stable local reason and no input dispatch.
- Live input is never exercised by unit, browser visual, or firmware build
  commands.
- The UI never edits third-party application keybinding files.
- No secrets or cloud credentials are introduced.

## 7. Verification Boundaries

- Pure firmware host tests prove EC11 state logic and event formatting only.
- ESP-IDF build proves source/config/toolchain compatibility only.
- Rust tests prove parsing, profile, target-decision, and dry-run behavior.
- React/Vitest tests prove UI state and validation projections.
- Browser screenshots prove the fixture UI layout only.
- Native Tauri launch proves window/WebView2 liveness only.
- COM, physical encoder, `SendInput`, flash, and monitor each require separate
  real-environment evidence and authorization.

### 7.1 Three-stage command workflow

- **Development Debug:** `pnpm --dir companion dev:fixture`,
  `check:software`, and `test:interaction` use the Vite browser fixture only;
  no native or firmware build and no real I/O occur.
- **Native Integration:** Tauri `dev`/`build` is explicitly later and remains
  separate from app-config, foreground, COM, and `SendInput` authorization.
- **Hardware Acceptance:** ESP-IDF build, flash, monitor, and HIL are a
  separately authorized stage.

## 8. Rollback and Isolation

All implementation files are new to `VentureD`. Rollback is limited to the
task-owned new files or a task commit; no source checkout rollback is needed.
Before and after implementation, compare the source checkout's HEAD, porcelain
status, and selected SHA-256 values. Any mismatch stops completion and is
reported rather than repaired automatically.
