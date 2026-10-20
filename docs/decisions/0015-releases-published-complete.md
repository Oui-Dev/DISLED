# 0015 — Releases are published complete

- Status: Accepted
- Date: 2026-10-21

## Context

release-please published each GitHub release as soon as the release pull
request was merged, and the build jobs attached the binaries afterwards.
The 0.2.0 releases showed two problems:

- The repository has immutable releases: once a release is published, its
  files and its tag are locked, and the name of a deleted release cannot
  be used again. Every upload failed (HTTP 422), so fw 0.2.0 and app 0.2.0
  have no binaries for good. (fw 0.1.0 has none either: its job looked for
  a `.hex` file that the build did not make.)
- Even without immutability, a release was public for about ten minutes
  without binaries, and stayed so when a build failed.

Options considered:

- **Drafts created at the merge**, published once complete (chosen).
- **Drafts filled while the release pull request is open**, published at
  the merge. The binaries could be tried before approving, but every push
  to `main` would rebuild the four targets, the firmware version would
  have to be injected (no tag yet), the binaries would come from the pull
  request branch rather than the tagged commit, and release-please would
  have to be bypassed for tagging and publishing.
- **Immutability turned off**, keeping the old flow: the simplest, but it
  gives up the guarantee that published files are never replaced, and
  keeps the window without binaries.

## Decision

- release-please creates the tag, then the release as a draft
  (`force-tag-creation`, `draft`). Drafts are visible to the maintainers
  only, never to the public nor to the app updater.
- The release workflow builds from the tag, attaches the binaries to the
  draft, and publishes it only once all of them are attached: the firmware
  after its own files, the app after its three installers and
  `latest.json`.
- Immutable releases stay on: GitHub adds an attestation (tag, commit,
  files) to every published release.
- Firmware releases are published without the "Latest" flag, which GitHub
  gives to one release per repository: it stays on the app release, where
  the updater looks ([ADR 0014](0014-app-updates.md)).

## Consequences

- A failed build leaves an unpublished draft: re-run the job, or fix the
  code on `main` for the next version.
- Releases and tags made by release-please are never deleted: release-please
  finds the previous release of each component through them, drafts
  included.
- The first complete releases are the ones after 0.2.0.
