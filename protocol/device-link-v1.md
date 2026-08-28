# VentureD device link protocol v1

Host-to-device configuration and device-to-host network status share the Board C
HID + Vendor USB link with `VKEY_INPUT/1`. Companion auto-opens VID `0x303A` /
PID `0x82D0` (product `VentureD Board C`) and does not use a COM port. HID
carries Teensy-style 64-byte RawHID frames: `report[0]` is length 1–63,
`report[1..]` is payload. Host hidapi writes may prepend report-id `0`. Vendor
WinUSB is the same newline stream for later raw USB clients. Each kind has its
own prefix. Records with an unknown prefix, unknown field, invalid JSON/UTF-8,
or more than 1024 bytes are ignored.

## Host → device: `VKEY_CONFIG/1`

```text
VKEY_CONFIG/1 {"seq":1,"ssid":"home","password":"secret","apiKey":"sk-...","model":"XingChenAGI/XingChenASR-V3.2-Ultra"}
```

| Field | Type | Meaning |
| --- | --- | --- |
| `seq` | unsigned 32-bit integer | Companion config sequence |
| `ssid` | string | STA SSID, 1–32 bytes |
| `password` | string | STA password, 0–64 bytes (empty = open) |
| `apiKey` | string | SiliconFlow bearer token, 0–256 bytes (empty = Wi-Fi only) |
| `model` | string | empty, or a SiliconFlow model id such as `XingChenAGI/XingChenASR-V3.2-Ultra` |

The firmware must not print this line, the password, or the API key.

## Device → host: `VKEY_NET/1`

```text
VKEY_NET/1 {"seq":1,"state":"CONNECTED","ssid":"home","ip":"192.168.1.8","rssi":-45}
```

| Field | Type | Meaning |
| --- | --- | --- |
| `seq` | unsigned 32-bit integer | Firmware network-status sequence |
| `state` | string | `DISCONNECTED`, `CONNECTING`, `CONNECTED`, or `FAILED` |
| `ssid` | string | Last requested or associated SSID |
| `ip` | string | IPv4 text, empty when not connected |
| `rssi` | signed integer | Station RSSI, `0` when unknown |

Optional field `reason` may be `AUTH`, `TIMEOUT`, `NO_AP`, `BAND`, or `UNKNOWN`
on `FAILED`. `BAND` means the requested SSID looks like a 5 GHz-only name;
ESP32-S3 station mode is 2.4 GHz only. `NO_AP` means a 2.4 GHz scan did not
see the SSID. Do not include password, API key, or raw HTTP bodies.

## Device → host: `VKEY_LOG/1`

```text
VKEY_LOG/1 {"seq":1,"msg":"sta got ip=10.0.0.8 rssi=-45 ssid=home"}
```

Human-readable debug only. Never include password or API key.

## Device → host: `VKEY_PING/1`

```text
VKEY_PING/1 {"seq":1,"host":"8.8.8.8","ok":true,"ms":18,"lost":0,"sent":3}
```

ICMP probe against `8.8.8.8` after the STA has an IP. One echo on connect and
about once a minute; heartbeats only republish Wi-Fi status. Probes are skipped
while ASR is uploading so they do not stall transcription. `ms` is the average
successful RTT. Companion may display it for link-stability checks.

## Device → host: `VKEY_REC/1`

```text
VKEY_REC/1 {"seq":3,"state":"DONE","ms":1200,"samples":19200,"rms":800,"peak":9000,"silence":false}
```

| Field | Type | Meaning |
| --- | --- | --- |
| `seq` | unsigned 32-bit integer | Firmware record sequence |
| `state` | string | `START`, `ACTIVE`, `DONE`, or `FAIL` |
| `ms` | unsigned integer | Elapsed capture time |
| `samples` | unsigned integer | Converted PCM sample count |
| `rms` | unsigned integer | Integer RMS of int16 PCM |
| `peak` | unsigned integer | Absolute peak of int16 PCM |

`DONE` includes `silence` (RMS below 80). `FAIL` may include `reason` =
`I2S|BUSY|WIFI|UNKNOWN`. GPIO9 and the far-to-near hand do not start capture
unless the station is `CONNECTED`; a drop mid-capture also fails with `WIFI`.
GPIO9 must not emit `VKEY_INPUT/1`. After a non-silent `DONE`, if Wi-Fi is up
and both `apiKey` and `model` are set, the firmware wraps 16 kHz mono 16-bit
PCM as WAV and POSTs it to SiliconFlow.

## Device → host: `VKEY_ASR/1`

```text
VKEY_ASR/1 {"seq":2,"state":"DONE","text":"今天天气不错"}
```

| Field | Type | Meaning |
| --- | --- | --- |
| `seq` | unsigned 32-bit integer | Firmware transcription sequence |
| `state` | string | `START`, `DONE`, or `FAIL` |
| `text` | string | Present on `DONE`. JSON-escaped, at most 360 bytes |
| `reason` | string | Present on some `FAIL`: `WIFI`, `KEY`, `AUTH`, `FORMAT`, `HTTP`, `MEM`, `BUSY`, `CANCEL` |

`CANCEL` means the current upload was stopped by GPIO9 or a far-to-near hand
while transcription was running. The firmware does not emit `BUSY` on every
retry; a later start waits until the user releases and triggers again.

Do not include PCM, password, API key, or raw HTTP bodies.

## Device → host: `VKEY_SENSOR/1`

```text
VKEY_SENSOR/1 {"seq":1,"pir":true,"state":"OK","distMm":312}
```

| Field | Type | Meaning |
| --- | --- | --- |
| `seq` | unsigned 32-bit integer | Firmware sensor sequence |
| `pir` | boolean | GPIO16 PIR: `true` when the module output is HIGH (motion) |
| `state` | string | `OK`, `TOF`, or `I2C` |
| `distMm` | unsigned integer | Present on a valid VL53L0X sample, millimeters |

`TOF` means the VL53L0X on I2C GPIO4/GPIO5 is missing or a sample was invalid.
`I2C` means the I2C master bus did not start. Do not include raw I2C dumps.
