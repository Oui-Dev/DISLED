# DISLED USB protocol, version 1.1

The lamp is a USB HID device with one vendor-defined interface. The host
sends **commands**; the lamp answers with **messages** and also sends
messages on its own (state changes, telemetry). The lamp owns the lamp
state: the host reads it, changes it and follows its notifications.

> Status: version 1.1, **frozen** on 2026-10-21 ([ADR 0012](../docs/decisions/0012-protocol-v1-frozen.md)).
> Only the compatible changes listed in [Compatibility](#compatibility)
> remain allowed; anything else needs version 2. The board (PCB 1.0.0) announces
> every capability of 1.1 except `ENTER_BOOTLOADER`, which the CH32V203
> cannot implement ([ADR 0010](../docs/decisions/0010-firmware-updates-from-the-app.md)).

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

Multi-byte numbers are little-endian; signed numbers are two's complement.
Booleans are `0` or `1`.

## Commands (host to lamp)

| Type | Name | Body | Reply |
|---|---|---|---|
| `0x01` | `GET_INFO` | none | `INFO` |
| `0x02` | `GET_STATE` | none | `STATE` |
| `0x03` | `SET_STATE` | 5 bytes, see below | `STATE` (the new state) |
| `0x04` | `SET_TELEMETRY` | `u16 period_ms`: 0 stops, otherwise 20 or more | `ACK` |
| `0x05` | `RESET` | none | `ACK`, then the lamp restarts after 50 ms |
| `0x06` | `SET_AUTO_BIAS` | `i8 bias`, see below | `ACK` |
| `0x07` | `GET_DIAGNOSTICS` | none | `DIAGNOSTICS` |
| `0x08` | `SELF_TEST` | none | `ACK`, then `SELF_TEST_RESULT` when the test ends |
| `0x09` | `ENTER_BOOTLOADER` | none | `ACK`, then the lamp restarts in its ROM bootloader after 50 ms |

A command that cannot be decoded gets an `ERROR` reply and changes
nothing. Commands `0x06` to `0x09` (version 1.1) are optional: each is
announced by a capability bit in `INFO`, and a lamp without it answers
`ERROR` (unknown command).

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

### SET_AUTO_BIAS body

| Offset | Field | Values |
|---|---|---|
| 0 | `bias` (`i8`) | −50 to +50, percent |

Shifts the automatic brightness curve: the level the curve gives for the
current light is multiplied by `(100 + bias) / 100`, then kept within the
lamp's range. 0 restores the factory curve. The lamp forgets the bias when
it restarts: the host sends it again after each connection. Manual mode is
not affected. The current value is reported in `DIAGNOSTICS`.

### SELF_TEST sequence

The lamp shows red, green, blue and white for 1.5 s each at 50 %, checks
the light sensor, then waits up to 10 s for a touch on the lid while it
glows blue at 20 %. It then restores its state and sends
`SELF_TEST_RESULT`, with the `seq` of the `SELF_TEST` command. During the
test the lamp ignores the touch and answers `SET_STATE`, `SET_AUTO_BIAS`
and a second `SELF_TEST` with `ERROR` (busy). Only a person can check the
colors: the result covers the light sensor and the touch.

## Messages (lamp to host)

| Type | Name | Sent |
|---|---|---|
| `0x81` | `INFO` | reply to `GET_INFO` |
| `0x82` | `STATE` | reply to `GET_STATE` and `SET_STATE`; unsolicited on every change |
| `0x83` | `TELEMETRY` | every `period_ms` after `SET_TELEMETRY` |
| `0x84` | `DIAGNOSTICS` | reply to `GET_DIAGNOSTICS` |
| `0x85` | `SELF_TEST_RESULT` | at the end of a self-test |
| `0x8E` | `ACK` | reply to `SET_TELEMETRY`, `RESET`, `SET_AUTO_BIAS`, `SELF_TEST` and `ENTER_BOOTLOADER` |
| `0x8F` | `ERROR` | reply to a rejected command |

### INFO body (56 bytes)

| Offset | Size | Field |
|---|---|---|
| 0 | 1 | protocol major |
| 1 | 1 | protocol minor |
| 2 | 3 | firmware major, minor, patch |
| 5 | 1 | board revision |
| 6 | 12 | chip unique ID, most significant byte first (also the USB serial number) |
| 18 | 4 | capabilities: bit 0 telemetry, bit 1 debug console, bit 2 `SET_AUTO_BIAS`, bit 3 `GET_DIAGNOSTICS`, bit 4 `SELF_TEST`, bit 5 `ENTER_BOOTLOADER` |
| 22 | 1 | reset cause: 0 unknown, 1 power-on, 2 reset pin, 3 software, 4 watchdog, 5 window watchdog, 6 low power |
| 23 | 1 | flags: bit 0 debug build, bit 1 a fault occurred before the last reset |
| 24 | 4 | fault `mcause` (0 without fault) |
| 28 | 4 | fault `mepc` (0 without fault) |
| 32 | 24 | firmware version text (`git describe`), zero padded, not terminated when 24 characters long |

### STATE body (9 bytes)

| Offset | Size | Field |
|---|---|---|
| 0 | 2 | `state_seq`: increases by one with every `STATE` message that carries a change; a reply without change repeats it |
| 2 | 1 | `on` |
| 3 | 1 | `color` |
| 4 | 1 | `brightness_mode` |
| 5 | 1 | `manual_level` (percent) |
| 6 | 1 | `applied_level`: brightness in use, percent (follows the light in automatic mode) |
| 7 | 1 | USB link: 0 unconfigured, 1 configured, 2 suspended |
| 8 | 1 | `source` of the latest change: 0 boot, 1 touch, 2 host, 3 automatic brightness, 4 USB |

Unsolicited `STATE` messages caused only by automatic brightness are sent
at most every 500 ms; changes in between are merged into the next message.
A host that sees `state_seq` jump by more than one has missed a message
and should send `GET_STATE`.

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
| 17 | 2 | ambient light sensor raw count, at the gain and integration time in use (the lamp changes them with the light) |
| 19 | 6 | PWM duty red, green, blue, in whole timer ticks (3600 = 100 %; the firmware dithers to 1/16 tick and reports the rounded value) |
| 25 | 2 | estimated LED current, mA x 10 |
| 27 | 2 | LED current cap in force, mA |
| 29 | 1 | USB link |
| 30 | 2 | brightness level in use, per mille |

Telemetry stops when the USB link leaves the configured state.

### DIAGNOSTICS body (36 bytes)

Counters "since power-up" survive resets (they live in the backup
registers); the others restart at every boot.

| Offset | Size | Field |
|---|---|---|
| 0 | 4 | uptime, ms |
| 4 | 2 | boots since power-up |
| 6 | 2 | watchdog resets since power-up |
| 8 | 2 | faults since power-up |
| 10 | 4 | last fault `mcause` (0 without fault) |
| 14 | 4 | last fault `mepc` |
| 18 | 4 | last fault `mtval` |
| 22 | 2 | stack bytes never used since boot (the smallest margin seen) |
| 24 | 2 | I2C errors since boot |
| 26 | 2 | light sensor probes since boot (1 when it answered at the first try) |
| 28 | 2 | touch samples lost since boot |
| 30 | 2 | touch baseline recalibrations since boot (finger at start-up, object on the lid) |
| 32 | 2 | USB suspends since boot |
| 34 | 1 | automatic brightness bias, `i8` percent |
| 35 | 1 | flags: bit 0 light sensor present, bit 1 crystal running |

### SELF_TEST_RESULT body (9 bytes)

| Offset | Size | Field |
|---|---|---|
| 0 | 1 | results: bit 0 light sensor answered with a plausible reading, bit 1 touch baseline within the expected range, bit 2 a touch was detected, bit 3 the test ran to its end |
| 1 | 4 | ambient light during the test, lux × 100 |
| 5 | 2 | touch baseline, TouchKey counts |
| 7 | 2 | largest touch drop seen while waiting, per mille (`i16`) |

### ACK and ERROR

| Message | Body |
|---|---|
| `ACK` | `u8` type of the acknowledged command |
| `ERROR` | `u8` type of the rejected command, `u8` error: 1 unknown command, 2 bad length, 3 bad value, 4 busy (version 1.1) |

## Compatibility

Within major version 1, a new minor version may only:

- add commands, messages, `INFO` capability bits, error codes or values of
  an enumeration;
- add fields at the end of a body.

To stay compatible with these changes:

- decoders accept a body **longer** than the size given here and ignore
  the extra bytes; a shorter body is an error;
- hosts ignore unsolicited messages of an unknown type, check a
  capability bit before using an optional command, and treat an
  unknown value of an enumeration as an error for that message only;
- lamps answer an unknown command with `ERROR` (unknown command).

Versions 1.0 of the firmware and of the app expect exact body sizes: they
predate these rules.

## Test vectors

[`vectors/v1.json`](vectors/v1.json) lists commands and messages with their
exact bytes. The firmware unit tests (`firmware/test/test_protocol`) and the
Rust codec tests (`crates/disled-protocol`, used by the desktop app) both
check their codec against it, so the two sides cannot drift apart.

Fields of a vector:

- `bytes`: the significant start of the 63-byte payload, as hexadecimal
  pairs; the rest of the payload is zero.
- commands: `status` (`ok`, `unknown_command`, `bad_length`,
  `bad_value`), `type`, `seq` and, when relevant, `set_state`,
  `telemetry_period_ms` or `auto_bias`. `"canonical": false` marks bytes
  that a decoder must accept but an encoder never produces (non-zero
  unselected fields, extra body bytes).
- messages: `kind` and the decoded fields. `"canonical": false` marks a
  message only a decoder is tested with (a body longer than this version
  defines).
