# Research: SiliconFlow transcriptions and VibeKey reuse

## SiliconFlow

- Endpoint: `POST https://api.siliconflow.cn/v1/audio/transcriptions`
- Auth: `Authorization: Bearer <API key>`
- Body: multipart `file` (≤1 h, ≤50 MB) + `model`
- Models: `FunAudioLLM/SenseVoiceSmall`, `TeleAI/TeleSpeechASR`
- Success: `{ "text": "..." }`
- Docs: https://api-docs.siliconflow.cn/docs/api/audio-transcriptions-post

This increment only stores and delivers the key/model. Upload is next.

## VibeKey (commit a2ef7cb)

No Wi-Fi STA, no Wi-Fi NVS secrets, no I2S mic, no SiliconFlow client, no
ST7789, no Chinese glyphs. Do not copy Setup UI / Hook / Observer / BLE.

Reusable already in VentureD: EC11 core, bounded serial lines, Chinese
Companion shell, reserved LCD pins 21/47/43/44.
