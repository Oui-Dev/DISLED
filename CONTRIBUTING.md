# Contributing to DISLED

DISLED is a firmware, a desktop app, a few shared Rust crates and the
hardware design files, in one repository.
This page explains how to propose a change and what the checks expect.
For a bug or an idea, open an [issue](https://github.com/Oui-Dev/DISLED/issues)
first, unless the fix is small; the [user guide](docs/user-guide.md#get-help)
says what a bug report needs.

## Before you start

Clone with the TinyUSB submodule:

```sh
git clone --recurse-submodules https://github.com/Oui-Dev/DISLED.git
```

(or `git submodule update --init` in an existing clone). Then set up the
part you work on:

| Part | Tools | Setup |
|---|---|---|
| Firmware | PlatformIO and clang-format through [uv](https://docs.astral.sh/uv/), a host C compiler for the unit tests | [firmware/README.md](firmware/README.md) |
| Desktop app | Node.js 24, Rust (`rustup`), the build tools of your system | [app/README.md](app/README.md) |
| Production tool, shared crates | Rust | [docs/production.md](docs/production.md) |
| Hardware | EasyEDA Pro (PCB), a 3D editor for the 3MF parts | [hardware/README.md](hardware/README.md) |

The design is described in [docs/architecture.md](docs/architecture.md),
the lamp behavior in [docs/behavior.md](docs/behavior.md) (the reference
specification) and the USB protocol in [protocol/README.md](protocol/README.md).

## Branches and pull requests

- One topic per branch, started from an up-to-date `main` and named
  `<type>/<topic>`, for example `feat/auto-brightness` or
  `fix/macos-adhoc-signing`.
- Open a pull request into `main`. Keep it a draft while you work: the
  checks run once it is ready for review, then on every push.
- Say what changes and why, and how you tested it: the board and firmware
  for a firmware change, the system for the app, screenshots for a change
  that shows.
- Update the documentation in the same pull request: the README of the
  part, [docs/behavior.md](docs/behavior.md) when the lamp behaves
  differently, the user guide when the app does.
- Pull requests are merged with a merge commit, so every commit lands on
  `main` as written and goes into the changelogs: tidy the history before
  the review.

## Commit messages

[Conventional Commits](https://www.conventionalcommits.org/), in English:
`type(scope): summary`, the summary in lower case and describing the
result, for example `fix(app): sign the macOS app ad hoc so that it opens
on Apple Silicon`. The body says why, wrapped at 72 characters.

- `feat`, `fix`, `perf`, `refactor` and `revert` release the part whose
  files the commit changes and appear in its changelog; `docs`, `style`,
  `build`, `chore`, `test` and `ci` release nothing
  ([docs/releasing.md](docs/releasing.md)).
- Scopes in use: `app`, `firmware`, `protocol`, `factory` (the production
  tool), `hardware`, `deps`.
- `feat!` or a `BREAKING CHANGE:` footer marks a breaking change. The USB
  protocol has its own rules: version 1 is frozen at 1.1 and only grows
  in compatible ways ([protocol/README.md](protocol/README.md#compatibility)).

## Checks

The pull request checks run these commands; run them before you push.

Firmware, from `firmware/`:

```sh
uv sync --locked
uv run clang-format --dry-run --Werror $(git ls-files 'lib/disled_core/*.[ch]' 'src/*.[ch]' 'test/*.c' 'tools/*.c')
uv run pio test -e native            # unit tests on the build machine
uv run pio run -e debug -e release
uv run pio check -e debug            # static analysis
```

Desktop app, from `app/`, then the Rust workspace from the root:

```sh
npm ci
npm run typecheck                    # vue-tsc, French messages included
npm run check                        # Biome; npm run fix applies the fixes
npm run build
cd ..
cargo fmt --all --check
cargo clippy --workspace --all-targets -- -D warnings
cargo test --workspace
```

After a change to a Rust type that the front end receives, run
`npm run bindings` and commit `app/src/bindings` (the checks compare
them).

Some changes can only be checked on a board: the lamp behavior
([test scenarios](docs/behavior.md#test-scenarios)), the touch, the light
sensor, USB sleep. [docs/bringup.md](docs/bringup.md) lists the checks.

## Code and files

- Formatting is checked: clang-format for the firmware, Biome for the
  front end, rustfmt for Rust; [`.editorconfig`](.editorconfig) for the
  rest.
- Every source file starts with an SPDX header
  ([LICENSING.md](LICENSING.md)); the firmware's also point to the
  permission for the WCH SDK.
- English for code, comments, documentation and commit messages. The app
  speaks English and French: add every new message to both
  `app/src/locales/en.json` and `fr.json` ([app/README.md](app/README.md#translations)).
- The firmware has its own rules (no dynamic allocation, no floating
  point, interrupt handlers that only capture data):
  [firmware/README.md](firmware/README.md#conventions).
- A protocol change updates the specification, the shared test vectors
  (`protocol/vectors`) and both codecs, the firmware's and
  `crates/disled-protocol`, in the same pull request.

## Dependencies

Dependabot opens update pull requests every week
([`.github/dependabot.yml`](.github/dependabot.yml)). Some versions stay
pinned on purpose:

- `@tauri-apps/cli` stays on 2.11: from 2.12, the Windows installer can
  abort an update started from the app with "Failed to kill DISLED"
  ([tauri-apps/tauri#16277](https://github.com/tauri-apps/tauri/issues/16277)).
  Close Dependabot's pull requests for 2.12 and later until that issue is
  fixed.
- wchisp, the flasher bundled with the app, is built by the release
  workflow at the version of `WCHISP_VERSION`. wchisp has no machine
  interface: the app and the production tool read its log lines
  (`crates/disled-flash/src/wchisp.rs`), so check them against a new
  version before changing it.

## Releases

The maintainers release the firmware and the app with release-please, and
the hardware by hand ([docs/releasing.md](docs/releasing.md)). The update
key that signs the app and the firmware lives in the repository secrets:
never commit a key.

## License

Contributions are accepted under the terms of the project
([LICENSING.md](LICENSING.md)): GPL-3.0-or-later with the additional
permission for the WCH SDK for the software, CERN-OHL-S-2.0 for the
hardware design files.
