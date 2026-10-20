# 0004 — Updates through the ROM bootloader buttons only

- Status: Superseded by [0010](0010-firmware-updates-from-the-app.md)
- Date: 2026-10-12

## Context

The CH32V203 has a USB bootloader in ROM. It starts when BOOT0 is held
during reset. The BOOT and RESET buttons sit inside the enclosure. A
software jump into that bootloader is documented for other WCH chips, but
one developer reports that the CH32V203 rejects it. A custom bootloader
would cost about 8 KB of the 32 KB flash.

## Decision

Firmware updates use the BOOT and RESET buttons with the enclosure open,
and `wchisp` (PlatformIO `upload_protocol = isp`). There is no custom
bootloader, and no flash is reserved for one.

## Consequences

- The whole 32 KB flash is available to the application.
- Every update requires opening the enclosure.
- The ROM bootloader cannot be erased, so a bad firmware never bricks a
  board.
