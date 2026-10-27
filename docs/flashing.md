# Flashing the firmware

You flash DISLED over USB, through the bootloader in the ROM of the
CH32V203. That bootloader cannot be erased, so a bad firmware never bricks
a board, because the BOOT button always brings it back. The buttons sit on
the board, so the enclosure has to be open. The CH32V203 starts its
bootloader only from a reset with BOOT held, never from software.

## From the desktop app

The desktop app offers a new firmware when the connected lamp runs an older
one, and guides the update in a window of its own, the Windows driver
included ([user guide](user-guide.md#updates)). A lamp that no longer starts
gets its firmware the same way, from **⋯**, **Reinstall the lamp
firmware…** on the settings page of the popup.

The rest of this page is for development and for flashing by hand.

## Enter the bootloader

1. Plug the board into the computer, directly or through a hub.
2. Hold **BOOT**, press and release **RESET**, then release **BOOT**.
   Holding BOOT while plugging the cable in works too.
3. The chip now shows up as a WCH device instead of DISLED. Flash it
   right away.

## First time on Windows: install the WinUSB driver

PlatformIO and the desktop app flash with
[`wchisp`](https://github.com/ch32-rs/wchisp), which talks to the
bootloader through WinUSB.

1. Enter the bootloader (above).
2. Run [Zadig](https://zadig.akeo.ie/), then **Options**, **List All
   Devices**.
3. Select the WCH device, whose USB ID is `4348:55E0` or `1A86:55E0`.
4. Choose **WinUSB** and click **Replace Driver** or **Install Driver**.

This replaces the official WCH driver for that device, so WCH's own
WCHISPTool no longer sees it. That is expected.

macOS needs no driver.

## Flash

From `firmware/`:

```sh
pio run -e debug -t upload     # development firmware: HID + CDC console
pio run -e release -t upload   # production firmware: HID only
```

To flash a published build instead, download `disled-fw-X.Y.Z.hex`, or
`disled-fw-X.Y.Z-debug.hex`, from the
[releases page](https://github.com/Oui-Dev/DISLED/releases) and run
[wchisp](https://github.com/ch32-rs/wchisp) directly. PlatformIO installs
it in `~/.platformio/packages/tool-wchisp`:

```sh
wchisp flash disled-fw-X.Y.Z.hex
```

After flashing, press **RESET**, or unplug the board and plug it back in.
The lamp starts green and shows up as:

| Build | USB ID | Interfaces |
|---|---|---|
| debug | `1209:0002` | HID "DISLED control" + serial port "DISLED console" |
| release | `1209:0001` | HID "DISLED control" |

## Debug console

A debug build adds a serial port. Open it with any terminal, at any baud
rate:

```sh
pio device monitor          # PlatformIO picks the port
```

Type `help` for the list of commands:

| Command | Use |
|---|---|
| `info`, `state` | firmware, chip, fault; lamp, USB link and LED output |
| `set on\|off\|auto\|color <0-3>\|level <1-100>\|bias <-50..50>` | change the lamp as the host does |
| `watch <ms>\|off` | stream touch, light (lux, count, level) and current (`watch 20`: every touch window) |
| `diag`, `tasks` | diagnostics counters; longest run of each task |
| `selftest` | production self-test (colors, light sensor, touch) |
| `cap <mA>\|off` | override the LED current cap, for the heat test of [bringup.md](bringup.md#8-heat) only |
| `log <0-2>`, `reset` | log level; restart |

## Without PlatformIO

Any WCH ISP tool can flash `firmware.bin` or `firmware.hex`, from
`firmware/.pio/build/<env>/` or from the CI artifacts: `wchisp flash
firmware.bin`, WCHISPTool on Windows, or WCHISPStudio on macOS.

For a batch of boards, `disled-factory` flashes and tests them one after
the other ([production.md](production.md)).

## Troubleshooting

| Symptom | Check |
|---|---|
| `wchisp` finds no device | The board is in its bootloader, Zadig installed WinUSB for the WCH device on Windows, and the cable carries data. |
| The lamp stays dark after flashing | Press RESET. The bootloader keeps running until a reset. |
| Windows shows an old interface set | Uninstall the device in Device Manager. Windows caches the interfaces per USB ID, which is why debug and release builds use different PIDs. |
| The console port does not appear | Only debug builds have it. |
