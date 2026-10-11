# DISLED USB protocol, version 1.0

The lamp is a USB HID device with one vendor-defined interface. The host
sends **commands**; the lamp answers with **messages** and also sends
messages on its own (state changes, telemetry). The lamp owns the lamp
state: the host reads it, changes it and follows its notifications.

> Status: draft for phase 1. It may change until it is frozen in phase 2;
> the major version changes on any incompatible change.

## Transport

| Item | Value |
|---|---|
| USB IDs | `1209:0001` (release builds), `1209:0002` (debug builds); pid.codes test PIDs for phase 1 |
| Interface | HID, usage page `0xFF00` (vendor), usage `0x01` |
| Command report | Output report, ID `1`, 63 bytes |
| Message report | Input report, ID `2`, 63 bytes |
| Endpoints | Interrupt IN and OUT, 64 bytes, polled every 4 ms |

Hosts write commands as output reports (interrupt OUT or SET_REPORT) and
read messages as input reports. With hidapi, a command is written as
`[0x01, payload[0..63]]` and a message reads as `[0x02, payload[0..63]]`.

## Frame

Every payload (the 63 bytes after the report ID) has the same layout:

| Offset | Size | Field |
|---|---|---|
| 0 | 1 | `type` |
| 1 | 1 | `seq`: chosen by the host in a command, echoed in the reply; `0` in unsolicited messages |
| 2 | 1 | `length` of the body, 0 to 60 |
| 3 | `length` | body |
| 3 + `length` | rest | zero |

Multi-byte numbers are little-endian. Booleans are `0` or `1`.

## Commands (host to lamp)

| Type | Name | Body | Reply |
|---|---|---|---|
| `0x01` | `GET_INFO` | none | `INFO` |
| `0x02` | `GET_STATE` | none | `STATE` |
| `0x03` | `SET_STATE` | 5 bytes, see below | `STATE` (the new state) |
| `0x04` | `SET_TELEMETRY` | `u16 period_ms`: 0 stops, otherwise 20 or more | `ACK` |
| `0x05` | `RESET` | none | `ACK`, then the lamp restarts after 50 ms |

A command that cannot be decoded gets an `ERROR` reply and changes
nothing.

### SET_STATE body

| Offset | Field | Values |
|---|---|---|
| 0 | `fields` | bit 0 power, bit 1 color, bit 2 brightness mode, bit 3 manual level; other bits must be 0 |
| 1 | `on` | 0 or 1 |
| 2 | `color` | 0 green, 1 orange, 2 red, 3 warm white |
| 3 | `brightness_mode` | 0 automatic, 1 manual |
| 4 | `manual_level` | 1 to 100 (percent) |

Only the selected fields are applied, and they are applied together or not
at all. Unselected bytes are ignored, whatever their value.

## Messages (lamp to host)

| Type | Name | Sent |
|---|---|---|
| `0x81` | `INFO` | reply to `GET_INFO` |
| `0x82` | `STATE` | reply to `GET_STATE` and `SET_STATE`; unsolicited on every change |
| `0x83` | `TELEMETRY` | every `period_ms` after `SET_TELEMETRY` |
| `0x8E` | `ACK` | reply to `SET_TELEMETRY` and `RESET` |
| `0x8F` | `ERROR` | reply to a rejected command |

### INFO body (56 bytes)

| Offset | Size | Field |
|---|---|---|
| 0 | 1 | protocol major |
| 1 | 1 | protocol minor |
| 2 | 3 | firmware major, minor, patch |
| 5 | 1 | board revision |
| 6 | 12 | chip unique ID, most significant byte first (also the USB serial number) |
| 18 | 4 | capabilities: bit 0 telemetry, bit 1 debug console |
| 22 | 1 | reset cause: 0 unknown, 1 power-on, 2 reset pin, 3 software, 4 watchdog, 5 window watchdog, 6 low power |
| 23 | 1 | flags: bit 0 debug build, bit 1 a fault occurred before the last reset |
| 24 | 4 | fault `mcause` (0 without fault) |
| 28 | 4 | fault `mepc` (0 without fault) |
| 32 | 24 | firmware version text (`git describe`), zero padded, not terminated when 24 characters long |

### STATE body (9 bytes)

| Offset | Size | Field |
|---|---|---|
| 0 | 2 | `state_seq`: increases by one at every change of the other fields |
| 2 | 1 | `on` |
| 3 | 1 | `color` |
| 4 | 1 | `brightness_mode` |
| 5 | 1 | `manual_level` (percent) |
| 6 | 1 | `applied_level`: brightness in use, percent (follows the light in automatic mode) |
| 7 | 1 | USB link: 0 unconfigured, 1 configured, 2 suspended |
| 8 | 1 | `source` of the latest change: 0 boot, 1 touch, 2 host, 3 automatic brightness, 4 USB |

Unsolicited `STATE` messages caused only by automatic brightness are sent
at most every 500 ms. A host that sees `state_seq` jump has missed
messages and should send `GET_STATE`.

### TELEMETRY body (32 bytes)

| Offset | Size | Field |
|---|---|---|
| 0 | 4 | uptime, ms |
| 4 | 2 | touch: last raw TouchKey reading |
| 6 | 2 | touch: window average |
| 8 | 2 | touch: baseline |
| 10 | 2 | touch: signed drop below the baseline, per mille |
| 12 | 1 | touch state: 0 calibrating, 1 idle, 2 pressed |
| 13 | 4 | ambient light, lux x 100 |
| 17 | 2 | ambient light sensor raw count |
| 19 | 6 | PWM duty red, green, blue, in timer ticks (7200 = 100 % at 144 MHz) |
| 25 | 2 | estimated LED current, mA x 10 |
| 27 | 2 | LED current cap in force, mA |
| 29 | 1 | USB link |
| 30 | 2 | brightness level in use, per mille |

Telemetry stops when the USB link leaves the configured state.

### ACK and ERROR

| Message | Body |
|---|---|
| `ACK` | `u8` type of the acknowledged command |
| `ERROR` | `u8` type of the rejected command, `u8` error: 1 unknown command, 2 bad length, 3 bad value |

## Test vectors

[`vectors/v1.json`](vectors/v1.json) lists commands and messages with their
exact bytes. The firmware unit tests (`firmware/test/test_protocol`) and the
desktop app tests (`app/src-tauri`) both check their codec against it, so
the two sides cannot drift apart.

Fields of a vector:

- `bytes`: the significant start of the 63-byte payload, as hexadecimal
  pairs; the rest of the payload is zero.
- commands: `status` (`ok`, `unknown_command`, `bad_length`,
  `bad_value`), `type`, `seq` and, when relevant, `set_state` or
  `telemetry_period_ms`.
- messages: `kind` and the decoded fields.
