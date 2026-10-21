// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

//! The windows: the popup under the tray icon (daily use), and two opened
//! on demand, Diagnostics (telemetry, recordings, self-test) and the lamp
//! update (firmware).

use std::sync::Mutex;
use std::time::{Duration, Instant};

use tauri::{AppHandle, Manager, Runtime, WebviewUrl, WebviewWindow, WebviewWindowBuilder};
use tauri_plugin_positioner::{Position, WindowExt};

pub const POPUP: &str = "popup";
pub const DIAGNOSTICS: &str = "diagnostics";
pub const LAMP_UPDATE: &str = "lamp-update";

/// A click on the tray icon first takes the focus away from an open popup,
/// which hides it; the click itself must then not reopen it.
const REOPEN_GUARD: Duration = Duration::from_millis(300);

#[derive(Default)]
pub struct PopupState {
    hidden_at: Mutex<Option<Instant>>,
}

fn popup<R: Runtime>(app: &AppHandle<R>) -> Option<WebviewWindow<R>> {
    app.get_webview_window(POPUP)
}

/// Places the popup next to the tray icon (above it on Windows, below it on
/// macOS), or in a screen corner while the icon position is unknown (no
/// click yet, or Linux).
fn place<R: Runtime>(window: &WebviewWindow<R>) {
    let (tray, fallback) = if cfg!(target_os = "macos") {
        (Position::TrayBottomCenter, Position::TopRight)
    } else if cfg!(target_os = "windows") {
        (Position::TrayCenter, Position::BottomRight)
    } else {
        (Position::TrayCenter, Position::Center)
    };
    if window.move_window_constrained(tray).is_err() {
        let _ = window.move_window(fallback);
    }
}

pub fn show_popup<R: Runtime>(app: &AppHandle<R>) {
    if let Some(window) = popup(app) {
        place(&window);
        let _ = window.show();
        let _ = window.set_focus();
    }
}

pub fn hide_popup<R: Runtime>(app: &AppHandle<R>) {
    if let Some(window) = popup(app) {
        let _ = window.hide();
        if let Ok(mut hidden_at) = app.state::<PopupState>().hidden_at.lock() {
            *hidden_at = Some(Instant::now());
        }
    }
}

pub fn toggle_popup<R: Runtime>(app: &AppHandle<R>) {
    let Some(window) = popup(app) else { return };
    if window.is_visible().unwrap_or(false) {
        hide_popup(app);
        return;
    }
    let just_hidden = app
        .state::<PopupState>()
        .hidden_at
        .lock()
        .map(|t| t.is_some_and(|t| t.elapsed() < REOPEN_GUARD))
        .unwrap_or(false);
    if !just_hidden {
        show_popup(app);
    }
}

/// Brings a window that is already open to the front.
fn raise<R: Runtime>(app: &AppHandle<R>, label: &str) -> bool {
    let Some(window) = app.get_webview_window(label) else {
        return false;
    };
    let _ = window.unminimize();
    let _ = window.show();
    let _ = window.set_focus();
    true
}

pub fn open_diagnostics<R: Runtime>(app: &AppHandle<R>) {
    hide_popup(app);
    if raise(app, DIAGNOSTICS) {
        return;
    }
    let built = WebviewWindowBuilder::new(app, DIAGNOSTICS, WebviewUrl::App("index.html".into()))
        .title("DISLED diagnostics")
        .inner_size(1040.0, 760.0)
        .min_inner_size(760.0, 560.0)
        .build();
    if let Ok(window) = built {
        let _ = window.set_focus();
    }
}

/// The lamp update guide: a window rather than the popup, which hides as
/// soon as the user turns to the lamp or to Zadig.
pub fn open_lamp_update<R: Runtime>(app: &AppHandle<R>) {
    hide_popup(app);
    if raise(app, LAMP_UPDATE) {
        return;
    }
    let built = WebviewWindowBuilder::new(app, LAMP_UPDATE, WebviewUrl::App("index.html".into()))
        .title("DISLED lamp update")
        .inner_size(460.0, 560.0)
        .resizable(false)
        .build();
    if let Ok(window) = built {
        let _ = window.set_focus();
    }
}
