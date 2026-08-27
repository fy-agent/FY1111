# Companion Reuse Expansion Audit

## Purpose

Record the 2026-08-27 scope refinement: minimize newly designed demo code and
prefer selective, provenance-tracked reuse from the read-only VibeKey checkout.
The source checkout remains immutable; reuse means copying or narrowly adapting
tracked source into `VentureD`, never creating a runtime dependency on the
source path.

## Current VentureD evidence

| Capability | Evidence | Accurate status |
| --- | --- | --- |
| Chinese compact UI | `companion/src/App.tsx:158-166` | Implemented and browser-fixture tested |
| Three fixed shortcut mappings | `companion/src/App.tsx:164-165` | Implemented; real device input not tested |
| Delayed foreground capture UI | `companion/src/App.tsx:90-102`, `companion/src/App.tsx:163` | Implemented |
| Native Windows foreground identity capture | `companion/src-tauri/src/lib.rs:85-108`, `companion/src-tauri/src/target.rs:75-138` | Implemented but not yet exercised through a native Tauri launch |
| Foreground restore / bring-to-front | no implementation under `companion/` | Missing |
| Read-only Hook status | no implementation under `companion/` | Missing |
| Native launch | `.trellis/tasks/08-27-vibekey-hardware-hackathon-demo/implement.md:130-133` | Pending |
| Fresh ESP-IDF 5.5.4 build | `implement.md:161` | Pending |
| Flash, serial monitor, physical input HIL | `implement.md:171-183` | Not authorized or performed |
| LCD initialization or HIL | no LCD driver in `firmware/board-c-demo` | Not implemented or verified |

The user's statement that the Companion has no foreground acquisition should
therefore be interpreted as a real-environment acceptance gap, not a total code
absence: the browser fixture returns `Fixture.exe`, while the native command
has not yet been launched and observed.

## Reuse-first candidates

Source baseline:

- checkout: `C:\Users\xk\Desktop\realkey\vibekey_new`
- branch: `dev/xxk`
- commit: `a2ef7cb00af8cb47460921741bbf13e80119df1a`

| Source path | SHA-256 | Planned treatment |
| --- | --- | --- |
| `experiments/desktop-control-agent/src/windows_foreground_restore.rs` | `0EC06A009E23BB46FE0D9058332D462A2EAC8CDB7F443BA6ABA3005251DBD5DA` | Copy nearly verbatim, adapting only local target/trait imports. Preserve exact-path window enumeration, minimized-window restore, bounded input-queue attachment, `SetForegroundWindow`, and post-focus verification. |
| `experiments/desktop-control-agent/src/foreground_target.rs` | `F517D4186256B74800EC671A509B08DE79D337F880B1497993C99585345DD868` | VentureD already contains a focused adaptation. Extend only with the source restore outcome/trait instead of inventing a second decision model. |
| `experiments/desktop-control-agent/src/windows_foreground_probe.rs` | `9F93B9EE54B02801148541C35EA09DDF82152D65EFB0347028A8751E8292DA34` | Already adapted in VentureD; keep its exact name/path identity boundary. |
| `experiments/desktop-control-agent/src/shortcut_backend.rs` | `B95063FD36C3F5B3FE372240F11E650F01D75D8ECB80802F4C24B43DC599887E` | Already adapted in VentureD; preserve modifier-state guard and isolated `SendInput` boundary. |
| `experiments/codex-desktop-observer/src/hook_config.rs` | `E788CD18399021F432EAF0CE41FCCE45040D1130D9FA416D5C32E724A878707D` | Copy only the read-only managed-Observer classifier and its parsing tests into a small local `hook_status` module. Do not copy proposal, install, replace, removal, backup, or restore builders. |
| `experiments/desktop-control-agent/setup-ui/src-tauri/src/hook_setup.rs` | `4A1A913E4B48F4238E5F285DE7E4228D178F7668B28B02EACE4DB02F0ABE2EA7` | Reference only for the read-only projection at lines 1641-1697. Do not copy the 128 KiB coordinator or transaction machinery. |
| `experiments/desktop-control-agent/setup-ui/src/MappingEditor.tsx` | `C42D51EA4B7FBF117750D9D93A087F9DDA11E5397C2FEE1069B2F4EE1476DA49` | Do not copy wholesale: it owns the production 19-input/catalog workflow. Retain the existing three-row Demo editor and reuse only already-adapted validation semantics. |

## Minimal combination boundary

```text
existing VentureD profile + mapping UI
  -> existing strict serial decoder
  -> copied VibeKey foreground restorer
  -> existing exact foreground recheck
  -> existing guarded shortcut dispatcher

selected Codex project directory
  -> fixed `.codex/hooks.json` path
  -> copied read-only classifier
  -> redacted status DTO
  -> compact Hook status card
```

The foreground restorer may only restore an already-running visible top-level
window whose executable path exactly matches the saved target. It must not
start an executable, run a shell command, choose a target supplied by serial,
or treat restore success as dispatch authority without a second foreground
identity check.

The Hook reader may report only a bounded state such as `missing`,
`not_installed`, `installed_unverified`, or `blocked`, plus event-group and
handler counts. It must not expose Hook commands or full private paths to the
React layer, scan Codex sessions, run an Observer, create a candidate, install,
repair, replace, remove, trust, or restore a Hook.

## Hardware and LCD sequencing

The shortest reliable hardware path is:

1. Fresh ESP-IDF 5.5.4 build with isolated `SDKCONFIG` and build directory.
2. Identify exact Board C module, flash/PSRAM, COM port, and candidate image.
3. Obtain fresh authorization for bounded flash/reset/monitor.
4. Verify USB serial boot and GPIO6/GPIO7 rotation before adding more display
   code.
5. Verify the separate action-button topology, if used.
6. Only then add a minimal ST7789 feasibility screen (solid colors, boot state,
   last input) if the user keeps LCD in the Demo scope.

LCD pins do not collide with the current GPIO6/GPIO7 rotary channels, but the
display is not currently an implemented or tested capability.

## Remaining product decision

The source supports a much larger multi-application profile, while the current
Demo owns one captured target and three physical events. The smallest reuse-led
path is to keep one saved target, restore that already-running window, and send
the configured shortcut. Expanding now to Codex plus WorkBuddy multi-application
switching would require a profile migration, application selector, active-app
state, more UI, and additional acceptance tests.
