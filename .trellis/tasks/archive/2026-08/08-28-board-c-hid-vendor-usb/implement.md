# Implement

## Done

- TinyUSB HID generic inout + Vendor WinUSB (MS OS 2.0) replaces USB-Serial-JTAG
  as the `VKEY_*` transport. VID `0x303A`, PID `0x82D0`.
- Companion `hidapi` auto-opens the HID link. UI shows 已插入/未插入; no COM picker.
- HID frames are Teensy-style 64-byte RawHID (`len` + payload). Host tests cover pack/unpack.
- Default mappings: GPIO11 `ENTER`, GPIO10 `CTRL+N`, GPIO8 `CTRL+SHIFT+N`.
- Save can replace an obsolete three-row `profile.json`.
- First image flashed over COM9. Later flashes use ROM download (BOOT + RESET).

## Verify

- `ceedling test:all` in `firmware/board-c-demo/host_tests`
- `pnpm --dir companion check:software`
- Rust unit tests including HID frame and obsolete-profile save
- Playwright fixture snapshot
- Windows enumerated `VID_303A&PID_82D0` HID + VentureD Vendor
