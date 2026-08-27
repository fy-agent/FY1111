# Extract VibeKey Hardware Hackathon Demo

## Goal

Create an independently buildable hardware-hackathon demo in this `VentureD`
repository by selectively reusing proven VibeKey input and Windows shortcut
delivery behavior. The demo must target the competition's Board C and leave
`C:\Users\xk\Desktop\realkey\vibekey_new` byte-for-byte untouched.

The immediate user value is a clean competition workspace with a small native
Companion that can configure and run the shortcut demo, while leaving clear
extension points for later microphone/Agent work without depending on or
mutating the production-oriented VibeKey checkout.

## Background and Confirmed Facts

- The source checkout is
  `C:\Users\xk\Desktop\realkey\vibekey_new` at commit
  `a2ef7cb00af8cb47460921741bbf13e80119df1a` on `dev/xxk`.
- The source checkout already contains two untracked user-owned paths:
  `.trellis/tasks/08-17-reset-shortcut-profile-defaults/` and
  `docs/当前实现介绍.md`. They are excluded from the write set and extraction.
- Reusable source behavior includes the pure EC11 Gray-code/debounce core,
  semantic `ENCODER_CW` / `ENCODER_CCW` / `ENCODER_PRESS` events, bounded UART
  line parsing, Windows foreground-process checks, and guarded `SendInput`
  shortcut delivery.
- The source EC11 profile uses GPIO1/GPIO2/GPIO21 and cannot be copied as Board
  C configuration. The confirmed demo topology assigns rotary ENA/CLK to
  GPIO6, ENB/DT to GPIO7, leaves encoder SW unconnected, and uses an independent
  active-low external confirm/action button on GPIO8 while retaining the
  `ENCODER_PRESS` protocol ID.
- The competition documentation identifies Board C as an ESP32-S3 AIoT kit and
  recommends ESP-IDF 5.5.4. Its documented fixed resources are:
  - initial board documentation: rotary encoder CLK GPIO6, DT GPIO7, switch GPIO8;
  - digital microphone: WS GPIO42, SD GPIO2, SCK GPIO41;
  - ST7789 LCD: SCL GPIO21, SDA GPIO47, DC GPIO43, CS GPIO44;
  - WS2812: GPIO48;
  - optional pull-up button: GPIO12.
- Board C's exact module flash/PSRAM capacity and the machine's future serial
  port are not yet proven. The demo must not hard-code either before a hardware
  identification check.
- No speech-recognition platform is selected in this task. Microphone and
  Agent integration need explicit extension seams but are not required to run
  the first shortcut demo.
- The approved product scope is a minimal firmware and Companion extraction
  with a new compact shortcut-configuration UI. It does not copy the source
  project's full Tauri Setup UI or its multi-application integration stack.
- A 2026-08-27 refinement changes the implementation strategy to
  **reuse-first**: copy or narrowly adapt proven tracked VibeKey modules when
  they already provide the required Demo behavior, rather than redesigning
  equivalent code. Every additional reuse still requires source commit/hash
  provenance and must remain independent of the source checkout at runtime.
- The current Companion already contains a Chinese `3 秒后捕获` UI and a native
  Windows foreground-identity command (`companion/src/App.tsx` and
  `companion/src-tauri/src/lib.rs`). Native launch/real foreground-capture
  evidence is still required.
- The first demo keeps a single captured application target: restore that
  already-running window, then send one of three configured shortcuts. It does
  not inspect Codex Hook/Observer status, and it does not copy Setup UI,
  application catalogs, or session pairing.
- The current `VentureD` repository is a clean, single-repository Trellis
  bootstrap with no product source tree yet. The independent
  `00-bootstrap-guidelines` task is not part of this task.
- GPIO6/GPIO7 conflict with camera use and GPIO8 conflicts with DHT11 use for
  this confirmed wiring; those peripherals remain out of scope unless a future
  pin-allocation decision changes the topology.

## Requirements

### R1. Source checkout remains read-only

- Do not edit, delete, rename, move, stage, commit, build into, flash from, or
  generate files inside the source checkout.
- Read only tracked source files that are explicitly selected for reuse.
- Record source commit and SHA-256 provenance for every copied/reworked source
  module.
