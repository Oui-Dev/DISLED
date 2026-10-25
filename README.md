# DISLED

A USB status light that sits on top of a monitor. The 4 cm white cube glows
green, orange or red to tell colleagues whether you are available. A touch
on the lid cycles the colors, the brightness follows the room light, and a
desktop app gives full control ([user guide](docs/user-guide.md)).

> **Status:** the product works on the final board (PCB 1.0.0, enclosure
> 1.0.0): the tray app, lamp updates from the app and the production tool
> are released. Still to come: the lamp's own USB IDs, requested from
> pid.codes, then the Mac, current and thermal tests. Binaries and
> hardware files are on the
> [releases page](https://github.com/Oui-Dev/DISLED/releases).

## Repository layout

| Path | Content |
|---|---|
| [`firmware/`](firmware/) | CH32V203 firmware (PlatformIO, C11, TinyUSB) |
| [`app/`](app/) | Desktop app (Tauri 2, Rust, Vue 3 and Nuxt UI), in English and French |
| [`tools/`](tools/) | Production tool, `disled-factory` (Rust) |
| [`crates/`](crates/) | Rust code shared by the app and the tools: protocol codec, flashing |
| [`protocol/`](protocol/) | USB HID protocol specification and shared test vectors |
| [`hardware/`](hardware/) | PCB (EasyEDA Pro, STEP) and enclosure (3MF) design files |
| [`docs/`](docs/) | User guide, architecture, behavior, flashing, bring-up, production and releases |

## Hardware at a glance

- WCH CH32V203G6U6 microcontroller (RISC-V, 144 MHz, 32 KB flash, 10 KB RAM)
- RGB 5050 power LED behind 1 mm of white PLA, driven by three MOSFETs
- Vishay VEML7700 ambient light sensor (I²C)
- Capacitive touch electrode on the lid (CH32V203 TouchKey)
- USB-C for power, flashing (WCH ROM bootloader) and communication

## Getting started

| I want to… | Read |
|---|---|
| Use the lamp and install its app | [docs/user-guide.md](docs/user-guide.md) |
| Contribute a change | [CONTRIBUTING.md](CONTRIBUTING.md) |
| Build, test and flash the firmware | [firmware/README.md](firmware/README.md), [docs/flashing.md](docs/flashing.md) |
| Run the desktop app from source | [app/README.md](app/README.md) |
| Understand the design | [docs/architecture.md](docs/architecture.md) |
| Know what the lamp does | [docs/behavior.md](docs/behavior.md) |
| Talk to the lamp over USB | [protocol/README.md](protocol/README.md) |
| Bring up a new board | [docs/bringup.md](docs/bringup.md) |
| Flash and test a batch of lamps | [docs/production.md](docs/production.md) |
| Publish a release | [docs/releasing.md](docs/releasing.md) |

## Licensing

Software is GPL-3.0-or-later with an additional permission for the WCH SDK.
Hardware is CERN-OHL-S-2.0. See [LICENSING.md](LICENSING.md).
