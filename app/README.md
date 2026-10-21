# DISLED desktop app

Tauri 2 app that lives in the menu bar (macOS) or the notification area
(Windows): a dot in the color of the lamp, and a popup under it to control
the lamp. A Diagnostics window, opened on demand, keeps the tuning tools:
live telemetry, CSV recordings, lamp diagnostics and the self-test.

## Install

Installers are attached to each `app-vX.Y.Z` release on the
[releases page](https://github.com/Oui-Dev/DISLED/releases): Windows
(`.exe`), macOS on Apple Silicon (`.dmg`) and Debian or Ubuntu (`.deb`).
They are not signed, so Windows and macOS warn on first launch. Once
installed, the app updates itself (0.1.0 has no updater: install a newer
version by hand once).

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

To try a lamp update from `tauri dev`, the app runs the wchisp on the PATH
(`cargo install wchisp@0.3.0 --locked`). It finds published firmware
signed with the update key only; a debug build can take a local file
instead, unsigned: start it with `DISLED_FIRMWARE_HEX` set to the path of
a `disled-fw-X.Y.Z.hex` (the version comes from the name), then use
**Check for updates** in the settings. Release builds ignore that
variable.

## Features

- **Tray icon**: shows the lamp (a dot in its color, a ring when it is off,
  a faint ring without a lamp). Left click opens the popup; right click
  opens a menu (Open, Diagnostics, Quit). On Linux the menu is the only
  way in: tray click events do not exist there.
- **Popup**: power, the four colors, automatic brightness with a "darker /
  brighter" correction, or a fixed level; a settings page (start at login,
  app and lamp updates, diagnostics, versions). It hides when it loses the
  focus.
- **Settings saved on the computer** and applied again every time the lamp
  connects (it restarts in automatic brightness with no correction): the
  brightness mode, the fixed level and the correction. Power and color are
  not saved: the lamp always starts green, "available".
- **Start at login**, turned on at the first launch (installed app only);
  one running instance (a second launch shows the popup).
- **Updates** ([ADR 0014](../docs/decisions/0014-app-updates.md)): the
  installed app looks for a new version at start-up and once a day (a
  setting turns it off); the popup offers it and installs it on request,
  then restarts. Updates are signed; `tauri dev` never checks by itself.
- **Lamp updates** ([ADR 0016](../docs/decisions/0016-lamp-updates.md)):
  with the app updates, the app looks for the newest firmware release and
  the popup offers it when the lamp runs an older one. A window guides the
  update (BOOT and RESET on the board, the WinUSB driver once on Windows),
  while the backend downloads the firmware, checks its signature (the
  update key), flashes it with [wchisp](https://github.com/ch32-rs/wchisp)
  and checks the version the lamp reports. wchisp is a separate GPL-2.0
  program that the installers put next to the app.
- **Diagnostics window**: connection, firmware, serial number, last fault;
  lamp controls; diagnostics counters (resets, faults, stack margin,
  errors); the production self-test; telemetry at 50 ms to 1 s with live
  charts, and CSV recordings for the tuning campaigns of
  [`docs/bringup.md`](../docs/bringup.md).

## Layout

| Path                                | Content                                                                             |
| ----------------------------------- | ----------------------------------------------------------------------------------- |
| `../crates/disled-protocol`         | Protocol codec (shared with the tools), tested against `protocol/vectors/v1.json`   |
| `../crates/disled-flash`            | Signed firmware files and wchisp, run as a separate program (shared with the tools) |
| `src-tauri/src/device.rs`           | USB worker thread: discovery, reconnection, settings applied on INFO, tray updates  |
| `src-tauri/src/settings.rs`         | Settings saved on the computer (store plugin) and the commands that apply them      |
| `src-tauri/src/tray.rs`             | Tray icon (drawn at run time), menu and clicks                                      |
| `src-tauri/src/windows.rs`          | Popup placement and hiding, Diagnostics and lamp update windows                     |
| `src-tauri/src/updates.rs`          | Update checks (start-up, then daily) and install                                    |
| `src-tauri/src/firmware.rs`         | Lamp updates: newest signed firmware release, download, update steps                |
| `src-tauri/src/lib.rs`              | Plugins, start-up, Tauri commands                                                   |
| `src/lib/protocol.ts`               | TypeScript mirror of the Rust message types                                         |
| `src/lib/device.ts`                 | Store of one window (each window has its own), settings, CSV recorder               |
| `src/lib/updates.ts`                | Update status shown in the popup                                                    |
| `src/lib/firmware.ts`               | Lamp update status, shown by the popup and the lamp update window                   |
| `src/components/`                   | Popup, Diagnostics and lamp update views, and their panels                          |
| `src-tauri/tauri.updater.conf.json` | Release builds: also create the signed update files                                 |
| `src-tauri/tauri.wchisp.conf.json`  | Release builds: bundle wchisp, its license and notice (`src-tauri/third-party`)     |
| `scripts/updater-manifest.mjs`      | Writes the `latest.json` of a release (release workflow)                            |

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
