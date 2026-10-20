# 0012 — USB protocol version 1 frozen at 1.1

- Status: Accepted
- Date: 2026-10-21

## Context

Phase 1 left the HID protocol as a draft. Phase 2 needed four new commands
(automatic brightness bias, diagnostics, self-test, bootloader entry), and
the lamps in the field will meet desktop apps of other versions. The
phase 1 decoders (firmware 0.1.0, app 0.1.0) expect exact body sizes, so
any change to an existing message would have broken them.

## Decision

- Version 1.1 adds only optional commands and messages, each announced by
  a capability bit in `INFO`, and an error code ("busy"). Existing message
  bodies keep their size: the bias is acknowledged with `ACK` and read back
  in `DIAGNOSTICS` rather than added to `STATE`.
- The specification ([protocol/README.md](../../protocol/README.md))
  states the compatibility rules: within version 1, a minor version may
  only add commands, messages, capability bits, error codes, enumeration
  values, or fields at the end of a body. Decoders accept longer bodies;
  hosts ignore unknown unsolicited messages and check capability bits.
- With the firmware implementing 1.1 and checked on the board, version 1 is
  frozen: anything incompatible needs version 2.

## Consequences

- An app that knows 1.1 works with any 1.x lamp, using only what `INFO`
  announces; a 1.x lamp works with an older 1.x app for everything that
  app knows.
- The shared vectors (`protocol/vectors/v1.json`) cover the new commands,
  the new messages and the longer-body rule, in the firmware and the Rust
  tests alike.
