# Companion Runtime Contracts

> Executable contracts for the Board C serial-event Companion. This spec is
> shared by the Rust/Tauri host and the React frontend.

## Scenario: Guarded Board C shortcut runtime

### 1. Scope / Trigger

Apply this spec when changing the Board C input protocol, the saved Companion
profile, a Tauri command, runtime state, serial polling, foreground guards, or
the React projection of those values. It also applies when changing Companion
development commands or user-visible language.

The production adapters must exist even when a task is limited to offline
verification. Tests replace serial, profile storage, foreground probes,
modifier state, and input dispatch through injected fakes; production code
must not be replaced with constant empty/error placeholders merely to avoid
performing side effects during tests.

### 2. Signatures

The typed Tauri command surface is:

```text
list_ports() -> Result<string[], string>
capture_target_after_delay() -> Result<TargetDraft, string>
load_profile() -> Result<ProfileDraft | null, string>
save_profile(draft: ProfileDraft) -> Result<ProfileDraft, string>
start_dry_run() -> Result<RuntimeStatus, string>
enable_live_for_run() -> Result<RuntimeStatus, string>
poll_runtime_event() -> Result<RuntimeStatus, string>
stop_runtime() -> Result<RuntimeStatus, string>
load_device_settings() -> Result<DeviceSettings, string>
save_device_settings(draft: DeviceSettings) -> Result<DeviceSettings, string>
apply_device_config(request) -> Result<NetworkStatus, string>
poll_network_status() -> Result<NetworkStatus, string>
```

React must consume these commands through one `CompanionHost` interface. It
must not parse raw serial lines, write the profile, probe the foreground
process, or call Windows input APIs.

The development command surface is:

```text
pnpm --dir companion dev              # browser fixture alias
pnpm --dir companion dev:fixture      # Vite fixture, no native host/build
pnpm --dir companion check:software   # lint + typecheck + Vitest only
pnpm --dir companion test:interaction # Playwright fixture interaction/visual
pnpm --dir companion dev:native       # Tauri internal bridge only
```

`dev:native` must be referenced by Tauri's `beforeDevCommand`; developers do
not use it as the ordinary debug entry point.

### 3. Contracts

Firmware emits one bounded UTF-8 record per line:

```text
VKEY_INPUT/1 {"seq":<u32>,"input":"ENCODER_CW|ENCODER_CCW|ENCODER_PRESS"}
```

Host-to-device Wi-Fi/API configuration and device-to-host network status use
the additional record kinds in `protocol/device-link-v1.md`:

```text
VKEY_CONFIG/1 {"seq":<u32>,"ssid":"...","password":"...","apiKey":"...","model":"..."}
VKEY_NET/1 {"seq":<u32>,"state":"DISCONNECTED|CONNECTING|CONNECTED|FAILED","ssid":"...","ip":"...","rssi":<i32>,"reason"?:"AUTH|TIMEOUT|NO_AP|BAND|UNKNOWN"}
VKEY_REC/1 {"seq":<u32>,"state":"START|ACTIVE|DONE|FAIL","ms":<u32>,"samples":<u32>,"rms":<u32>,"peak":<u32>,"silence"?:bool,"reason"?:"I2S|BUSY|UNKNOWN"}
VKEY_ASR/1 {"seq":<u32>,"state":"START|DONE|FAIL","text"?:"...","reason"?:"WIFI|KEY|AUTH|FORMAT|HTTP|MEM|BUSY"}
```

`apiKey` and `model` may be empty (Wi-Fi only). A non-empty model is any
printable id up to 64 bytes; the UI preset is
`XingChenAGI/XingChenASR-V3.2-Ultra`. GPIO9 must never emit `VKEY_INPUT/1`.
ASR `text` is at most 360 bytes and must not include secrets or PCM.

The persisted shortcut profile is exactly:

