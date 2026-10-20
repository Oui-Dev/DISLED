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
     Silicon (`.dmg`), Linux `.deb` (Debian, Ubuntu and derivatives); for
     the updater, the macOS app archive (`.app.tar.gz`), one signature
     (`.sig`) per installer and `latest.json`.

While the versions are below 1.0.0, `feat` raises the minor version and a
breaking change does too; `fix` raises the patch version.

## App updates

The installed apps update themselves from the latest GitHub release
([ADR 0014](decisions/0014-app-updates.md)):

- the `app` job signs the installers with the update key (the secrets
  below) and attaches a `.sig` next to each one;
- the `updater` job then writes `latest.json` from these signatures
  (`app/scripts/updater-manifest.mjs`), attaches it to the app release and
  marks that release as the latest;
- the `firmware` job marks the newest app release as the latest again:
  publishing a firmware release takes the flag, and the apps read
  `releases/latest/download/latest.json`.

If the build of one platform failed, re-run its job, then the `updater`
job, so that `latest.json` lists it.

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
- `protocol/`, `crates/` and the root `Cargo.*` files belong to no
  component: a change there is released with the component that the same
  commit touches (a protocol change touches the firmware, the app or both).
- One release pull request for both components: with one per component,
  both edited `.release-please-manifest.json`, so merging one left the
  other in conflict, and release-please does not rebuild a pull request
  whose content did not change. The cost: when both components have
  pending changes, they are released together.
