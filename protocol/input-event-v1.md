# VentureD input event protocol v1

The Board C firmware emits a UTF-8 line for every semantic physical input:

```text
VKEY_INPUT/1 {"seq":1,"input":"ENCODER_CW"}
```

`VKEY_INPUT/1` is the fixed resynchronization prefix. The JSON object contains
exactly these fields:

| Field | Type | Meaning |
| --- | --- | --- |
| `seq` | unsigned 32-bit integer | Monotonically increasing firmware event number |
| `input` | string | `ENCODER_CW`, `ENCODER_CCW`, or `ENCODER_PRESS` |

`ENCODER_PRESS` is the stable protocol ID for the independent active-low GPIO8
confirm/action button. It is not an integrated rotary-encoder switch in this
Board C wiring.

Records with an unknown prefix, unknown field, invalid JSON/UTF-8, missing
field, unknown input, or more than 1024 bytes are ignored. A Companion resets
its sequence tracker for each new serial session. Duplicate and backward
records are rejected; forward gaps are reported while the valid record can
continue.

Push-to-talk and microphone data require a new record kind or protocol version.
They must never be represented as an encoder shortcut input.
