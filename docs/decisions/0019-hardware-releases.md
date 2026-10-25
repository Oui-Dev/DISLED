# 0019 — Hardware releases, by hand and never the latest

- Status: Accepted
- Date: 2026-10-25

## Context

The repository kept each board version as its own file (`v1_…` to
`v4_…epro2`), next to the renders and the design review of v4, and the
enclosure 3MF files in `hardware/housing`. No version could be downloaded
as such, and nothing said which files made one. The firmware and the app
are released by release-please
([ADR 0015](0015-releases-published-complete.md)).

Options considered:

- **release-please packages** for the PCB and the enclosure, versioned
  from the commits. But a hardware version is a decision (a board order,
  a print), not a sum of commits; and with the single release pull
  request, releasing the app would also release any design with pending
  commits.
- **Releases by hand, from a workflow** (chosen).
- **No releases**: the files in the tree only, the older versions in the
  history.

## Decision

- The PCB and the enclosure are two designs with their own versions and
  tags, `pcb-vX.Y.Z` and `enclosure-vX.Y.Z`, outside release-please.
  `hardware/pcb` and `hardware/enclosure` hold the current version only,
  under file names without version or date, and a changelog written by
  hand.
- The **hardware release** workflow, run by hand on `main`, tags the
  commit, attaches the files of the folder (its changelog aside), takes
  the notes from the changelog, and publishes the release complete, as the
  release workflow does.
- A hardware release is never the "Latest" one: that flag stays on the app
  release, where the updater looks ([ADR 0014](0014-app-updates.md)).
- A design that was never manufactured is a pre-release.
- The board release has the EasyEDA Pro project and the STEP model; the
  STEP is versioned in `hardware/pcb` for that.
- The designs made before this decision were released once, from the
  commits that brought them: boards v1 to v3 as pre-releases 0.1.0 to
  0.3.0, the first enclosure as 0.1.0. Board v4 (silkscreen "Rev 1.0") and
  the current enclosure are 1.0.0.

## Consequences

- Commits under `hardware/` release nothing by themselves, whatever their
  type.
- The app is not affected: its updater reads `releases/latest`, and it
  looks for the firmware by its `fw-v` tags.
- The STEP model adds about 3 MB to the repository per board version.
- The decision records and the measurements keep the old names (the v4
  board, the enclosure V2); the hardware README maps them to versions.
