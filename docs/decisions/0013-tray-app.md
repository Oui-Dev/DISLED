# 0013 — A tray app with a popup

- Status: Accepted, amended by [0018](0018-nuxt-ui-themes-languages.md)
  (French as well as English, color modes)
- Date: 2026-10-21

## Context

The phase 1 app was a window full of tuning tools. Day to day, the lamp
needs a glance and two clicks: change the color, switch it off, adjust the
brightness. The app also has to start with the computer and bring the lamp
back to the user's settings, since the lamp keeps nothing in flash.

## Decision

- The app lives only in the menu bar (macOS, no Dock icon) or the
  notification area (Windows). Its icon is a dot in the lamp's color, drawn
  at run time; a ring means the lamp is off, a faint ring that no lamp is
  connected.
- Left click opens a popup under the icon (power, colors, brightness, a
  settings page); it hides when it loses the focus. Right click opens a
  short menu (Open, Diagnostics, Quit). Linux has no tray click events: the
  menu is the way in there.
- The tuning tools of phase 1 move to a Diagnostics window opened on
  demand, with the lamp diagnostics and the self-test.
- Settings saved on the computer (brightness mode, fixed level, automatic
  correction) are applied by the USB worker each time the lamp connects.
  Power and color are not: the lamp starts green ("available") on purpose.
- Start at login is on by default (set once, at the first launch of an
  installed app) and can be turned off; one instance only.
- The interface is in English.

## Consequences

- Plugins: positioner (popup under the icon), store (settings), autostart,
  single instance.
- Windows that create other windows must do it from async commands:
  a synchronous command deadlocks with WebView2 (seen on Windows).
- The popup and the Diagnostics window each run their own copy of the
  front-end store; the backend broadcasts setting changes to keep them in
  step.
