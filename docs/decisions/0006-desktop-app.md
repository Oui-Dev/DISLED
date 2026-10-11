# 0006 — Tauri 2 desktop app with Vue 3

- Status: Accepted
- Date: 2026-10-12

## Context

Phase 1 needs a tool to drive the lamp, watch telemetry and record tuning
sessions. The product later needs a desktop app that runs in the
background, starts with the system and keeps user data on the computer.
The developer is a web developer (Vue, Nuxt).

## Decision

Build one Tauri 2 app:

- **Backend (Rust):** USB access through `hidapi`, the protocol codec,
  device discovery and reconnection. The macOS web view has no WebHID, so
  USB access stays in Rust.
- **Frontend:** Vue 3, Vite and TypeScript.
- **Phase 1:** test and tuning tool (controls, live state, telemetry chart,
  CSV export).
- **Phase 2:** product app (menu bar or tray icon, start at login,
  installers).

Integrations such as Slack or Teams are out of scope for now.

## Consequences

- Building the app requires the Rust toolchain (and the MSVC Build Tools on
  Windows).
- Unsigned builds trigger Gatekeeper and SmartScreen warnings. Signing and
  notarization are a later choice.
- The Rust codec is tested against the same vectors as the firmware codec.
