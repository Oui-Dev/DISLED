# 0016 — Lamp updates: signed firmware, wchisp in the installers

- Status: Accepted
- Date: 2026-10-21
- Amends: [0010](0010-firmware-updates-from-the-app.md)

## Context

ADR 0010 moves the firmware updates into the desktop app: it downloads
the newest `fw-v*` release, checks its SHA-256, asks for BOOT and RESET,
and flashes with `wchisp`, a GPL-2.0 program shipped next to the app.
Building it settled what that record left open.

- `SHA256SUMS` comes from the same release as the `.hex`: it catches a
  corrupted download, not a replaced file. The firmware runs on a USB
  device that the computer trusts, which could for instance present
  itself as a keyboard. It deserves the protection of the app updates
  ([ADR 0014](0014-app-updates.md)).
- wchisp has no machine interface: only its log lines tell what it found
  and how far it got.
- Its license asks for its source to go with the binary.
- The popup hides as soon as it loses the focus, while an update needs
  the user's hands on the lamp, and on Windows a driver from Zadig once.

## Decision

- **Signed firmware.** The firmware release job signs the release build
  (`disled-fw-X.Y.Z.hex`) with the update key of the app, the version
  included in the signed comment. The app checks the signature with the
  public key it carries, and that the signed comment names this file and
  this version. It offers signed releases only: 0.2.1 and older never.
- **When.** The app looks for the newest firmware with the app updates,
  at start-up and once a day, under the same setting, through the GitHub
  releases API (published releases only). The popup offers the update
  when the connected lamp runs an older firmware, after any pending app
  update. It always installs the release build.
- **Where.** A window of its own guides the update: open the lamp, BOOT
  and RESET, Zadig on Windows, then the progress and the result. The
  backend runs the update and the window shows it: closing the window
  stops nothing. The same window installs the firmware on a lamp that
  does not start (no lamp connected).
- **wchisp.** Pinned (0.3.0) and built by the release workflow on each
  runner (`cargo install --locked`; static C runtime on Windows, like the
  app). Only the release builds bundle it, through
  `tauri.wchisp.conf.json`, with its license and a notice; development
  builds use the wchisp on the PATH. The app runs it without a console
  window and reads its log lines (`probe`, then `flash`); the parsing is
  tested against the messages of that version.
- **Source.** Every app release carries the crates.io package of wchisp
  that the workflow builds, checked against the crates.io index, and the
  notice. Its `Cargo.lock` pins the libraries compiled into it, which are
  published on crates.io; their sources are not attached (40 KB instead
  of 21 MB).

## Consequences

- One key signs the app and the firmware: losing it stops both kinds of
  updates.
- The first lamp update the app can make is the first signed firmware
  release, the one after 0.2.1.
- A new wchisp version means checking its messages against
  `crates/disled-flash/src/wchisp.rs` first.
- A firmware that breaks the protocol (a new major version, ADR 0012)
  needs an app release that knows it first; the popup offers the app
  update before the lamp update.
