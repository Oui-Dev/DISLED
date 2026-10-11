# DISLED

A USB status light that sits on top of a monitor. The 4 cm white cube glows
green, orange or red to tell colleagues whether you are available. A touch
on the lid cycles the colors, the brightness follows the room light, and a
desktop app gives full control.

> **Status:** phase 1 (bring-up and MVP) in progress. The v4 boards are
> being manufactured.

## Repository layout

| Path | Content |
|---|---|
| [`firmware/`](firmware/) | CH32V203 firmware (PlatformIO, C11, TinyUSB) |
| [`app/`](app/) | Desktop app (Tauri 2, Rust and Vue 3) |
| [`protocol/`](protocol/) | USB HID protocol specification and shared test vectors |
| [`hardware/`](hardware/) | PCB design files, renders and design review |
| [`docs/`](docs/) | Architecture, behavior, flashing, bring-up and [decision records](docs/decisions/) |

## Hardware at a glance

- WCH CH32V203G6U6 microcontroller (RISC-V, 144 MHz, 32 KB flash, 10 KB RAM)
- RGB 5050 power LED behind 1 mm of white PLA, driven by three MOSFETs
- Vishay VEML7700 ambient light sensor (I²C)
- Capacitive touch electrode on the lid (CH32V203 TouchKey)
- USB-C for power, flashing (WCH ROM bootloader) and communication

## Getting started

| I want to… | Read |
|---|---|
| Build, test and flash the firmware | [firmware/README.md](firmware/README.md), [docs/flashing.md](docs/flashing.md) |
| Run the desktop app | [app/README.md](app/README.md) |
| Understand the design | [docs/architecture.md](docs/architecture.md), [decision records](docs/decisions/) |
| Know what the lamp does | [docs/behavior.md](docs/behavior.md) |
| Talk to the lamp over USB | [protocol/README.md](protocol/README.md) |
| Bring up a new board | [docs/bringup.md](docs/bringup.md) |

## Licensing

Software is GPL-3.0-or-later with an additional permission for the WCH SDK.
Hardware is CERN-OHL-S-2.0. See [LICENSING.md](LICENSING.md).