```json
{
  "version": 1,
  "revision": "sha256:<digest>",
  "serial": { "port": "<selected-port>", "baud": 115200 },
  "target": { "processName": "App.exe", "processPath": "C:\\Path\\App.exe" },
  "mappings": [
    { "input": "ENCODER_CW", "displayName": "Previous item", "keys": ["CTRL", "TAB"] }
  ]
}
```

- Rust serde types own the persisted schema and use `deny_unknown_fields` at
  the profile, serial, target, and mapping boundaries.
- `revision` is the SHA-256 of the complete versioned profile with `revision`
  cleared. Load validates both content and revision; save uses the submitted
  revision as the optimistic-concurrency expectation.
- Wi-Fi SSID/password, SiliconFlow API key, and model persist in a separate
  `device.json` document (`version: 1`). They must not appear in
  `profile.json`, `lastEvent`, notices, or firmware logs.
- Runtime status serializes as `state`, `liveEnabled`, `lastEvent`,
  `gapMissed`, and `network`. States are `STOPPED`, `DRY_RUN`, and `LIVE`.
  `network.state` is `UNKNOWN`, `DISCONNECTED`, `CONNECTING`, `CONNECTED`, or
  `FAILED`.
- Apply may open the selected serial port while shortcut runtime is stopped.
  Dry-run/live reuse that port. Stop clears shortcut mode but keeps the device
  session so network status can continue.
- Duplicate or backward serial sequences are dropped. A forward gap keeps the
  current valid event and reports the missed count through `gapMissed` and a
  stable `SERIAL_GAP/<n>` status.
- Live permission is process-local. It is never persisted, starts false, and
  is cleared by a successful stop or serial-error shutdown.
- The visible UI language is Simplified Chinese (`zh-CN`). Product names,
  protocol IDs, GPIO numbers, Tauri command names, and shortcut tokens remain
  unchanged technical identifiers. English native status strings are projected
  through one frontend localization owner before display.
- GPIO6/GPIO7 own the encoder phases. GPIO8 owns an independent active-low
  external confirm/action button while retaining the stable `ENCODER_PRESS`
  protocol ID; it is not presented as an integrated encoder switch. GPIO9 owns
  the microphone hold-to-talk button and reports `VKEY_REC/1` only.
- Shortcut chords are recorded from a real key-down combination after the user
  clicks the mapping control. Allowed tokens are `CTRL`/`ALT`/`SHIFT` plus one
  or more primaries from `A-Z`, `0-9`, `F1-F24`, `ENTER`, `TAB`, `ESC`,
  `SPACE`, `[`, or `]`, totaling one to four keys. Save persists the recorded tokens; live
  dispatch uses only the saved profile.

### 4. Validation & Error Matrix

| Condition | Required result |
| --- | --- |
| Unknown protocol version/field/input, invalid UTF-8/JSON, or line over 1024 bytes | Ignore the record and never resolve or dispatch a shortcut. Resynchronize at the next newline. |
| Duplicate/backward sequence | Drop the event. |
| Forward sequence gap | Accept the current valid event and report the exact missed count. |
| Profile version is not 1, contains unknown/flat serial fields, has a missing target/port, duplicate inputs/chords, invalid name/chord, or wrong revision | Reject load/save fail-closed. |
| Save/load/capture/start while runtime is active, or a repeated start | Reject without changing profile or runtime ownership. |
| Dry-run event | Resolve and report the configured chord; do not construct or call a restorer or input dispatcher. |
| Live event with wrong foreground identity, restore missing/rejected, dirty supported key state, unmapped input, or dispatch failure | Keep the event rejected and do not send input. Never start an executable to satisfy a missing target. |
| Live event after restore | Recheck exact foreground process name/path before modifiers or `SendInput`. |
| Serial read error | Close the source, enter `STOPPED`, clear live permission, and report a stable stopped reason. |
| UI stop request fails | Do not claim `STOPPED` or clear the displayed live state locally; preserve active/unknown state and allow retry. |
| Ordinary development command | Use the browser fixture and perform no native/firmware build, COM access, foreground probe, profile write, or input dispatch. |
| Explicit later Tauri integration | Use `dev:native`; never inherit fixture mode from the ordinary `dev` alias. |
| User-visible English runtime text | Project it to Chinese before rendering; preserve only allowed technical identifiers. |

