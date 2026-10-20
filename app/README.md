# DISLED desktop app

Tauri 2 app that lives in the menu bar (macOS) or the notification area
(Windows): a dot in the color of the lamp, and a popup under it to control
the lamp. A Diagnostics window, opened on demand, keeps the tuning tools:
live telemetry, CSV recordings, lamp diagnostics and the self-test.

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

- **Tray icon**: shows the lamp (a dot in its color, a ring when it is off,
  a faint ring without a lamp). Left click opens the popup; right click
  opens a menu (Open, Diagnostics, Quit). On Linux the menu is the only
  way in: tray click events do not exist there.
- **Popup**: power, the four colors, automatic brightness with a "darker /
  brighter" correction, or a fixed level; a settings page (start at login,
  diagnostics, versions). It hides when it loses the focus.
- **Settings saved on the computer** and applied again every time the lamp
  connects (it restarts in automatic brightness with no correction): the
  brightness mode, the fixed level and the correction. Power and color are
  not saved: the lamp always starts green, "available".
- **Start at login**, turned on at the first launch (installed app only);
  one running instance (a second launch shows the popup).
- **Diagnostics window**: connection, firmware, serial number, last fault;
  lamp controls; diagnostics counters (resets, faults, stack margin,
  errors); the production self-test; telemetry at 50 ms to 1 s with live
  charts, and CSV recordings for the tuning campaigns of
  [`docs/bringup.md`](../docs/bringup.md).

## Layout

| Path                        | Content                                                                            |
| --------------------------- | ---------------------------------------------------------------------------------- |
| `../crates/disled-protocol` | Protocol codec (shared with the tools), tested against `protocol/vectors/v1.json`  |
| `src-tauri/src/device.rs`   | USB worker thread: discovery, reconnection, settings applied on INFO, tray updates |
| `src-tauri/src/settings.rs` | Settings saved on the computer (store plugin) and the commands that apply them     |
| `src-tauri/src/tray.rs`     | Tray icon (drawn at run time), menu and clicks                                     |
| `src-tauri/src/windows.rs`  | Popup placement and hiding, Diagnostics window                                     |
| `src-tauri/src/lib.rs`      | Plugins, start-up, Tauri commands                                                  |
| `src/lib/protocol.ts`       | TypeScript mirror of the Rust message types                                        |
| `src/lib/device.ts`         | Store of one window (each window has its own), settings, CSV recorder              |
| `src/components/`           | Popup and Diagnostics views and their panels                                       |

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
