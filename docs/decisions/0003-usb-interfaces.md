# 0003 — Vendor HID protocol, CDC console in debug builds only

- Status: Accepted
- Date: 2026-10-12

## Context

The host must read and change the lamp state and receive notifications.
The developer also needs logs and live tuning, without a hardware debugger.

## Decision

- **HID with a vendor usage page (0xFF00)** carries the product protocol.
  It needs no driver on macOS or Windows, needs no special permission, and
  works from Rust (`hidapi`), Python, Swift or WebHID.
- **A CDC ACM serial console** exists only in the `debug` build. It prints
  logs and accepts text commands for tuning. The `release` build is HID
  only.
- **USB identity:** VID:PID `1209:0001` (pid.codes test PID) during phase 1,
  a dedicated pid.codes PID before production. Product string "DISLED",
  serial number from the chip unique ID, `bcdDevice` from the firmware
  version, `bMaxPower` = 300 mA.

## Consequences

- The debug build is a composite device (HID plus CDC with an IAD); the
  release build is smaller.
- The protocol is specified once in `protocol/` with shared test vectors.
