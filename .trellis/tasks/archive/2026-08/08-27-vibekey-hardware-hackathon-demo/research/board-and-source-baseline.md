# Board C and Source Baseline

## Evidence priority

1. Local source checkout code, configuration, tests, Git state, and hashes.
2. Competition pre-match documentation for Board C and toolchain data.
3. Official competition example and Agent_link repository for architectural
   context only.

## Board C findings

The competition pre-match document identifies Board C as an ESP32-S3 AIoT kit
and recommends ESP-IDF 5.5.4. Relevant documented pins are:

| Resource | Pins |
| --- | --- |
| Rotary encoder | CLK GPIO6, DT GPIO7, switch GPIO8 |
| Digital microphone | WS GPIO42, SD GPIO2, SCK GPIO41 |
| ST7789 LCD | SCL GPIO21, SDA GPIO47, DC GPIO43, CS GPIO44 |
| WS2812 | GPIO48 |
| Pull-up button | GPIO12 |

### Confirmed demo wiring refinement

The physical demo wiring is more specific than the pre-match board table:
rotary ENA/CLK uses GPIO6, ENB/DT uses GPIO7, and encoder SW is unconnected.
An independent three-pin pull-up confirm/action button uses SIG -> GPIO8 with
3.3 V and common GND (idle HIGH, pressed LOW). It keeps the fixed
`ENCODER_PRESS` protocol ID. GPIO6/GPIO7 conflict with camera use and GPIO8
conflicts with DHT11 use, so those peripherals cannot be enabled under this
topology.

The exact supplied core module's flash/PSRAM capacity and future COM port were
not established. Configuration must target `esp32s3` without carrying over the
source project's N16R8 assumptions or a hard-coded port.

Source: <https://tidb-pre-match-intro-dct7ede.gamma.site/>

## Source checkout state

- Path: `C:\Users\xk\Desktop\realkey\vibekey_new`
- Branch: `dev/xxk`
- HEAD: `a2ef7cb00af8cb47460921741bbf13e80119df1a`
- Upstream divergence: `+0 -0`
- Pre-existing untracked paths excluded from extraction:
  - `.trellis/tasks/08-17-reset-shortcut-profile-defaults/`
  - `docs/当前实现介绍.md`

## Selected source hashes

| Source path | SHA-256 |
| --- | --- |
| `firmware/vibekey-device/main/input/ec11_encoder_core.c` | `CB08D8838F317C2633B3739FFD61E443DCA29CCE299512D627780292C81B66B6` |
| `firmware/vibekey-device/main/input/ec11_encoder_core.h` | `E993748178E881236351273191232B9E89B7D5A3ABFB794E31226715CE035FFA` |
| `firmware/vibekey-device/main/input/ec11_encoder_events.h` | `07E882212BDF6066F8BB92F315257F2CD47B27C2E30F0BF8FC7E0EE518B1597B` |
| `experiments/desktop-control-agent/src/key_event.rs` | `2DA72EB96956F5BB35EE158FBB26951C514B5CC1D435D7EF0F4BC99D4791DA0C` |
| `experiments/desktop-control-agent/src/serial_event_source.rs` | `4102FCCA72555A7FA9E253C5BE0DD64BDA656C2B3970028AE1E2B80E662E9F01` |
| `experiments/desktop-control-agent/src/shortcut_backend.rs` | `B95063FD36C3F5B3FE372240F11E650F01D75D8ECB80802F4C24B43DC599887E` |
| `experiments/desktop-control-agent/src/windows_foreground_probe.rs` | `9F93B9EE54B02801148541C35EA09DDF82152D65EFB0347028A8751E8292DA34` |
| `experiments/desktop-control-agent/src/foreground_target.rs` | `F517D4186256B74800EC671A509B08DE79D337F880B1497993C99585345DD868` |

## Reuse conclusions

- Reuse the EC11 pure state logic and its safeguards.
- Replace test-only ESP_LOG substring parsing with a new strict versioned
  machine event record.
- Preserve the bounded serial reader, foreground process guard, and guarded
  SendInput behavior at their semantic boundaries.
- Build a new fixed-three-row UI using the source UI's bounded-name,
  canonical-chord, persistence, and duplicate-rejection lessons.
- Do not copy the full Setup UI, Codex/WorkBuddy catalogs, Hook/Observer,
  provisioning, identity, MSC, BLE, or cloud/Agent modules.

## External architectural context

The official `esp32-agent-lcd` example uses USB CDC JSONL between an ESP32
device and a desktop bridge. This supports the bridge shape but does not prove
this demo's competition acceptance or hardware behavior.

Source: <https://github.com/you06/esp32-agent-lcd>
