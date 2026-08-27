# VentureD Hackathon AI Handoff

> Read this file first when continuing the hackathon work in another AI
> session. It separates confirmed requirements, implemented software, pending
> validation, and future product ideas so that offline evidence is not
> mistaken for hardware or competition acceptance.

## 1. Mission

Build a 48-hour hardware-hackathon demo by extracting only the useful input
and guarded Windows shortcut behavior from the existing VibeKey project into
this independent repository.

The immediate demo is a compact physical shortcut controller:

```text
Board C rotary input
  -> versioned USB serial event
  -> compact Windows Companion
  -> locally configured shortcut mapping
  -> foreground-application guard
  -> dry-run or explicitly enabled Windows shortcut dispatch
```

The longer-term product direction is a context-aware voice input device that
can remember and correct domain terms such as `trellis` and `workbuddy`, then
insert recognized text into the active application. That voice path is a
future extension and is not implemented in the current demo.

## 2. Repositories and ownership boundaries

### Current implementation repository

- Path: `C:\Users\xk\Desktop\VentureD`
- Branch: `master`
- Active Trellis task:
  `.trellis/tasks/08-27-vibekey-hardware-hackathon-demo`
- Task status at the time of this handoff: `in_progress`
- The repository currently contains uncommitted task-owned firmware,
  Companion, protocol, documentation, and Trellis changes. Inspect the current
  Git status before editing and do not overwrite unrelated work.

### Read-only VibeKey source repository

- Path: `C:\Users\xk\Desktop\realkey\vibekey_new`
- Recorded branch: `dev/xxk`
- Recorded commit:
  `a2ef7cb00af8cb47460921741bbf13e80119df1a`
- This checkout is evidence and selective source material only. It must remain
  byte-for-byte untouched by this task.
- Do not edit, delete, rename, move, stage, commit, format, build into, flash
  from, or generate files inside this source checkout.
- The source checkout already had user-owned untracked paths when extraction
  began:
  `.trellis/tasks/08-17-reset-shortcut-profile-defaults/` and
  `docs/当前实现介绍.md`. They are outside this task.
- Reused source paths and their recorded SHA-256 values are documented in
  `docs/source-provenance.md`. Recheck the source HEAD, Git status, and hashes
  before declaring extraction complete; report differences instead of
  repairing or reverting the source checkout.

## 3. Competition materials

- Hardware-track presentation:
  <https://tidb-ai-hardware-hacktho-ect1jik.gamma.site/>
- Pre-match instructions and Board C pin information:
  <https://tidb-pre-match-intro-dct7ede.gamma.site/>
- Agent Link repository:
  <https://github.com/DeotalandDev/Agent_link>
- Scenario challenge references:
  <https://tidb-scenario-challenge-52ota9p.gamma.site/>
- TiDB Agent Stack introduction:
  <https://tidb-agent-stack-intro-avsk9wk.gamma.site/>
- Agent Stack development skill repository:
  <https://github.com/mem9-ai/agent-stack-dev-guide>
- Agent Stack development skill document:
  <https://tidb-agent-stack-develop-50arovb.gamma.site/>
- Hello World and Agent Stack example:
  <https://github.com/you06/esp32-agent-lcd>
- Submission artifact requirements:
  <https://tidb-sumbit-artifacts-hxy4mrq.gamma.site/>
- Board C manufacturer documentation:
  <https://www.openjumper.com/doc/esp32s3-aiot>

The official pre-match material presents `agent_link` as the normal path from
the ESP32-S3 boards through the ROROLEE phone application to a cloud Agent. It
also documents Wi-Fi as an advanced direct-Agent path. The current repository
does not yet implement either path. Before final submission, the team must
confirm with the competition rules or organizers whether the USB shortcut MVP
alone is acceptable or whether `agent_link`/ROROLEE or a TiDB Agent integration
is mandatory.

## 4. Approved current MVP

The current task intentionally implements only the smallest independent
shortcut demo:

