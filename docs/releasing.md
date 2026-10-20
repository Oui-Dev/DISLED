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
3. Merge the release pull request when you want to release.
   release-please then tags the merge commit for each bumped component and
   creates one **draft** release per component with its changelog: drafts
   are visible to the maintainers only. The workflow builds each tag,
   attaches the binaries to its draft, and publishes the release only once
   all of them are attached:
   - **firmware:** release and debug builds as `.hex`, `.bin` and `.elf`,
     with `SHA256SUMS`;
   - **app:** Windows installer (`.exe`), macOS disk image for Apple
     Silicon (`.dmg`), Linux `.deb` (Debian, Ubuntu and derivatives); for
     the updater, the macOS app archive (`.app.tar.gz`), one signature
     (`.sig`) per installer and `latest.json`.

   Published releases are immutable (repository setting, Settings,
   General, Releases): their files and tag can no longer change, only their
   title and notes ([ADR 0015](decisions/0015-releases-published-complete.md)).

While the versions are below 1.0.0, `feat` raises the minor version and a
breaking change does too; `fix` raises the patch version.

## When a release build fails

The release stays a draft, with its tag: nothing is published.

- If the cause is outside the code (runner, secret), re-run the failed
  jobs; the publication follows.
- If the code must change, fix it on `main`: the fix goes out with the
  next version. Leave the unpublished draft as it is.
- Never delete a release or a tag made by release-please: it finds the
  previous release of each component through them (drafts included), and
  without it the next changelog would take the whole history. The name of
  a deleted published release cannot be used again either.

## The "Latest" release

GitHub marks one release per repository as "Latest", whatever the
component. It is always the newest app release: the installed apps read
`releases/latest/download/latest.json`. Firmware releases are published
without the flag; the app finds the newest firmware by its `fw-v*` tag.

## App updates

The installed apps update themselves from the latest GitHub release
([ADR 0014](decisions/0014-app-updates.md)):

- the `app` job signs the installers with the update key (the secrets
  below) and attaches a `.sig` next to each one;
- once the three installers are attached, the `app-publish` job writes
  `latest.json` from these signatures (`app/scripts/updater-manifest.mjs`),
  attaches it, and publishes the release as the latest.

Local builds (`npm run tauri build`) need no key: only the release
workflow adds `src-tauri/tauri.updater.conf.json`, which asks for the
update files.

### Update key (once)

1. Create the key pair outside the repository, with a password:

   ```sh
   cd app
   npm run tauri signer generate -- -w ~/.tauri/disled-updater.key
   ```

   In PowerShell, write the path `$env:USERPROFILE\.tauri\disled-updater.key`.

2. In the repository settings (Secrets and variables, Actions), add two
   repository secrets: `TAURI_SIGNING_PRIVATE_KEY`, the content of
   `disled-updater.key`, and `TAURI_SIGNING_PRIVATE_KEY_PASSWORD`, its
   password.
3. Put the content of `disled-updater.key.pub` in `plugins.updater.pubkey`
   of `app/src-tauri/tauri.conf.json`.
4. Keep a backup of the private key and its password, outside the
   computer: the installed apps accept no other key, and a new key means
   a manual reinstall for every user.

## Notes

- The installers are not code-signed (the update signatures above only
  serve the updater): Windows SmartScreen and macOS Gatekeeper warn on
  first launch (right-click, then Open, on macOS). Code signing is a later
  choice ([ADR 0006](decisions/0006-desktop-app.md)).
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
- Dependabot opens weekly update pull requests (`.github/dependabot.yml`)
  with Conventional Commit messages: `fix(deps)` for what the app ships
  (npm production dependencies, Rust crates), which releases the app;
  `chore(deps-dev)` (app tooling), `build(deps)` (firmware tools) and
  `ci(deps)` (actions) release nothing. An update of the root `Cargo.lock`
  alone belongs to no component: it goes out with the next app release.
- `protocol/`, `crates/` and the root `Cargo.*` files belong to no
  component: a change there is released with the component that the same
  commit touches (a protocol change touches the firmware, the app or both).
- One release pull request for both components: with one per component,
  both edited `.release-please-manifest.json`, so merging one left the
  other in conflict, and release-please does not rebuild a pull request
  whose content did not change. The cost: when both components have
  pending changes, they are released together.