- Recheck source `HEAD`, Git status, and selected-source hashes after the
  extraction work.

### R2. Independent competition-demo layout

- Create all new product files only in this `VentureD` repository.
- The resulting demo must not use path dependencies or runtime reads from the
  source checkout.
- Keep firmware, Windows companion, shared protocol, documentation, and local
  scripts in explicit top-level boundaries.
- The Companion must use a small native Windows UI rather than requiring users
  to hand-edit its normal configuration file.
- Include a provenance manifest that distinguishes copied, adapted, and new
  files without implying that unverified source features were accepted on
  Board C.

### R3. Board C ESP-IDF baseline

- Create an ESP-IDF project targeting `esp32s3` and document ESP-IDF 5.5.4 as
  the competition toolchain baseline.
- Provide defaults for Board C's GPIO6/GPIO7 rotary encoder and independent
  GPIO8 confirm/action button, and reserve the
  documented microphone, LCD, WS2812, camera, SD-card, and optional sensor
  resources against accidental reuse.
- Do not inherit VibeKey's 16 MiB flash, 8 MiB octal PSRAM, custom identity
  partition, MSC installer image, Security HID, or BLE settings without Board
  C evidence and a demo requirement.
- Keep serial port, baud, and build directory parameterized. Do not hard-code a
  COM port.
- Provide separate commands for `set-target`, configure, build, flash, and
  monitor so a build does not implicitly write hardware.
- Any later real flash requires fresh confirmation of the board, enumerated
  port, candidate image/commit, write regions, and monitor/reset permission.

### R4. Minimum hardware input behavior

- Reuse/adapt the tested EC11 pure logic rather than rewriting Gray-code and
  button debounce behavior.
- Emit a small, versioned, machine-readable serial event contract for encoder
  clockwise, counter-clockwise, and independent GPIO8 press events while
  retaining `ENCODER_PRESS` as the fixed protocol ID.
- A malformed, overlong, incomplete, or unknown event must not produce a
  shortcut.
- Reserve a versioned input-event space for a future push-to-talk button and
  microphone session without implementing speech recognition now.

### R5. Windows shortcut Companion and compact configuration UI

- Consume Board C events through a user-selected serial port.
- Provide one compact native window containing only:
  - serial-port refresh and selection;
  - current connection/runtime state;
  - delayed capture and display of the allowed foreground target application;
  - one mapping row each for encoder clockwise, counter-clockwise, and press;
  - save, dry-run start/stop, and explicitly enabled live start/stop controls.
- Each mapping row must support a 1–40 character local display name and one
  validated shortcut chord from the bounded key allowlist.
- Physical input IDs are fixed to the three encoder events. The UI must reject
  duplicate chords and invalid/incomplete mappings before save.
- Persist the local profile atomically and restore it after restart. Runtime
  live-dispatch permission must default to off on every process start and must
  not be persisted as enabled.
- Dispatch only explicitly configured shortcut chords.
- Before dispatch, verify the allowed foreground process identity and require
  clean keyboard modifier state.
- Provide dry-run mode that parses and resolves events without calling
  `SendInput`.
- The UI must not read or modify Codex/WorkBuddy shortcut files. Its profile is
  owned only by this demo.
- Prefer the source project's already tested foreground-restore and guarded
  shortcut boundaries over a new implementation. The copied/adapted restore
  path may target only an already-running visible window with an exact saved
  executable path, and a successful restore must be followed by another exact
  foreground identity check before `SendInput`.
- Do not start an application executable, invoke a shell command, or accept an
  application path from the serial stream.
- Do not import the source project's provisioning, identity, MSC, BLE pairing,
  WorkBuddy parser, full application catalog, complete Setup UI, or Codex
  Hook/Observer inspection.

### R6. Extension boundary for later voice work

- Document, but do not implement, the future boundary:
  `microphone capture -> audio transport -> speech recognizer -> text/Agent ->
  guarded text insertion`.
- Do not select, mock as accepted, or embed credentials for an ASR/Agent
  provider in the first demo.

### R7. Bring-up and LCD evidence

- Preserve the current evidence order: fresh ESP-IDF 5.5.4 build, exact board
  identification, separately authorized flash/reset/monitor, serial boot and
  input HIL, then optional LCD feasibility.
