# 0007 — The device owns the lamp state; the host synchronizes

- Status: Accepted
- Date: 2026-10-12

## Context

The lamp must work on its own (touch, automatic brightness) and still give
the desktop app full control.

## Decision

- The firmware holds the lamp state machine. The touch changes it locally;
  the host reads it, changes it and receives a notification for every
  change.
- Nothing is stored in flash. At power-up the lamp is on, green, with
  automatic brightness. The app applies its saved settings again when it
  connects.
- The full behavior is specified in [behavior.md](../behavior.md).

## Consequences

- The lamp works on a plain USB charger.
- The app treats the device as the source of truth and must handle
  notifications in any order through a state sequence number.
- Flash wear is not a concern.
