# Architecture decision records

Each record captures one decision: its context, the choice, and its
consequences. Records are never rewritten; a new record supersedes an old
one.

| # | Decision | Status |
|---|---|---|
| [0001](0001-monorepo.md) | One repository for firmware, app, protocol and hardware | Accepted |
| [0002](0002-firmware-stack.md) | PlatformIO, WCH NoneOS SDK, TinyUSB, C11, bare metal | Accepted |
| [0003](0003-usb-interfaces.md) | Vendor HID protocol, CDC console in debug builds only | Accepted |
| [0004](0004-firmware-updates.md) | Updates through the ROM bootloader buttons only | Superseded by 0010 |
| [0005](0005-licensing.md) | GPL-3.0-or-later with a WCH SDK exception, CERN-OHL-S-2.0 | Accepted |
| [0006](0006-desktop-app.md) | Tauri 2 desktop app with Vue 3 | Accepted |
| [0007](0007-device-behavior.md) | The device owns the lamp state; the host synchronizes | Accepted |
| [0008](0008-toolchain-baseline.md) | Toolchain baseline from the spike (versions, interrupts, clocks) | Accepted, amended by 0009 |
| [0009](0009-bring-up-corrections.md) | Corrections from the first board bring-up (clocks, idle, I2C, LED, touch) | Accepted |
| [0010](0010-firmware-updates-from-the-app.md) | Firmware updates from the desktop app; no software entry into the ROM bootloader | Accepted |