- A new ESP-IDF project for competition Board C.
- A strict, versioned serial input protocol.
- A compact Tauri 2 + React/Vite Windows Companion.
- Exactly three fixed logical inputs:
  `ENCODER_CW`, `ENCODER_CCW`, and `ENCODER_PRESS`.
- A one-window configuration UI containing:
  - serial-port refresh and selection;
  - current runtime state;
  - delayed foreground-target capture;
  - one mapping row for each fixed input;
  - a 1-40 character local display name per row;
  - a bounded shortcut chord per row;
  - save, dry-run start/stop, and explicitly enabled live start/stop controls;
  - last-event and status projection.
- An application-owned, atomically saved local profile.
- Runtime-only live permission that always starts disabled after a Companion
  restart and is never persisted as enabled.
- Fail-closed foreground process and keyboard modifier checks before live
  `SendInput` dispatch.
- A browser fixture for UI and interaction testing that cannot open serial
  ports, write the real profile, capture a foreground target, or call
  `SendInput`.

The Companion never reads or modifies Codex, WorkBuddy, or other third-party
`keybindings.json` files. Shortcut chords are owned by the demo profile.

## 5. Explicitly excluded from the current MVP

- Microphone capture or audio transport.
- Selecting or integrating an ASR provider.
- TiDB Agent Stack runtime or credentials.
- `agent_link`, ROROLEE, BLE pairing, or Wi-Fi provisioning.
- Automatic text insertion, clipboard mutation, or a Windows IME.
- LCD, OLED, camera, SD card, speaker, sensor, motor, or WS2812 behavior.
- The original VibeKey Setup UI, Hook, Observer, WorkBuddy parser,
  provisioning, identity, MSC installer, security HID, application catalogs,
  or S1-S16 profile system.

These exclusions are scope boundaries, not claims that the features are
unnecessary for the final competition submission.

## 6. Board C hardware facts and current wiring decision

### Confirmed Board C baseline

- MCU family: ESP32-S3.
- Competition toolchain baseline: ESP-IDF 5.5.4.
- Exact flash and PSRAM configuration has not yet been proven and must not be
  copied from VibeKey's N16R8 assumptions.
- Do not hard-code a COM port.

Documented Board C resources:

| Resource | Pins | Current MVP state |
| --- | --- | --- |
| Rotary channels | CLK GPIO6, DT GPIO7 | enabled |
| Independent action input | GPIO8 | implemented in firmware, physical wiring pending |
| Digital microphone | WS GPIO42, SD GPIO2, SCK GPIO41 | reserved, disabled |
| ST7789 LCD | SCL GPIO21, SDA GPIO47, DC GPIO43, CS GPIO44 | reserved, disabled |
| WS2812 | GPIO48 | reserved, disabled |
| Optional pull-up button | GPIO12 | reserved, disabled |

GPIO6/GPIO7 conflict with camera operation, and GPIO8 conflicts with DHT11
operation. Do not enable those peripherals without a new pin-allocation
decision.

### Rotary encoder connector clarification

The supplied rotary encoder contains a five-terminal EC11 mechanism, but the
module and Board C provide a four-pin connection labelled `V G 6 7`.

The currently confirmed cable connection is:

```text
encoder module V   -> Board C V (3.3 V module rail)
encoder module G   -> Board C GND
encoder CLK/A      -> GPIO6
encoder DT/B       -> GPIO7
```

This four-pin path carries power, ground, and the two rotary channels. It does
not carry GPIO8, so it does not prove that pressing the encoder shaft reaches
the ESP32-S3. The encoder's mechanical switch must be treated as unconnected.
Do not infer connector orientation from wire colors; follow the silk-screened
labels.

The current firmware and protocol retain `ENCODER_PRESS`, but project it as a
separate active-low GPIO8 confirm/action button, not the integrated encoder
switch. That separate physical button connection has not been validated on
the actual kit. If the team prefers to use the documented GPIO12 pull-up
button instead, that is a coordinated firmware, documentation, and test change
that requires an explicit product decision; do not silently change the pin.

