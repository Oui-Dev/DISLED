# Flashing the firmware

DISLED is flashed over USB through the bootloader in the CH32V203 ROM. That
bootloader cannot be erased, so a bad firmware never bricks a board: the
BOOT button always brings it back. The buttons sit on the board, so the
enclosure has to be open ([ADR 0010](decisions/0010-firmware-updates-from-the-app.md)).

## From the desktop app

The app offers a new firmware in its popup when the connected lamp runs an
older one (Settings, **Update the lamp**), and guides the update in a
window of its own. It downloads the release build, checks its signature
([ADR 0016](decisions/0016-lamp-updates.md)), asks for BOOT and RESET,
flashes the lamp and checks the version it then reports. On Windows it
also shows the driver steps below when they are needed.

A lamp that no longer starts gets its firmware the same way: Settings,
**Lamp not detected? Reinstall its firmware**.

The rest of this page is for development and for flashing by hand.

## Enter the bootloader

1. Plug the board into the computer (directly or through a hub).
2. Hold **BOOT**, press and release **RESET**, then release **BOOT**.
   Holding BOOT while plugging the cable in works too.
3. The chip now shows up as a WCH device instead of DISLED. Flash it
   right away.

## First time on Windows: install the WinUSB driver

PlatformIO and the desktop app flash with
[`wchisp`](https://github.com/ch32-rs/wchisp), which talks to the
bootloader through WinUSB.

1. Enter the bootloader (above).
2. Run [Zadig](https://zadig.akeo.ie/), then **Options → List All Devices**.
3. Select the WCH device. Its USB ID should be `4348:55E0`; write down the
   ID you actually see in the bring-up notes.
4. Choose **WinUSB** and click **Replace Driver** (or **Install Driver**).

This replaces the official WCH driver for that device, so WCH's own
WCHISPTool no longer sees it. That is expected.

macOS needs no driver.

## Flash

From `firmware/`:

```sh
pio run -e debug -t upload     # development firmware: HID + CDC console
pio run -e release -t upload   # production firmware: HID only
```

To flash a published build instead, download `disled-fw-X.Y.Z.hex` (or
`-debug.hex`) from the [releases page](https://github.com/Oui-Dev/DISLED/releases)
and use [wchisp](https://github.com/ch32-rs/wchisp) directly (PlatformIO
installs it in `~/.platformio/packages/tool-wchisp`):

```sh
wchisp flash disled-fw-X.Y.Z.hex
```

After flashing, press **RESET** (or unplug and plug back in). The lamp
starts green and shows up as:

| Build | USB ID | Interfaces |
|---|---|---|
| debug | `1209:0002` | HID "DISLED control" + serial port "DISLED console" |
| release | `1209:0001` | HID "DISLED control" |

## Debug console

In debug builds, open the serial port with any terminal (the baud rate is
ignored):

```sh
pio device monitor          # PlatformIO picks the port
```

Type `help` for the list of commands:

| Command | Use |
|---|---|
| `info`, `state` | firmware, chip, fault; lamp, USB link and LED output |
| `set on\|off\|auto\|color <0-3>\|level <1-100>\|bias <-50..50>` | change the lamp as the host does |
| `watch <ms>\|off` | stream touch, light and current (`watch 20`: every touch window) |
| `diag`, `tasks` | diagnostics counters; longest run of each task |
| `selftest` | production self-test (colors, light sensor, touch) |
| `touch`, `palette`, `pulse`, `cap`, `i2c` | tuning and bring-up tools |
| `log <0-3>`, `reset` | log level; restart |

## Without PlatformIO

Any WCH ISP tool can flash `firmware.bin` or `firmware.hex` from
`firmware/.pio/build/<env>/`, or from the CI artifacts: `wchisp flash
firmware.bin`, WCHISPTool on Windows, or WCHISPStudio on macOS.

## Troubleshooting

| Symptom | Check |
|---|---|
| `wchisp` finds no device | Bootloader entered? WinUSB installed with Zadig for the WCH device? Data cable, not a charge-only cable? |
| The lamp stays dark after flashing | Press RESET: the bootloader keeps running until a reset. |
| Windows shows an old interface set | Uninstall the device in Device Manager; debug and release builds use different PIDs to avoid this. |
| The console port does not appear | Only debug builds have it. |
