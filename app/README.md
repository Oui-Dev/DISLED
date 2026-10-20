# DISLED desktop app

Tauri 2 app that drives the lamp over USB and records measurements for
tuning. Phase 1 scope: test and tuning tool. Phase 2 turns it into the
product app (menu bar or tray icon, start at login, installers).

## Install

Installers are attached to each `app-vX.Y.Z` release on the
[releases page](https://github.com/Oui-Dev/DISLED/releases): Windows
(`.exe`), macOS on Apple Silicon (`.dmg`) and Debian or Ubuntu (`.deb`).
They are not signed, so Windows and macOS warn on first launch.

On Linux, only root may open a USB HID device by default. The `.deb`
installs a udev rule, `/usr/lib/udev/rules.d/70-disled.rules`
([source](src-tauri/linux/70-disled.rules)), that gives the logged-in user
access to the lamp and to the WCH bootloader (for `wchisp`). To run the
app from source on Linux, install the rule once by hand:

```sh
sudo cp src-tauri/linux/70-disled.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
```

## Run it

Prerequisites: Node.js 24 or later, the Rust toolchain (`rustup`) and, on
Windows, the Visual Studio Build Tools with the C++ workload (macOS: Xcode
command line tools).

```sh
npm install
npm run tauri dev
```

The app finds the lamp by itself (USB IDs `1209:0001` and `1209:0002`) and
reconnects when it is plugged back in. No driver is needed.

## Features

- Connection status, firmware version, serial number, last reset cause and
  fault report.
- Lamp controls: power, the four colors, automatic or manual brightness.
  The lamp owns the state; the controls follow its notifications, including
  touches on the lid.
- Telemetry at 50 ms to 1 s: touch reading, baseline and signal, ambient
  light, LED current and cap, PWM duties, with live charts.
- Recording of the telemetry to CSV, for the tuning campaigns described in
  [`docs/bringup.md`](../docs/bringup.md).

## Layout

| Path                        | Content                                                                           |
| --------------------------- | --------------------------------------------------------------------------------- |
| `../crates/disled-protocol` | Protocol codec (shared with the tools), tested against `protocol/vectors/v1.json` |
| `src-tauri/src/device.rs`   | USB worker thread: discovery, reconnection, reads and writes                      |
| `src-tauri/src/lib.rs`      | Tauri commands (`device_connection`, `device_send`, `save_csv`)                   |
| `src/lib/protocol.ts`       | TypeScript mirror of the Rust message types                                       |
| `src/lib/device.ts`         | Device store shared by the components, CSV recorder                               |
| `src/components/`           | Header, lamp panel, telemetry panel, canvas line chart                            |

The Rust code is one Cargo workspace at the repository root: this app
(`src-tauri`) and the shared crates (`../crates`). Its build output goes to
`../target`.

The USB access stays in Rust: the macOS web view has no WebHID, and a
backend-owned connection keeps working once the app runs in the background.

## Checks

```sh
npm run typecheck      # vue-tsc
npm run lint           # ESLint
npm run format:check   # Prettier
cd ..                  # the Rust workspace root
cargo fmt --all --check
cargo clippy --workspace --all-targets -- -D warnings
cargo test --workspace # protocol codec against the shared vectors
```
