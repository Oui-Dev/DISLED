# 0010 — Firmware updates from the desktop app

- Status: Accepted, amended by [0016](0016-lamp-updates.md) (signed
  firmware instead of a SHA-256 check)
- Date: 2026-10-21
- Supersedes: [0004](0004-firmware-updates.md)

## Context

Updating a lamp meant building or downloading the firmware, holding BOOT
while pressing RESET, and running `wchisp` by hand. Phase 2 moves this into
the desktop app. The ROM bootloader stays the only way in: there is no room
for a custom bootloader in 32 KB, and the ROM one can never be erased.

Whether the firmware can start the ROM bootloader by itself, without the
buttons, was tested on the first board (2026-10-21):

- **Boot mode switch.** CH32V00x and X035 parts select the ROM at the next
  software reset through `BOOT_MODE` (bit 14 of `FLASH->STATR`), unlocked
  by a key register right after `MODEKEYR`. The CH32V20x SDK and reference
  manual have neither, and on the CH32V203 the bit reads back as 0 after
  the unlock sequence: the switch does not exist.
- **Jump into the system flash** (`0x1FFF8000`), after a reset so that the
  watchdog is not running yet. Two findings:
  - `main()` runs in **user mode**: the startup code sets `mstatus = 0x88`
    (MPP = 0) before its `mret`. The ROM starts by writing machine-only
    CSRs (`0xBC0`), so the jump must come from an exception handler (an
    `ecall`), which then `mret`s into the ROM in machine mode.
  - Even so the ROM fails: it is linked to run at address 0, where BOOT0
    maps it at reset, and computes its RAM addresses relative to its own
    position (`auipc`). Run from `0x1FFF8000`, its stack and data land at
    `0x3FFF8000`, where there is no memory, and it hangs without enabling
    USB.

The ROM can therefore only start from a reset with BOOT0 held high.

## Decision

- The desktop app updates the firmware: it finds the latest `fw-v*`
  release, downloads the `.hex`, checks its SHA-256, asks the user to hold
  BOOT and press RESET, then flashes with `wchisp` and checks the version
  the lamp reports.
- `wchisp` (GPL-2.0-only, which our GPL-3.0-or-later code cannot link) is
  shipped next to the app as a separate executable (a Tauri sidecar), with
  its source attached to every app release.
- The `ENTER_BOOTLOADER` command of the protocol stays defined but this
  board never announces it: a future board or chip with a boot mode switch
  can use it without a protocol change.

## Consequences

- Updates still require opening the enclosure to reach the buttons; the
  app removes every other step.
- On Windows the bootloader needs the WinUSB driver once (Zadig); the app
  explains it when `wchisp` cannot open the device.
- Code that needs machine mode (CSR writes, the bootloader jump) must run
  in an exception handler: worth knowing for any future low-level work.