Routine command errors must be stable user-facing summaries. Do not expose raw
OS errors, full private paths, or serial payload contents in generic status
messages.

### 5. Good/Base/Bad Cases

- Good: a saved valid profile is loaded at startup, the UI restores its port,
  target, names, chords, and revision, and the new runtime still reports
  `STOPPED` with `liveEnabled=false`.
- Base: an absent profile leaves the UI in an editable default state and blocks
  dry-run/live start until a complete profile is saved.
- Bad: a profile with top-level `serialPort`/`baud`, a stale revision, or an
  unknown nested serial field is rejected instead of migrated implicitly.
- Bad: a test replaces `list_ports` with a permanent empty production command,
  or a dry-run path constructs a dispatcher and relies on it not being called.
- Good: `pnpm --dir companion dev` opens the Chinese browser fixture without
  compiling or starting the native host.
- Bad: Tauri's `beforeDevCommand` calls the ordinary fixture `dev` alias, so a
  later native integration silently runs against fake host methods.

### 6. Tests Required

- Firmware host tests assert full clockwise/counter-clockwise detents, invalid
  transitions, debounce boundaries, exact formatter output, and Board C pin
  ownership.
- Rust tests assert malformed/overlong/multi-line resynchronization, sequence
  duplicate/backward/gap behavior, exact profile JSON shape, unknown/version/
  revision rejection, atomic save/load, active-runtime guards, serial-error
  shutdown, restart live-off, dry-run zero-dispatch, live guard rejection,
  restore missing/rejected with zero dispatch, restore then exact recheck then
  one dispatch, and already-matching foreground leaving restore unchanged.
- TypeScript tests assert Rust-equivalent name/chord validation and saved-profile
  hydration, Chinese state/error projection including restore missing/rejected,
  command-script boundaries, and that Tauri uses the separate native bridge.
- Browser visual tests assert exactly three mapping rows, a collapsed settings
  control (no Wi-Fi fields until expanded), initial stopped/live off state, the
  configure/save/dry-run/stop flow, the fixture apply-network flow, and no
  clipping, overlap, or horizontal overflow. They also assert `lang="zh-CN"`,
  Chinese controls and GPIO8 external-button wording. A frontend build is not
  visual-regression evidence.
- Native build proves compilation only. Real serial, foreground capture,
  profile storage, Windows input, flash, and HIL require separate evidence and
  authorization.

### 7. Wrong vs Correct

#### Wrong

```typescript
// UI privately invents a second persisted contract and assumes stop succeeded.
type Profile = { serialPort: string; baud: number };
await host.stop().catch(() => undefined);
setRuntime({ state: "STOPPED", liveEnabled: false, lastEvent: "", gapMissed: null });
```

#### Correct

```typescript
// UI imports the shared projection and changes state only from the host result.
interface ProfileDraft {
  version: 1;
  revision: string | null;
  serial: { port: string; baud: number };
  target: TargetDraft | null;
  mappings: MappingDraft[];
}

const confirmed = await host.stop();
setRuntime(confirmed);
```

For command routing:

```jsonc
// Wrong: native integration inherits fixture mode.
{ "scripts": { "dev": "vite --mode fixture" }, "tauriBeforeDev": "pnpm dev" }

// Correct: daily development is fixture-only; Tauri has an explicit bridge.
{
  "scripts": {
    "dev": "pnpm dev:fixture",
    "dev:fixture": "vite --mode fixture --host 127.0.0.1 --port 1425 --strictPort",
    "dev:native": "vite --host 127.0.0.1 --port 1425 --strictPort"
  },
  "tauriBeforeDev": "pnpm dev:native"
}
```
