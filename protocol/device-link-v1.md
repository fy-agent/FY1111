# VentureD device link protocol v1

Host-to-device configuration and device-to-host network status share the USB
Serial-JTAG console with `VKEY_INPUT/1`. Each kind has its own prefix. Records
with an unknown prefix, unknown field, invalid JSON/UTF-8, or more than 1024
bytes are ignored.

## Host → device: `VKEY_CONFIG/1`

```text
VKEY_CONFIG/1 {"seq":1,"ssid":"home","password":"secret","apiKey":"sk-...","model":"FunAudioLLM/SenseVoiceSmall"}
```

| Field | Type | Meaning |
| --- | --- | --- |
| `seq` | unsigned 32-bit integer | Companion config sequence |
| `ssid` | string | STA SSID, 1–32 bytes |
| `password` | string | STA password, 0–64 bytes (empty = open) |
| `apiKey` | string | SiliconFlow bearer token, 1–256 bytes |
| `model` | string | `FunAudioLLM/SenseVoiceSmall` or `TeleAI/TeleSpeechASR` |

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

ICMP probe against a public host after the STA has an IP. `ms` is the average
successful RTT. Companion may display it for link-stability checks.
