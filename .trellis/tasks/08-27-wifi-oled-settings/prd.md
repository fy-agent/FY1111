# Board C WiFi settings and OLED status

## Goal

Add a first increment of Board C cloud connectivity: a collapsed Companion
settings panel for Wi-Fi and the SiliconFlow API key, serial delivery of those
secrets to the ESP32, STA connection with status back to Companion, and a first
ST7789/OLED status screen that shows the same network state in a small Chinese
glyph set.

Microphone capture, HTTP upload, transcription insert, and focused-window text
injection are **out of scope** for this increment. They are the next product
step after this settings and link path exists.

## Requirements

- Keep the existing three-row shortcut mapping UI, `VKEY_INPUT/1`, and
  version-1 shortcut profile contract unchanged.
- Companion shows a collapsed **设置** control. Expanding it reveals Wi-Fi
  SSID, Wi-Fi password, SiliconFlow API key, and transcription model.
- Saving/applying settings persists them locally (not inside the shortcut
  `profile.json`) and, when a serial port is selected, writes one bounded
  host-to-device config record.
- ESP32 stores credentials in NVS, joins the STA network, and emits
  `VKEY_NET/1` status records. Companion shows the latest connection state
  even when the settings panel is collapsed.
- API key, Wi-Fi password, and raw config lines must never appear in UI
  notices, runtime `lastEvent`, or firmware logs.
- Start a reserved-pin ST7789 status display: 未连接 / 连接中 / 已连接 / 失败
  plus IP when connected. Display init failure must not block Wi-Fi.
- Do not archive `00-bootstrap-guidelines`. Do not flash or open a COM monitor
  unless the user later authorizes that exact board and port.

## Acceptance Criteria

- [ ] Settings is collapsed by default and does not add a fourth mapping row.
- [ ] Fixture and native hosts expose load/save/apply/poll for device settings
      and network status through `CompanionHost`.
- [ ] Shortcut `profile.json` remains version 1 with no secret fields.
- [ ] Device settings persist across Companion restart; API key is masked.
- [ ] `VKEY_CONFIG/1` and `VKEY_NET/1` are specified, parsed strictly, and
      host-tested. Unknown prefixes are still ignored.
- [ ] Firmware can parse config, connect STA, and format `VKEY_NET/1` without
      echoing secrets.
- [ ] OLED/LCD status labels use the small Chinese glyph set; missing display
      hardware is fail-soft.
- [ ] Existing shortcut software checks still pass. Visual tests cover the
      collapsed settings chip and the apply flow in the fixture.

## Notes

Later increment (not this task): I2S microphone on GPIO42/2/41, ESP32 HTTPS
multipart upload to `https://api.siliconflow.cn/v1/audio/transcriptions`
(`FunAudioLLM/SenseVoiceSmall` or `TeleAI/TeleSpeechASR`), OLED preview of
returned `text`, and Companion insertion into a focused desktop input box.
