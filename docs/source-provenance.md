# Source provenance

The source checkout was read only during this extraction. Its verified baseline
was commit `a2ef7cb00af8cb47460921741bbf13e80119df1a` on `dev/xxk`. The source
is not a runtime, build, Cargo, frontend, or documentation dependency of this
project.

| Source path | SHA-256 | Destination | Treatment | Verification |
| --- | --- | --- | --- | --- |
| `firmware/vibekey-device/main/input/ec11_encoder_core.c` | `CB08D8838F317C2633B3739FFD61E443DCA29CCE299512D627780292C81B66B6` | `firmware/board-c-demo/main/input/ec11_core.c` | Adapted local naming; preserves 16-entry Gray table, invalid-jump reset and four-step detent | host tests |
| `firmware/vibekey-device/main/input/ec11_encoder_core.h` | `E993748178E881236351273191232B9E89B7D5A3ABFB794E31226715CE035FFA` | `firmware/board-c-demo/main/input/ec11_core.h` | Adapted local API | host tests |
| `firmware/vibekey-device/main/input/ec11_encoder_events.h` | `07E882212BDF6066F8BB92F315257F2CD47B27C2E30F0BF8FC7E0EE518B1597B` | `firmware/board-c-demo/main/protocol/input_event.*` | Adapted fixed three-input enum and v1 formatter | host tests |
| `experiments/desktop-control-agent/src/key_event.rs` | `2DA72EB96956F5BB35EE158FBB26951C514B5CC1D435D7EF0F4BC99D4791DA0C` | `companion/src-tauri/src/input.rs` | Reworked from log parsing to strict v1 machine records | Rust tests |
| `experiments/desktop-control-agent/src/serial_event_source.rs` | `4102FCCA72555A7FA9E253C5BE0DD64BDA656C2B3970028AE1E2B80E662E9F01` | `companion/src-tauri/src/serial.rs` | Adapted bounded line and sequence semantics; no port opened by tests | Rust tests |
| `experiments/desktop-control-agent/src/shortcut_backend.rs` | `B95063FD36C3F5B3FE372240F11E650F01D75D8ECB80802F4C24B43DC599887E` | `companion/src-tauri/src/runtime.rs` | Adapts the clean-key guard and isolated production `SendInput` boundary; offline tests inject fakes and never invoke Windows input | Rust tests |
| `experiments/desktop-control-agent/src/windows_foreground_probe.rs` | `9F93B9EE54B02801148541C35EA09DDF82152D65EFB0347028A8751E8292DA34` | `companion/src-tauri/src/target.rs` | Reworked as a probe interface, not executed in checks | Rust tests |
| `experiments/desktop-control-agent/src/foreground_target.rs` | `F517D4186256B74800EC671A509B08DE79D337F880B1497993C99585345DD868` | `companion/src-tauri/src/target.rs` | Adapted strict, case-insensitive name/path decision; extended with source restore outcomes and path matcher | Rust tests |
| `experiments/desktop-control-agent/src/windows_foreground_restore.rs` | `0EC06A009E23BB46FE0D9058332D462A2EAC8CDB7F443BA6ABA3005251DBD5DA` | `companion/src-tauri/src/windows_foreground_restore.rs` | Copied nearly verbatim; adapted only local `Target` / restorer-trait imports. Preserves exact-path window enumeration, minimized restore, bounded input-queue attachment, `SetForegroundWindow`, and post-focus verification | Rust tests |

The current repository owns all destination code. No copied source file is
claimed as Board C hardware acceptance.
