# 0018 — Nuxt UI, color modes and languages

- Status: Accepted
- Date: 2026-10-24
- Amends: [0013](0013-tray-app.md) (the interface is no longer English
  only)

## Context

The front end of the app grew component by component, each with its own
CSS:

- dark mode only through `prefers-color-scheme`, without a choice;
- the system font, English only (ADR 0013);
- a hand-made canvas chart, native inputs, `window.confirm`;
- the protocol types copied by hand from the Rust ones.

The lamp's users read French as often as English, and the popup has to
work with a keyboard and with a screen reader.

## Decision

- **Nuxt UI 4**, through its Vite plugin on Vue 3 and Vite (no Nuxt). Its
  components serve wherever one fits, charts included (TanStack Charts).
  Components are auto-imported; functions are imported explicitly. The
  Lucide icons are bundled, so the app stays offline and within its CSP.
- **Design system.** Montserrat, self-hosted, and two palettes in OKLCH at
  the hue of the logo (`app/src/assets/css/palettes.css`):
  - `ink`, the neutral palette, from the brand white (`#fefcf7`, the light
    background) to the brand black (`#050a06`, the dark background, with
    a hint of the green of the logo; the greys keep a faint tint only);
  - `brand`, the primary palette. The logo green (`#2ecc71`) is the
    primary of dark mode; a darker 500 serves light mode at about 4:1,
    the level of Nuxt UI's own palettes. Neutral text keeps 4.5:1.
  - Nuxt UI's semantic colors (success, info, warning, error) stay as
    they are. Alerts keep their text in the neutral tokens, the color
    on their icon, background and border.
- **Color mode and language are settings**, saved with the others by the
  backend: System, Light or Dark; System, English or French. The backend
  applies them where the system draws: the theme of the window frames,
  the tray menu and tooltip, the window titles. "System" takes the first
  system language the app speaks (sys-locale), English otherwise.
- **Translations.** One JSON file per language in `app/src/locales`,
  read by vue-i18n in the windows (compiled at build time) and by the
  backend (`include_str!`) for its few strings. English is the
  reference: TypeScript rejects a French file that lacks a message, and
  a Rust test checks that both files have the same keys. Messages from
  libraries (wchisp, the updater, hidapi) stay in English, inside a
  translated sentence.
- **State.** Pinia stores, one per domain, behind a typed layer for the
  Tauri commands and events. ts-rs generates the TypeScript types the
  front end exchanges with the backend from the Rust types.
- **Popup.** It takes the height of its content, within the screen, and
  Escape hides it.
- **Accessibility.** WCAG 2.2 AA is the target, the primary contrast
  aside (above). The colors are a radio group; sliders and switches are
  labelled; switching pages in the popup moves the focus to the page
  title; statuses are announced; our own animations and Nuxt UI's
  looping ones respect reduced motion (its loading spinners and the short
  opening of menus and tooltips do not). An axe-core audit of the three
  windows in both color modes finds nothing else.

## Consequences

- The bundle grows (Nuxt UI, the charts); each window loads only its own
  view.
- A new language means a JSON file; a variant of the `Language` and
  `Locale` enums in Rust, with its message file and system tag in
  `i18n.rs`; its messages, formats and Nuxt UI words in `app/src/i18n.ts`;
  and an entry in the language choices of the popup (step by step in
  `app/README.md`).
- After a change to a Rust type the front end receives, `npm run
  bindings` writes the TypeScript types again; CI fails when they are
  out of date.
- A palette changes in `palettes.css`, by hand: nothing checks its
  contrasts, so a change keeps the levels above.
- "The interface is in English" of ADR 0013 no longer holds.
