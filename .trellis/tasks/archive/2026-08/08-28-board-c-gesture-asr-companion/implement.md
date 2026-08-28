# Implement

## Done

- `tof_hand_gesture` + `rec_gate` with host tests
- Wi-Fi gate, ASR cancel, PCM wipe, lighter ping during ASR
- Transient LCD success/error; ready title 可录音
- Companion `TranscriptPanel` and serial ASR merge that keeps last text
- Sensor protocol `VKEY_SENSOR/1`; VL53L0X cloned at configure time only

## Verify

- `pnpm --dir companion check:software`
- Ceedling host tests for rec gate, gesture, and device-link ASR/REC
- Flashed `build-usbjtag` on COM9
