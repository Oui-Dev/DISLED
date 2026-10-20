# Releasing

Releases are automated with
[release-please](https://github.com/googleapis/release-please). The
firmware and the desktop app are released separately, each with its own
version, tag and changelog:

| Component | Path | Tag | Changelog | Version source |
|---|---|---|---|---|
| Firmware | `firmware/` | `fw-vX.Y.Z` | [firmware/CHANGELOG.md](../firmware/CHANGELOG.md) | the tag (`scripts/version.py`); `version.txt` only tracks it for release-please |
| Desktop app | `app/` | `app-vX.Y.Z` | [app/CHANGELOG.md](../app/CHANGELOG.md) | `app/package.json`, read by `tauri.conf.json` |

## Flow

1. Work on a branch and merge it into `main` with a pull request. Commit
   messages follow [Conventional Commits](https://www.conventionalcommits.org/):
   `feat` adds a feature, `fix` fixes a bug, `feat!` or a
   `BREAKING CHANGE:` footer breaks compatibility. `docs`, `chore`, `ci`,
   `test` and `refactor` do not trigger a release.
2. On each push to `main`, the `release` workflow opens or updates a
   single release pull request. It bumps only the components that have
   releasable commits, with their next version (computed from the
   commits) and their changelog entries. Commits are assigned to a
   component by the files they change.
3. Merge the release pull request when you want to release. The workflow
   then tags the merge commit for each bumped component, publishes one
   GitHub release per component with its changelog, and attaches the
   binaries:
   - **firmware:** release and debug builds as `.hex`, `.bin` and `.elf`,
     with `SHA256SUMS`;
   - **app:** Windows installer (`.exe`), macOS disk image for Apple
     Silicon (`.dmg`), Linux `.deb` (Debian, Ubuntu and derivatives).

While the versions are below 1.0.0, `feat` raises the minor version and a
breaking change does too; `fix` raises the patch version.

## Notes

- The installers are not signed: Windows SmartScreen and macOS Gatekeeper
  warn on first launch (right-click, then Open, on macOS). Signing is a
  phase 2 decision ([ADR 0006](decisions/0006-desktop-app.md)).
- On a private repository the macOS and Windows runners consume the free
  Actions minutes 10 and 2 times faster; an app release costs about 150 to
  200 minutes. Public repositories have no such limit.
- The repository must allow the workflow to open pull requests: Settings,
  Actions, General, Workflow permissions: **Allow GitHub Actions to create
  and approve pull requests**. The default token permissions can stay
  read-only: the release workflow asks for write access itself, and the
  other workflows only need to read.
- To force a version, add `Release-As: X.Y.Z` to the body of a commit that
  touches the component.
- One release pull request for both components: with one per component,
  both edited `.release-please-manifest.json`, so merging one left the
  other in conflict, and release-please does not rebuild a pull request
  whose content did not change. The cost: when both components have
  pending changes, they are released together.
