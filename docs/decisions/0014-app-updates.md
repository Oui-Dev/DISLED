# 0014 — App updates through GitHub Releases

- Status: Accepted
- Date: 2026-10-21

## Context

The app lives in the tray and starts with the computer: its users never
visit the releases page again after the first install. Fixes and firmware
updates (ADR 0010) reach them only if the app updates itself. The
installers are not code-signed (ADR 0006), and the project has no server
of its own.

## Decision

- The Tauri updater plugin, fed by GitHub Releases. The release workflow
  signs every installer with an update key and publishes a `latest.json`
  manifest on the app release, built once all the installers are attached
  (`app/scripts/updater-manifest.mjs`). The app reads
  `releases/latest/download/latest.json`.
- The app release is the repository's "latest" release: firmware
  releases are published without that flag
  ([ADR 0015](0015-releases-published-complete.md)).
- Signatures carry the version they were made for, and the app refuses an
  update whose signature does not match the announced version
  (`requireSignedVersion`). A forged manifest cannot pair a new version
  number with an old, genuinely signed installer to force a downgrade.
- The private key and its password are GitHub secrets, kept outside the
  repository; the public key is in `tauri.conf.json`.
- The app checks at start-up (after 30 s) and then once a day, unless the
  user turns it off; debug builds never check by themselves. The popup
  offers the update and installs it only when the user asks.
- Every installer updates itself: the NSIS installer on Windows (passive,
  progress bar only), the app bundle on macOS, the `.deb` on Linux (the
  system asks for the administrator password).

## Consequences

- Losing the private key or its password ends automatic updates: the
  installed apps accept only that key. Keep a backup of both. A new key
  means a manual reinstall for every user.
- Releases built without the signing secrets fail: no installer is ever
  published unsigned.
- The checks contact GitHub once a day (an anonymous download of
  `latest.json`); the setting turns them off.
- The first app release with the updater must be installed by hand once
  (0.1.0 has no updater, 0.2.0 no binaries).
