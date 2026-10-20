# DISLED firmware

Firmware of the CH32V203G6U6 on the DISLED board: lamp state machine,
capacitive touch, ambient light, LED output with a current cap, and a USB
HID protocol. C11 on the WCH NoneOS SDK and TinyUSB, built with PlatformIO.

## Quick start

```sh
git submodule update --init          # TinyUSB, once after cloning
pio run -e debug                     # build (debug: HID + CDC console)
pio run -e debug -t upload           # flash through the ROM bootloader
pio device monitor                   # debug console, type "help"
pio test -e native                   # unit tests on the build machine
pio check -e debug                   # static analysis (cppcheck)
```

The unit tests need a host C compiler (`gcc`). CI runs them on every pull
request into `main`; on Windows, a MinGW-w64 build such as WinLibs works.

See [docs/flashing.md](../docs/flashing.md) for the bootloader, the
Windows driver and troubleshooting.

## Environments

| Environment | Content |
|---|---|
| `debug` (default) | HID protocol + CDC console and logs, USB ID `1209:0002` |
| `release` | HID protocol only, no logs, USB ID `1209:0001` |
| `native` | Unity tests of `lib/disled_core`, with the protocol vectors |

## Layout

| Path | Content |
|---|---|
| `src/main.c` | entry point |
| `src/app/` | wiring and tasks (`app.c`), host protocol (`app_host.c`), console commands, **all tunables** (`app_config.c`) |
| `src/bsp/` | pin map (`board.h`), clocks, time base, watchdog, faults, unique ID |
| `src/drivers/` | LED PWM, TouchKey, I2C, VEML7700 |
| `src/usb/` | TinyUSB configuration, descriptors, link state and report queues |
| `src/diag/` | logs and console (debug builds) |
| `lib/disled_core/` | hardware-independent logic, unit tested |
| `lib/tinyusb/` | TinyUSB 0.21.0 (submodule `upstream/`) and its build description |
| `test/` | Unity tests, one folder per core module |
| `scripts/` | version from git, memory budget, Intel HEX, protocol vectors to C |
| `requirements.txt` | CI tools (PlatformIO, clang-format), kept up to date by Dependabot |

The design is explained in [docs/architecture.md](../docs/architecture.md);
the lamp behavior in [docs/behavior.md](../docs/behavior.md).

## Conventions

- C11 (GNU dialect for the SDK headers), formatted with clang-format 23
  (`.clang-format`), warnings are errors on our sources.
- Every source file starts with its SPDX header (GPL-3.0-or-later with the
  WCH SDK permission, see [LICENSING.md](../LICENSING.md)).
- No dynamic allocation, no floating point, no 64-bit division; fixed
  point with static assertions on ranges.
- Interrupt handlers use `BSP_ISR` and only capture data.
- Includes are relative to `src/`, so each one names the layer it uses.
- The firmware version comes from the latest `fw-vX.Y.Z` tag.
