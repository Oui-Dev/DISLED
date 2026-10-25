# DISLED desktop app

Tauri 2 app that lives in the menu bar (macOS) or the notification area
(Windows): a square in the color of the lamp, and a popup under it to
control the lamp. A Diagnostics window, opened on demand, keeps the tuning tools:
live telemetry, CSV recordings, lamp diagnostics and the self-test.

## Install

Installers are attached to each `app-vX.Y.Z` release on the
[releases page](https://github.com/Oui-Dev/DISLED/releases): Windows
(`.exe`), macOS on Apple Silicon (`.dmg`) and Debian or Ubuntu (`.deb`).
Once installed, the app updates itself (0.1.0 has no updater: install a
newer version by hand once).

They carry no publisher certificate, so Windows and macOS warn on first
launch:

- **Windows**: SmartScreen shows "Windows protected your PC": **More
  info**, then **Run anyway**.
- **macOS**: drag DISLED into Applications and open it: macOS refuses
  once. Then open System Settings, Privacy & Security, and click **Open
  Anyway** next to the message about DISLED; confirm with your password.
  The app is signed ad hoc only, which Apple Silicon requires.
- **macOS, app 0.4.0 and older**: macOS says the app "is damaged and can't
  be opened", because those builds had no signature at all. Move the app to
  Applications, then run once in Terminal
  `xattr -cr /Applications/DISLED.app` and open it.

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

- **Tray icon**: shows the lamp (a rounded square in its color, an outline
  when it is off, a faint outline without a lamp). Left click opens the
  popup; right click opens a menu (Open, Diagnostics, Quit). On Linux the
  menu is the only way in: tray click events do not exist there.
- **Popup**: off (grey, struck through) and the four colors, as rounded
  squares; the brightness, automatic with a "darker / brighter" correction
  of the curve or a fixed level (one slider for either, an Auto switch, the
  level in use); a settings page (start at login, daily update checks,
  color mode, language, versions), with the update on offer on top and a
  "⋯" menu for the rare actions (check for updates, Diagnostics, reinstall
  the lamp firmware). It takes the height of its content, and hides when it
  loses the focus or on Escape.
- **Settings saved on the computer** and applied again every time the lamp
  connects (it restarts in automatic brightness with no correction): the
  brightness mode, the fixed level and the correction. Power and color are
  not saved: the lamp always starts green, "available".
- **Color mode and language** ([ADR
  0018](../docs/decisions/0018-nuxt-ui-themes-languages.md)): System, Light
  or Dark; System, English or French. They apply to every window, the
  window frames, the tray menu and its tooltip.
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
- **Diagnostics window**: connection, firmware, serial number, last crash
  and restart on top; on the left, tabs for the lamp controls, its health
  (the diagnostics counters by theme, with a warning on what needs a look)
  and the production self-test; on the right, always in view, telemetry at
  50 ms to 1 s with live charts, and CSV recordings for the tuning campaigns
  of [`docs/bringup.md`](../docs/bringup.md). The stream starts with the
  window, at 250 ms, and the backend stops it when the window closes.

## Front end

Vue 3 with [Nuxt UI](https://ui.nuxt.com) 4 through its Vite plugin (no
Nuxt): its components wherever one fits, charts included. Components are
auto-imported (`src/components.d.ts`, written by the plugin; a file starts
with its folder: `lamp/LampColorPicker.vue` is `<LampColorPicker>`);
functions are imported explicitly, through the `@/` alias of `src/`.

- **State**: one Pinia store per domain in `src/stores`, started by each
  window for what it shows (`src/App.vue`). They talk to the backend
  through `src/ipc`: typed wrappers of the Tauri commands and a typed map
  of the events.
- **Types from Rust**: ts-rs writes the types exchanged with the backend
  to `src/bindings` when the Rust tests run. After a change to one of
  those Rust types, run `npm run bindings`; CI fails when the committed
  files are out of date.

### Design system

`src/assets/css/main.css` brings Tailwind, Nuxt UI, the Montserrat font
(self-hosted) and the token overrides. The two palettes, `ink` (neutral:
from the brand white `#fefcf7`, the light background, to the brand black
`#050a06`, the dark one) and `brand` (primary, on the logo green
`#2ecc71`), come from a script that also checks their contrast. Edit the
script, never `palettes.css`:

```sh
node branding/palette.mjs           # writes src/assets/css/palettes.css
node branding/palette.mjs --check   # what CI runs
```

### Translations

The messages live in `src/locales/en.json` and `src/locales/fr.json`. The
windows read them with [vue-i18n](https://vue-i18n.intlify.dev) (compiled
at build time); the backend reads the `tray` and `window` sections for the
tray menu and the window titles (`src-tauri/src/i18n.rs`), which must stay
plain text. English is the reference: TypeScript rejects a French file
that lacks a message, and `cargo test` checks that both files have the
same keys. Errors from the backend's libraries stay in English, inside a
translated sentence.

To add a language: a JSON file, a variant of the `Language` and `Locale`
enums in Rust, and an entry in the language choices of the popup.

## Layout

| Path                                | Content                                                                             |
| ----------------------------------- | ----------------------------------------------------------------------------------- |
| `../crates/disled-protocol`         | Protocol codec (shared with the tools), tested against `protocol/vectors/v1.json`   |
| `../crates/disled-flash`            | Signed firmware files and wchisp, run as a separate program (shared with the tools) |
| `src-tauri/src/device.rs`           | USB worker thread: discovery, reconnection, settings applied on INFO, tray updates  |
| `src-tauri/src/settings.rs`         | Settings saved on the computer (store plugin) and the commands that apply them      |
| `src-tauri/src/i18n.rs`             | Languages: "System" resolved, the messages of the tray and of the window titles     |
| `src-tauri/src/tray.rs`             | Tray icon (drawn at run time), menu and clicks                                      |
| `src-tauri/src/windows.rs`          | Popup placement, height and hiding, Diagnostics and lamp update windows             |
| `src-tauri/src/updates.rs`          | Update checks (start-up, then daily) and install                                    |
| `src-tauri/src/firmware.rs`         | Lamp updates: newest signed firmware release, download, update steps                |
| `src-tauri/src/lib.rs`              | Plugins, start-up, Tauri commands                                                   |
| `src/views/`                        | One view per window: popup, Diagnostics, lamp update                                |
| `src/components/`                   | Their parts, by domain: `lamp/`, `popup/`, `diagnostics/`, `update/`                |
| `src/stores/`                       | Pinia stores: lamp, settings, telemetry, diagnostics, updates, firmware, errors     |
| `src/ipc/`                          | Typed Tauri commands and events                                                     |
| `src/bindings/`                     | Types generated from the Rust ones (ts-rs, `npm run bindings`)                      |
| `src/locales/`, `src/i18n.ts`       | Messages in English and French (shared with the backend), vue-i18n setup            |
| `src/composables/`, `src/utils/`    | Appearance, popup height, error wording; versions, update steps, CSV, faults        |
| `src/assets/css/`                   | Tailwind, Nuxt UI, the font and the palettes (`palettes.css` is generated)          |
| `src-tauri/tauri.updater.conf.json` | Release builds: also create the signed update files                                 |
| `src-tauri/tauri.wchisp.conf.json`  | Release builds: bundle wchisp, its license and notice (`src-tauri/third-party`)     |
| `scripts/updater-manifest.mjs`      | Writes the `latest.json` of a release (release workflow)                            |
| `branding/`                         | The logo and its installer images (`render.py`), the palettes (`palette.mjs`)       |
| `src-tauri/installer/`              | Windows installer bitmaps and macOS disk image background, made by `render.py`      |

The Rust code is one Cargo workspace at the repository root: this app
(`src-tauri`) and the shared crates (`../crates`). Its build output goes to
`../target`.

The USB access stays in Rust: the macOS web view has no WebHID, and a
backend-owned connection keeps working once the app runs in the background.

## Logo

The logo is the lamp in isometric view, at the enclosure's dimensions, lit
green. `branding/render.py` draws it (`branding/logo.png`) with the
installer images; the app icons come from that logo. After a change, from
the repository root:

```sh
uv run --with numpy --with pillow python app/branding/render.py
cd app && npx tauri icon branding/logo.png
rm -r src-tauri/icons/android src-tauri/icons/ios src-tauri/icons/64x64.png
```

The tray icon is not an image: `src-tauri/src/tray.rs` draws it at run
time in the color of the lamp.

## Checks

```sh
npm run typecheck      # vue-tsc, including the French messages against the English ones
npm run check          # Biome: lint, format, import order (npm run fix applies the fixes)
node branding/palette.mjs --check
cd ..                  # the Rust workspace root
cargo fmt --all --check
cargo clippy --workspace --all-targets -- -D warnings
cargo test --workspace # protocol codec, message keys of both languages, TypeScript types
git diff --exit-code -- app/src/bindings
```
