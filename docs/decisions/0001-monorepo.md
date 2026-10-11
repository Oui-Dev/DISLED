# 0001 — One repository for firmware, app, protocol and hardware

- Status: Accepted
- Date: 2026-10-12

## Context

The firmware and the desktop app share a USB protocol. A protocol change
touches the specification, the C codec, the Rust codec and their test
vectors at once. The PCB design files already live next to the firmware.

## Decision

Use a single repository named `disled`:

- `firmware/`, `app/`, `protocol/`, `hardware/` and `docs/` at the root;
- one CI workflow per component, triggered only by changes in its folders
  (and in `protocol/`);
- separate version tags per component: `fw-vX.Y.Z` and `app-vX.Y.Z`.

Large generated files (STEP models) and local tools (`.claude/`,
`easyeda-api-skill/`) stay out of git.

## Consequences

- A protocol change lands in one commit, so firmware, app and test vectors
  never drift apart.
- Issues, CI and documentation live in one place.
- CI must filter by path so that an app change does not rebuild the
  firmware.
- The repository stays private during phase 1 and becomes public before the
  USB PID request (see [0005](0005-licensing.md)).