For a rotation-only demonstration, the two EC11 switch terminals may remain
unconnected. If a future decision requires pressing the same physical knob,
identify the switch pair while fully powered off with a continuity meter, then
design a separate GPIO and ground connection. Do not solder or fly-wire based
only on EC11 shape or wire color.

## 7. Firmware and serial contract

The firmware project is under `firmware/board-c-demo` and targets `esp32s3`.
The pin manifest is owned by
`firmware/board-c-demo/main/board_c_pins.h`.

The retained EC11 behavior includes a 16-entry Gray-code transition table,
four-quarter-step detents, invalid two-bit-jump resynchronization, bounded
counters, bounce/reversal cancellation, and active-low button debounce.
Business mappings do not belong in the firmware driver.

Firmware emits one bounded UTF-8 record per line:

```text
VKEY_INPUT/1 {"seq":1,"input":"ENCODER_CW"}
```

Allowed input IDs are exactly:

- `ENCODER_CW`
- `ENCODER_CCW`
- `ENCODER_PRESS`

The Companion rejects unknown versions, unknown fields, unknown input IDs,
invalid UTF-8/JSON, incomplete records, and lines over 1024 bytes. Duplicate
or backward sequence numbers are dropped. A forward gap reports the missed
count while allowing the current valid event to continue. Serial data can
select only a physical input ID; it can never supply a shortcut chord,
process path, prompt, file path, or command.

The canonical protocol description is `protocol/input-event-v1.md`.

## 8. Companion contracts

The Companion is under `companion` and uses:

- Tauri 2 and Rust for serial access, profile persistence, target probing,
  runtime state, and guarded Windows input.
- React, Vite, and TypeScript for the one-window UI.
- A single typed `CompanionHost` boundary between UI and native commands.

The profile contains version, revision, selected serial port and baud, exact
foreground process identity, and the three mappings. The native host is the
schema authority and rejects unknown fields, stale revisions, invalid names,
invalid chords, duplicate inputs, and duplicate canonical chords.

Dry-run opens and parses the selected production serial source but must not
construct an input dispatcher. Live mode requires explicit per-process
enablement, an exact foreground process match, and clean supported modifier
state. Stop or serial-error shutdown clears live permission.

The detailed executable contract is
`.trellis/spec/backend/companion-runtime-contracts.md`.

## 9. Repository map

```text
VentureD/
├── AI_HANDOFF.md                         # this cross-session handoff
├── README.md                             # operator workflow and boundaries
├── firmware/board-c-demo/               # independent ESP-IDF project
├── companion/                           # Tauri/Rust + React/Vite Companion
├── protocol/input-event-v1.md           # serial protocol authority
├── docs/board-c-demo.md                  # Board C setup and evidence limits
├── docs/source-provenance.md             # read-only VibeKey reuse record
└── .trellis/tasks/08-27-vibekey-hardware-hackathon-demo/
    ├── prd.md
    ├── design.md
    └── implement.md
```

## 10. Current implementation status

The Trellis implementation checklist currently marks these areas as created:

- independent project skeleton and provenance document;
- Board C firmware, pin manifest, EC11 adapter, serial formatter, and host
  tests;
- Rust/Tauri serial, profile, target, runtime, and guarded dispatch layers;
- compact Chinese Companion UI, validation, persistence projection, fixture,
  unit tests, and Playwright interaction/visual test;
- README, protocol documentation, and Board C guide;
- a native debug/no-bundle build.

Treat those checkboxes as task progress, not permanent proof. A new session
should rerun the relevant checks before making a release or acceptance claim.

Known pending items in the current implementation plan:

- a fresh ESP-IDF 5.5.4 build in an isolated build/config path;
- a bounded native Tauri launch/liveness check;
- actual Board C identification, flash/PSRAM confirmation, and COM selection;
- physical encoder and independent action-button HIL;
- any flash, reset, or serial-monitor session;
- any foreground capture or real `SendInput` test;
- any microphone, ASR, `agent_link`, ROROLEE, TiDB Agent, or text-insertion
  integration.

## 11. Verification and authorization boundaries

