# Board C HID + Vendor USB auto-link

## Goal

Replace the UART COM refresh/select flow with a plug-and-play TinyUSB
composite device (HID generic inout + Vendor WinUSB) so Companion connects
when the cable is inserted.

Reuse TinyUSB `hid_generic_inout` and `webusb_serial` Microsoft OS 2.0
descriptors (MIT). Do not write a new USB stack.

## Requirements

- Device enumerates as HID + Vendor. Windows binds HID immediately; Vendor
  uses MS OS 2.0 WINUSB (no INF).
- Same `VKEY_*` line protocol, framed on HID as 64-byte RawHID reports.
- Companion finds VID/PID and opens automatically. No COM dropdown.
- Keep `00-bootstrap-guidelines` unarchived.
- First flash can still use USB-Serial-JTAG; later flashes use ROM download
  mode because TinyUSB takes the USB PHY.

## Acceptance Criteria

- [x] Firmware no longer installs USB-Serial-JTAG as the VKEY transport.
- [x] Companion shows inserted/not-inserted and auto-opens the Board C USB link.
- [x] Apply Wi-Fi and dry-run/live work without choosing a COM port.
- [x] Host tests cover HID frame pack/unpack.
- [x] Companion software checks pass.