- The ST7789 LCD is currently reserved only. No current artifact may claim it
  can initialize or display content.
- If LCD remains in the Demo scope after basic input HIL, the first display
  increment is limited to a minimal self-test/status screen. A complete UI,
  animation system, or copied production display stack is not required.

## Acceptance Criteria

- [ ] AC1: `VentureD` contains an independent ESP-IDF Board C demo with no
      absolute or relative dependency on `vibekey_new`.
- [ ] AC2: configuration inspection shows target `esp32s3`, Board C GPIO6/GPIO7
      rotary pins plus the independent GPIO8 action button, and explicit
      reservations for the documented fixed
      resources; no VibeKey N16R8 or identity/MSC assumptions leak in.
- [ ] AC3: an ESP-IDF 5.5.4 build is attempted in a fresh out-of-tree build
      directory. Success is reported as build evidence only; an environment
      blocker is recorded with the exact missing prerequisite.
- [ ] AC4: no flash, reset, serial monitor, or other hardware write occurs
      without a later explicit hardware authorization.
- [ ] AC5: host tests cover complete EC11 detents, invalid transitions, button
      debounce, valid event decoding, malformed/overlong event rejection,
      unmapped inputs, and dry-run dispatch.
- [ ] AC6: the Windows companion builds independently and can consume fixture
      serial lines into resolved encoder mappings without `SendInput` in
      dry-run mode.
- [ ] AC7: the compact UI exposes serial selection, target capture, exactly
      three encoder mapping rows, validation, persistence, and runtime controls
      in one window; save/reload/restart preserves names and chords.
- [ ] AC8: duplicate or incomplete chords cannot be saved, and live dispatch
      is off again after every Companion process restart.
- [ ] AC9: browser-based visual regression covers the compact window, while a
      separate native Tauri launch check establishes only window/runtime
      liveness and does not claim real serial or `SendInput` acceptance.
- [ ] AC10: any live `SendInput` result is reported separately from offline
      tests and requires an explicitly selected foreground test target.
- [ ] AC11: the source checkout's final `HEAD`, Git status, and selected file
      hashes match the recorded pre-extraction snapshot.
- [ ] AC12: README instructions cover environment setup, build-only workflow,
      separately authorized flash/monitor commands, mapping configuration,
      known limitations, and the deferred microphone/Agent seam.
- [ ] AC13: a native Tauri run proves the existing delayed foreground capture
      against one harmless selected application; browser fixture output is not
      accepted as native foreground evidence.
- [ ] AC14: when the saved target is running but not foreground, the reused
      restore path either focuses the exact executable window and rechecks it
      before dispatch, or reports a stable missing/rejected result with zero
      dispatch. It never launches an executable.
- [ ] AC15: provenance records the additional foreground-restore source path,
      commit, SHA-256 value, and adaptation boundary. Codex Hook classifier
      paths are not part of this demo.
- [ ] AC16: LCD remains reported as unverified until a separately authorized
      physical self-test succeeds; serial input HIL is completed first.

## Out of Scope

- Choosing or integrating a speech-recognition platform.
- Microphone audio capture or transport.
- TiDB Agent Stack runtime integration and credentials.
- Automatic text insertion, clipboard mutation, or a true Windows IME.
- Wi-Fi provisioning, Agent_link/ROROLEE integration, and BLE pairing.
- Camera, SD card, OLED, speaker, sensor, motor, or a full LCD UI. A minimal
  LCD feasibility/status increment is deferred until after basic input HIL.
- The source Companion's full Setup UI, application catalogs, mapping search,
  reorderable S1–S16 profile, Codex Hook/Observer inspection, onboarding,
  install/repair/restore, Observer execution, diagnostics, and BLE screens.
- VibeKey provisioning, device identity, MSC installer media, Security HID, or
  formal V1 product contracts.
- Editing, cleaning, staging, committing, building, or flashing from the source
  checkout.
- Real hardware flashing or HIL in the planning phase.

## Closed Product Decision

This 48-hour Demo keeps the current single captured application target:
restore that already-running window and send one of three configured
shortcuts. It does not expand to the source project's Codex + WorkBuddy
multi-application profile, Hook inspection, or switching model.