Use three separate stages and never merge their claims.

### Development Debug

```powershell
pnpm --dir companion dev:fixture
pnpm --dir companion check:software
pnpm --dir companion test:interaction
```

These commands exercise the browser fixture only. They are not native, COM,
foreground-capture, profile-storage, `SendInput`, firmware, or hardware
evidence.

### Native Integration

Relevant commands include:

```powershell
pnpm --dir companion tauri build --debug --no-bundle
pnpm --dir companion tauri dev
```

Native compilation proves compilation. A native launch proves window and
process liveness only. Neither action authorizes COM access, profile writes,
foreground capture, or `SendInput` unless the user has explicitly approved
that exact runtime operation.

### Hardware Acceptance

After exporting ESP-IDF 5.5.4, the build-only shape is:

```powershell
idf.py -C firmware/board-c-demo set-target esp32s3
idf.py -C firmware/board-c-demo -B <fresh-build-dir> -D SDKCONFIG=<fresh-sdkconfig> build
```

Do not run `flash`, `erase-flash`, `monitor`, reset the board, or open a COM
port merely because a build succeeds. Before any hardware write or monitor
operation, obtain fresh authorization for:

- the exact attached board and currently enumerated COM port;
- the candidate commit and image path/hash;
- the exact flash regions to be written;
- reset permission;
- serial-monitor permission;
- explicit exclusions such as full erase, bootloader/partition replacement,
  security fuses, Secure Boot, Flash Encryption, identity media, and unrelated
  partitions.

Prior VibeKey COM/HIL results are not authorization for this Board C.

## 12. Future voice and Agent extension

The intended future boundary is:

```text
microphone capture
  -> bounded audio transport
  -> selected speech recognizer
  -> glossary/context correction backed by an Agent and TiDB
  -> reviewed text result
  -> guarded insertion into the explicitly allowed foreground application
```

Important unresolved product decisions include:

- which ASR platform to use and how it handles streaming, Chinese/English
  mixed speech, latency, pricing, privacy, and custom vocabulary;
- whether `agent_link`/ROROLEE is required by judging or whether a direct Wi-Fi
  and TiDB Agent path is acceptable;
- what contextual memory is stored in TiDB and how users inspect, correct, or
  delete it;
- whether text insertion uses clipboard paste, simulated keystrokes, an
  application adapter, or another guarded Windows mechanism;
- how a hardware push-to-talk action is allocated without colliding with the
  current encoder, camera, DHT11, microphone, and display resources.

Do not fabricate a provider, credentials, latency result, or hardware
acceptance to fill these gaps.

## 13. Recommended first actions for another AI session

1. Read `AGENTS.md`, this file, and the active Trellis task artifacts.
2. Run `python .\.trellis\scripts\get_context.py` and inspect both Git
   worktrees before editing.
3. Preserve the current write-set boundaries; do not reset, clean, stage, or
   reformat unrelated files.
4. Read the protocol and Companion runtime contract before changing any event,
   pin, profile, Tauri command, runtime state, or UI projection.
5. Reconfirm whether the next goal is software verification, native launch,
   hardware build/HIL, or competition Agent integration. Each has a different
   approval and evidence boundary.
6. If touching hardware, stop at the authorization gate described above.
7. Before calling the demo competition-ready, reconcile the current USB-only
   MVP with the official `agent_link`/ROROLEE/TiDB Agent expectations and the
   submission artifact checklist.

## 14. Definition of honest status

Use precise evidence language:

- `implemented`: relevant source exists in this repository;
- `offline tested`: named software tests were run and passed in the current
  environment;
- `build verified`: the named build command completed for the recorded
  toolchain and inputs;
- `native liveness verified`: the actual Tauri window/process launched and was
  observed;
- `HIL verified`: the exact physical board/image/port and operator actions
  were recorded and passed;
- `competition accepted`: only after requirements and submission evidence are
  checked, not merely because the software builds.

If a level was not directly verified in the current session, report it as
pending or memory-derived rather than upgrading the claim.
