# Implement

## 1. Contracts and protocol

- [x] Write `protocol/device-link-v1.md`.
- [x] Extend Companion runtime contracts with device settings, new Tauri
      commands, `RuntimeStatus.network`, and the two new record kinds.

## 2. Firmware host-testable records + display labels

- [x] `config_record` parser and `net_event` formatter.
- [x] Status label map for OLED copy.
- [x] Ceedling tests for parse/format/pin ownership.

## 3. Firmware STA + display

- [x] NVS + Wi-Fi STA module.
- [x] Config RX task on USB-Serial-JTAG stdin.
- [x] ST7789 status screen with the small Chinese font; fail-soft.

## 4. Companion host

- [x] `device_settings` store.
- [x] Serial write + `VKEY_NET/1` decode; shared device session.
- [x] Tauri commands and fixture host.

## 5. Companion UI

- [x] Collapsed settings + apply + live status chip.
- [x] Validation and Chinese projection.

## 6. Checks

- [x] `pnpm --dir companion check:software`
- [x] `pnpm --dir companion test:interaction`
- [x] `cargo test` in `companion/src-tauri`
- [x] Firmware host tests

## 7. GPIO9 record + SiliconFlow ASR (same task, later increment)

- [x] Hold-to-talk on GPIO9; never emit `VKEY_INPUT/1`.
- [x] Keep 16 kHz mono 16-bit PCM, wrap a 44-byte WAV header, stream HTTPS
      multipart to `api.siliconflow.cn/v1/audio/transcriptions`.
- [x] Default model `XingChenAGI/XingChenASR-V3.2-Ultra`; empty key/model is
      Wi-Fi only; custom model ids are allowed.
- [x] `VKEY_REC/1` and `VKEY_ASR/1` to Companion; LCD 转写 / 转写完成 / 转写失败.
- [x] Enable octal 8MB PSRAM; record until release or keep-buffer full
      (~minutes), not a 3 s hard stop.
