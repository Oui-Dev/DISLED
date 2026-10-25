# Production

`disled-factory` ([`tools/disled-factory`](../tools/disled-factory))
flashes, checks and self-tests the boards one after the other, and logs
each one in a CSV file. It runs from the repository: it is not released.
A board takes about 30 seconds.

## Before a batch

- **Rust** (`rustup`) to build the tool, and the repository.
- **wchisp**: `cargo install wchisp@0.3.0 --locked`. Without it, the tool
  takes the one PlatformIO installs for the firmware, or the one given
  with `--wchisp`.
- **Bootloader access**, once per computer. Windows: the WinUSB driver,
  installed with Zadig ([flashing.md](flashing.md#first-time-on-windows-install-the-winusb-driver));
  the tool shows the steps when the driver is missing. Linux: the udev
  rule of the app ([app/README.md](../app/README.md#install)). macOS needs
  nothing.
- **The firmware**: from the firmware release to produce (`fw-vX.Y.Z` on
  the [releases page](https://github.com/Oui-Dev/DISLED/releases)),
  download the release build `disled-fw-X.Y.Z.hex` and its signature
  `disled-fw-X.Y.Z.hex.sig` into the same folder. The tool flashes only a
  release build signed with the update key: the first signed release
  is the one after 0.2.1.
- **USB IDs**: lamps that leave your hands must use the pid.codes PIDs of
  DISLED, not the test PIDs of phase 1
  ([LICENSING.md](../LICENSING.md#usb-identifiers)).

## Run

```sh
cargo run --release -p disled-factory -- path/to/disled-fw-X.Y.Z.hex
```

| Option | Use |
|---|---|
| `--log <file>` | CSV log, appended (default `disled-production.csv`) |
| `--no-flash` | Test lamps that already run their firmware: no bootloader, no flash |
| `--unsigned` | Flash a firmware without signature, at your own risk (below) |
| `--wchisp <path>` | The wchisp program to use |

Stop with Ctrl+C: the log already holds every finished board.

## For each board

1. Plug the board in with **BOOT** held: a blank board starts its
   bootloader only this way. The tool finds the bootloader and flashes:
   erase, write, verify, restart.
2. The lamp starts its firmware. The tool checks what it reports
   (`INFO`): the version of the release, a release build, protocol 1.
3. **Self-test**: the lamp shows red, green, blue and warm white, 1.5 s
   each, while the tool names them. **Touch the lid** when it glows blue
   (10 s). The lamp checks its light sensor and its touch by itself.
4. The tool asks whether the four colors were right; if not, which ones.
5. `PASS` or `FAIL`, one row in the log. Unplug the board: the tool waits
   for the next one.

A board plugged in without BOOT starts its firmware, if it has one: the
tool says so, unplug it and plug it in again with BOOT held.

## Second pass

`--no-flash` tests lamps that already run their firmware, without the
buttons: for instance once the board sits in its enclosure, to check the
touch through the lid, as the lamp will be used. The firmware file still
gives the version the lamps must report.

## The log

One row per board, in `disled-production.csv` unless `--log` says
otherwise:

| Column | Content |
|---|---|
| `date` | End of the test, UTC (RFC 3339) |
| `uid` | Chip unique ID, also the USB serial number |
| `firmware` | Version the lamp reports |
| `signature` | `checked`, or `none` with `--unsigned` |
| `board_revision` | Board revision the firmware reports |
| `colors` | `ok`, or `fail:` and the wrong colors (operator) |
| `light_sensor`, `lux` | The sensor answered with a plausible reading; the light it measured |
| `touch_idle`, `touch_baseline` | Touch reading at rest in the expected range; that reading |
| `touch`, `touch_peak_permille` | A touch was detected; the largest drop seen |
| `result` | `PASS` when every check passed |
| `note` | Why the board failed outside these checks (flash, version), and remarks such as a fault reported before the last reset |

## When a board fails

| Failure | Check |
|---|---|
| The bootloader does not show up | BOOT held while plugging in? A data cable, not a charge-only one? On Windows, the WinUSB driver (the tool says so). |
| wchisp error | Plug the board in again with BOOT held: flashing again is harmless. |
| The lamp does not start | Press RESET, or plug it in again. |
| Wrong firmware | The lamp does not run the given release: check the file, flash again. |
| Light sensor | The sensor and its soldering. |
| Touch at rest out of range | The electrode, its trace and soldering. |
| No touch detected | Touched within the 10 s while blue? Then the electrode. |
| Colors | The LED, its MOSFETs and their soldering. |

The ROM bootloader cannot be erased: a board that failed can always be
flashed and tested again.

## Unsigned firmware

`--unsigned` flashes any firmware file (`.hex`, `.bin` or `.elf`) without
checking it: to try the tool with a local build, or to install a custom
firmware, at your own risk. The tool then checks no version, and the log
marks the rows `signature: none`.
