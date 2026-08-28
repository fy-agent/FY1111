# Board C microphone record test

This increment verifies I2S capture and GPIO9 hold-to-talk. When Wi-Fi and a
SiliconFlow key are configured, the firmware also uploads 16 kHz mono WAV and
returns `VKEY_ASR/1`. Focused-window text insertion is still out of scope.

## Hardware

| Function | Wiring | Idle / pressed |
| --- | --- | --- |
| Mic start | GPIO9, same 3.3 V / GND / SIG pull-up as GPIO8 | HIGH / LOW |
| Digital mic | WS 42, SD 2, SCK 41 | I2S Philips RX |

GPIO8 remains the shortcut confirm button. GPIO12 stays reserved.

Firmware behaviour:

- Press and hold GPIO9: I2S starts, LCD shows `录音 <rms>`, serial emits
  `VKEY_REC/1` `START` then `ACTIVE` about every 250 ms.
- Release, or when the keep buffer is full: I2S stops, LCD shows `完成 <rms>`
  or `失败`, serial emits `DONE` with `silence` or `FAIL` with `reason=I2S`.
- GPIO9 never emits `VKEY_INPUT/1`.

Default slot is I2S left. If speaking keeps RMS at 0, rebuild with
`CONFIG_VENTURED_MIC_SLOT_RIGHT=y`.

## Offline checks (no board)

```powershell
pnpm --dir companion check:software
cargo test --manifest-path companion/src-tauri/Cargo.toml
```

From `firmware/board-c-demo/host_tests` if Ceedling is available:

```powershell
ceedling test:all
```

These prove pin uniqueness, PCM RMS/peak/silence, `VKEY_REC/1` formatting, and
Companion decoding. They do not prove a live microphone.

## Hardware checks (after an explicit flash)

1. Flash the current image to the Board C USB-Serial-JTAG port, then start
   Companion native and open **设置**.
2. Idle: no `VKEY_REC` lines. Encoder CW/CCW and GPIO8 still emit
   `VKEY_INPUT/1` only.
3. Hold GPIO9 in a quiet room for about 1 s. Expect `START` → `ACTIVE` →
   `DONE`. `silence` may be true; RMS should stay low (often under 80).
4. Hold GPIO9 and speak for about 1–2 s. Expect RMS and peak to rise clearly
   above the quiet take. LCD bottom line should change from `录音` to `完成`
   with a larger number. Companion settings should show 录音完成 and the RMS.
5. Hold GPIO9 for more than 3 s. Capture must continue until release (or
   buffer full), then `DONE`.
6. Press GPIO8 while not holding GPIO9. Only `ENCODER_PRESS` should appear.
   Press GPIO9; no `ENCODER_PRESS`.

Pass: quiet vs speak RMS is distinguishable, GPIO8/GPIO9 stay separate, Wi-Fi
status still updates. Fail: RMS stays 0 while speaking (`I2S` or wrong slot),
or GPIO9 emits a shortcut input.

Next increment (not this one): keep a short WAV and POST it to SiliconFlow.
