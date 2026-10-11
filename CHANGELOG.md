# Changelog

All notable changes are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/). The firmware and
the desktop app are versioned separately with [SemVer](https://semver.org/),
using the tags `fw-vX.Y.Z` and `app-vX.Y.Z`.

## [Unreleased]

### Added

- Monorepo layout, licensing and architecture decision records.
- Firmware (phase 1 MVP): lamp state machine, capacitive touch with short
  and long press, automatic brightness from the VEML7700, LED output with
  CIE L* dimming, fades and a current cap per USB state, USB HID protocol
  v1, CDC debug console, watchdog and fault reports.
- Protocol v1 specification and shared test vectors.
- Desktop app (Tauri 2): lamp controls, live telemetry, CSV recording.
- CI: firmware format, unit tests, builds, static analysis; app type
  check, lint, format, clippy and tests.
- Documentation: architecture, behavior, flashing, bring-up campaigns.
