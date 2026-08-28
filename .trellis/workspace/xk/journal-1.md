# Journal - xk (Part 1)

> AI development session journal
> Started: 2026-08-27

---



## Session 1: Archive Board C demo and ignore generated outputs

**Date**: 2026-08-27
**Task**: Archive Board C demo and ignore generated outputs
**Branch**: `master`

### Summary

Finished and archived the Board C hardware-hackathon demo task, kept generated IDF/Companion outputs out of git, and recorded the committed demo plus archive path for the next session.

### Main Changes

- Archived 08-27-vibekey-hardware-hackathon-demo to archive/2026-08 and left 00-bootstrap-guidelines active.
- Kept firmware/Companion generated outputs gitignored and committed only source, docs, and Trellis artifacts.
- Updated AI_HANDOFF.md to the archived task path and remaining HIL/voice gaps.

### Git Commits

| Hash | Message |
|------|---------|
| `a78b2e4` | (see git log) |
| `e8ae048` | (see git log) |

### Testing

- [OK] git status --ignored showed build-usbjtag, sdkconfig, node_modules, target, gen, dist, and test-results ignored.

### Status

[OK] **Completed**

### Next Steps

- Do competition HIL for encoder and GPIO8 restore plus live SendInput only with fresh authorization.
- Do not archive 00-bootstrap-guidelines until that first-time setup task is actually finished.


## Session 2: Archive wifi-oled-settings and commit Board C ASR

**Date**: 2026-08-28
**Task**: Archive wifi-oled-settings and commit Board C ASR
**Branch**: `master`

### Summary

Finished GPIO9 hold-to-talk, XingChen ASR upload, longer PSRAM capture, archived 08-27-wifi-oled-settings, and committed locally on master.

### Main Changes

- Added on-device SiliconFlow WAV upload and VKEY_ASR display
- Enabled octal 8MB PSRAM so GPIO9 records until release or buffer full
- Archived 08-27-wifi-oled-settings; left 00-bootstrap-guidelines active

### Git Commits

| Hash | Message |
|------|---------|
| `5168bcf` | (see git log) |

### Testing

- [OK] Firmware flashed to COM9 with Embedded PSRAM 8MB
- [OK] Companion software checks and firmware host tests passed earlier in the session

### Status

[OK] **Completed**

### Next Steps

- Do not archive 00-bootstrap-guidelines
- Focused-window insertion of ASR text is still out of scope


## Session 3: Archive gesture ASR companion and push Board C voice path

**Date**: 2026-08-28
**Task**: Archive gesture ASR companion and push Board C voice path
**Branch**: `main`

### Summary

Committed hand-gesture ASR, Companion transcript, and 可录音 ready state; archived 08-28-board-c-gesture-asr-companion.

### Main Changes

- Far-near ToF record control, rec gate, Wi-Fi gate, ASR cancel, Companion transcript card
- LCD/Companion ready title is 可录音; recording is 录音中
- Archived 08-28-board-c-gesture-asr-companion; left 00-bootstrap-guidelines active

### Git Commits

| Hash | Message |
|------|---------|
| `c9d6d6f` | (see git log) |

### Testing

- [OK] pnpm --dir companion check:software
- [OK] Flashed build-usbjtag to COM9

### Status

[OK] **Completed**

### Next Steps

- Do not archive 00-bootstrap-guidelines
- Focused-window insertion of ASR text is still out of scope


## Session 4: Archive HID Vendor USB and push Board C auto-link

**Date**: 2026-08-28
**Task**: Archive HID Vendor USB and push Board C auto-link
**Branch**: `main`

### Summary

Replaced COM picker with HID+Vendor USB auto-link and archived 08-28-board-c-hid-vendor-usb.

### Main Changes

- TinyUSB HID+Vendor transport; Companion hidapi auto-connect; no COM picker
- Default GPIO11 ENTER, GPIO10 CTRL+N, GPIO8 CTRL+SHIFT+N; obsolete 3-row profile save
- Archived 08-28-board-c-hid-vendor-usb; left 00-bootstrap-guidelines active

### Git Commits

| Hash | Message |
|------|---------|
| `6659e01` | (see git log) |
| `0967a18` | (see git log) |

### Testing

- [OK] ceedling hid_frame + companion check:software + Windows VID_303A PID_82D0 enum

### Status

[OK] **Completed**

### Next Steps

- Do not archive 00-bootstrap-guidelines
- Later flashes use ROM download (BOOT+RESET); focused-window ASR insert still out of scope
