# Design: Wi-Fi settings, device link, OLED status

## Decisions

1. **Shortcut profile stays v1.** Secrets live in a second file
   `device.json` under the Tauri app config dir. This avoids revision-hash
   breakage and keeps API keys out of the mapping contract.
2. **Host → device is a new record kind**, not a new `VKEY_INPUT` field:
   `VKEY_CONFIG/1 {"seq","ssid","password","apiKey","model"}`.
3. **Device → host status** is `VKEY_NET/1 {"seq","state","ssid","ip","rssi"}`.
   Optional `reason` is `AUTH|TIMEOUT|NO_AP|BAND|UNKNOWN` only. `BAND` is the
   5 GHz-name reject; ESP32-S3 STA is 2.4 GHz only.
4. **One COM session is shared.** Apply may open the selected port while the
   shortcut runtime is stopped. Dry-run/live reuse that port. Stop clears
   shortcut mode but keeps the device session so status can continue.
5. **VibeKey has no Wi-Fi, NVS secrets, ST7789, or Chinese font.** Reuse is
   limited to the existing bounded serial line buffer and the reserved Board C
   LCD pins. New STA, NVS, config parser, and 16×16 glyphs are local.
6. **SiliconFlow key is stored for the next increment.** This task only
   configures and delivers it. No HTTP upload yet.
7. **Display is the reserved ST7789 SPI panel** (SCL 21, SDA 47, DC 43, CS 44,
   RST unused). User-facing copy may say OLED; the pin owner is this LCD.

## Companion

- Collapsed settings row under the serial section: label **设置** plus a
  network chip (未连接 / 连接中 / 已连接 / 失败).
- Expanded fields: SSID, password, API key (`type=password`), model select.
- **保存并下发联网** validates, persists `device.json`, writes `VKEY_CONFIG/1`.
- `pollNetworkStatus` while a device session exists; `pollRuntimeEvent` also
  copies the latest network state into `RuntimeStatus.network`.

## Firmware

- `nvs_flash_init` + namespace `ventured`.
- stdin / USB-Serial-JTAG line reader parses `VKEY_CONFIG/1`.
- STA connect; event handler emits `VKEY_NET/1` and refreshes the status
  canvas. Encoder `VKEY_INPUT/1` is unchanged.
- Small 16×16 Chinese glyph set: 未连接中已失败网络 plus ASCII digits/`.:-`.

## Security

- Never log or format password/apiKey.
- Companion notices say only “已下发” / “联网失败”, never the payload.
- `device.json` is local-only and gitignored if created in-repo.
