# Board C gesture ASR companion

## Goal

Finish the Board C voice path that was left after Wi-Fi and GPIO9 ASR:
hand-gesture record control, safe start/stop, Companion-visible transcripts,
and a ready-state LCD that says 可录音 instead of 录音.

## Requirements

- PIR on GPIO16 is seat occupancy only. VL53L0X on GPIO4/5 drives far→near
  start and near→far stop. Sky readings 20-70 mm are empty, never near.
- GPIO9 remains hold-to-talk and never emits `VKEY_INPUT/1`.
- Recording starts only when Wi-Fi is CONNECTED. A second press or far→near
  during ASR cancels the upload once.
- Companion always shows a 转写 card. Idle is 可录音; active record is 录音中.
- LCD ready hero is 可录音; recording hero is 录音中. Add only generated
  glyphs; do not invent a second ToF driver or commit `components/vl53l0x/`.
- Do not archive `00-bootstrap-guidelines`.

## Acceptance Criteria

- [x] Far→near starts record when seated or to cancel ASR; near→far stops record.
- [x] Rec gate uses rising-edge start, min length, cooldown, and one CANCEL.
- [x] Offline record is rejected with `VKEY_REC` reason `WIFI`.
- [x] Companion shows ASR text without opening Settings; START keeps last text.
- [x] Ready UI says 可录音 on LCD and Companion; recording says 录音中.
- [x] Host tests cover gesture, rec gate, and ASR CANCEL; Companion checks pass.
- [x] `vl53l0x` vendor tree stays gitignored.

## Notes

- Focused-window insertion of ASR text remains out of scope.
