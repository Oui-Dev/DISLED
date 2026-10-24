// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

//! DISLED desktop app: a tray icon with a popup for daily use, a
//! Diagnostics window for tuning, the USB worker behind both, the app
//! updates and the lamp firmware updates.

mod device;
mod firmware;
mod i18n;
mod settings;
mod tray;
mod updates;
mod windows;

use device::{Connection, DeviceHandle};
use disled_protocol::{Command, SetState};
use firmware::{FirmwareState, FirmwareStatus};
use i18n::{Locale, LocaleState};
use serde::Deserialize;
use settings::{Language, Settings, SettingsPatch, SettingsState};
use tauri::{AppHandle, Emitter, Manager, RunEvent, State, WindowEvent};
use tauri_plugin_autostart::ManagerExt;
use tauri_plugin_opener::OpenerExt;
use tray::TrayState;
use updates::{UpdateState, UpdateStatus};

/// Argument of the login item: started at login, the app stays in the tray.
const AUTOSTART_ARG: &str = "--autostart";

const EVENT_SETTINGS: &str = "settings-changed";
const EVENT_LOCALE: &str = "locale-changed";

/// Current connection, for a frontend that starts after the first event.
#[tauri::command]
fn device_connection(device: State<'_, DeviceHandle>) -> Connection {
    device.connection()
}

/// Queues a command for the lamp. Replies arrive as `device-message` events.
#[tauri::command]
fn device_send(device: State<'_, DeviceHandle>, command: Command) -> Result<(), String> {
    device.send(command)
}

/// Writes a recording chosen by the user in a save dialog.
#[tauri::command]
fn save_csv(path: String, contents: String) -> Result<(), String> {
    if !path.to_ascii_lowercase().ends_with(".csv") {
        return Err("recordings are saved as .csv files".into());
    }
    std::fs::write(&path, contents).map_err(|e| e.to_string())
}

#[tauri::command]
fn settings_get(settings: State<'_, SettingsState>) -> Settings {
    settings.get()
}

/// Changes settings, saves them, and applies what changed to the app and to
/// the lamp.
#[tauri::command]
fn settings_set(
    app: AppHandle,
    device: State<'_, DeviceHandle>,
    patch: SettingsPatch,
) -> Result<Settings, String> {
    let mut settings = app.state::<SettingsState>().get();
    settings.apply(&patch);
    settings::save(&app, settings)?;
    // Every window keeps its own copy: tell them all.
    let _ = app.emit(EVENT_SETTINGS, settings);
    if patch.color_mode.is_some() {
        app.set_theme(settings.color_mode.theme());
    }
    if patch.language.is_some() {
        apply_language(&app, settings.language);
    }
    if device.connection().connected {
        if patch.brightness_mode.is_some() || patch.manual_level.is_some() {
            device.send(Command::SetState(SetState {
                brightness_mode: Some(settings.brightness_mode),
                manual_level: Some(settings.manual_level),
                ..SetState::default()
            }))?;
        }
        if patch.auto_bias.is_some() && device.auto_bias_supported() {
            device.send(Command::SetAutoBias {
                bias: settings.auto_bias,
            })?;
        }
    }
    Ok(settings)
}

/// Puts the tray and the window titles in the language of the setting, and
/// tells the windows when it changes.
fn apply_language(app: &AppHandle, language: Language) {
    let locale = Locale::of(language);
    if app.state::<LocaleState>().replace(locale) != locale {
        tray::relabel(app, locale);
        windows::retitle(app, locale);
        let _ = app.emit(EVENT_LOCALE, locale);
    }
}

/// The language of the app now, for a window that starts.
#[tauri::command]
fn locale_get(locale: State<'_, LocaleState>) -> Locale {
    locale.get()
}

#[tauri::command]
fn autostart_get(app: AppHandle) -> bool {
    app.autolaunch().is_enabled().unwrap_or(false)
}

#[tauri::command]
fn autostart_set(app: AppHandle, enabled: bool) -> Result<bool, String> {
    let launcher = app.autolaunch();
    let result = if enabled {
        launcher.enable()
    } else {
        launcher.disable()
    };
    result.map_err(|e| e.to_string())?;
    Ok(launcher.is_enabled().unwrap_or(enabled))
}

#[tauri::command]
fn update_status(updates: State<'_, UpdateState>) -> UpdateStatus {
    updates.status()
}

#[tauri::command]
async fn update_check(app: AppHandle) -> UpdateStatus {
    updates::check(&app).await
}

/// Returns only on failure: on success the app restarts.
#[tauri::command]
async fn update_install(app: AppHandle) -> Result<(), String> {
    updates::install(&app).await
}

#[tauri::command]
fn firmware_status(firmware: State<'_, FirmwareState>) -> FirmwareStatus {
    firmware.status()
}

#[tauri::command]
async fn firmware_check(app: AppHandle) -> FirmwareStatus {
    firmware::check(&app).await
}

/// Starts a lamp update; `firmware-status` events follow it.
#[tauri::command]
fn firmware_install(app: AppHandle) -> Result<(), String> {
    firmware::install(&app)
}

#[tauri::command]
fn firmware_cancel(app: AppHandle) {
    firmware::cancel(&app);
}

/// Async on purpose: a synchronous command that creates a window deadlocks
/// on Windows (WebView2), leaving a blank window and a frozen app.
#[tauri::command]
async fn open_diagnostics(app: AppHandle) {
    windows::open_diagnostics(&app);
}

/// Async for the same reason as `open_diagnostics`.
#[tauri::command]
async fn open_lamp_update(app: AppHandle) {
    firmware::clear_result(&app);
    windows::open_lamp_update(&app);
}

/// Pages the windows may open in the browser.
#[derive(Deserialize)]
#[cfg_attr(test, derive(ts_rs::TS), ts(export))]
#[serde(rename_all = "snake_case")]
enum Link {
    /// The tool that installs the WinUSB driver of the lamp's bootloader.
    Zadig,
    /// Flashing by hand, when the app cannot.
    FlashingGuide,
}

#[tauri::command]
fn open_link(app: AppHandle, link: Link) -> Result<(), String> {
    let url = match link {
        Link::Zadig => "https://zadig.akeo.ie/",
        Link::FlashingGuide => "https://github.com/Oui-Dev/DISLED/blob/main/docs/flashing.md",
    };
    app.opener()
        .open_url(url, None::<&str>)
        .map_err(|e| e.to_string())
}

#[tauri::command]
fn quit(app: AppHandle) {
    app.exit(0);
}

/// Loads the settings; at the very first launch of an installed app, turns
/// on start at login (never for `tauri dev`, which would register the debug
/// executable).
fn init_settings(app: &AppHandle) -> Settings {
    let mut settings = settings::load(app);
    if !settings.autostart_initialized && !cfg!(debug_assertions) {
        let _ = app.autolaunch().enable();
        settings.autostart_initialized = true;
    }
    let _ = settings::save(app, settings);
    settings
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    let started_at_login = std::env::args().any(|arg| arg == AUTOSTART_ARG);
    let app = tauri::Builder::default()
        // First: a second launch only shows the popup of the running app.
        .plugin(tauri_plugin_single_instance::init(|app, _args, _cwd| {
            windows::show_popup(app);
        }))
        .plugin(
            tauri_plugin_autostart::Builder::new()
                .args([AUTOSTART_ARG])
                .build(),
        )
        .plugin(tauri_plugin_positioner::init())
        .plugin(tauri_plugin_store::Builder::default().build())
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_updater::Builder::new().build())
        // Used from Rust only (open_link): no window permission.
        .plugin(tauri_plugin_opener::init())
        .manage(SettingsState::default())
        .manage(LocaleState::default())
        .manage(TrayState::default())
        .manage(windows::PopupState::default())
        .manage(UpdateState::default())
        .manage(FirmwareState::default())
        .setup(move |app| {
            // No Dock icon: the app lives in the menu bar.
            #[cfg(target_os = "macos")]
            app.set_activation_policy(tauri::ActivationPolicy::Accessory);

            let handle = app.handle().clone();
            let settings = init_settings(&handle);
            handle.set_theme(settings.color_mode.theme());
            let locale = Locale::of(settings.language);
            handle.state::<LocaleState>().replace(locale);
            tray::create(&handle, locale)?;
            if let Some(popup) = app.get_webview_window(windows::POPUP) {
                let owner = handle.clone();
                popup.on_window_event(move |event| {
                    if let WindowEvent::Focused(false) = event {
                        windows::hide_popup(&owner);
                    }
                });
            }
            app.manage(DeviceHandle::spawn(handle.clone()));
            updates::spawn_automatic_checks(handle.clone());
            if !started_at_login {
                windows::show_popup(&handle);
            }
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            device_connection,
            device_send,
            save_csv,
            settings_get,
            settings_set,
            locale_get,
            autostart_get,
            autostart_set,
            update_status,
            update_check,
            update_install,
            firmware_status,
            firmware_check,
            firmware_install,
            firmware_cancel,
            open_diagnostics,
            open_lamp_update,
            open_link,
            quit
        ])
        .build(tauri::generate_context!())
        .expect("error while building the DISLED app");

    app.run(|_app, event| {
        // A tray app keeps running with no window open; Quit calls exit(0),
        // which comes with an exit code.
        if let RunEvent::ExitRequested {
            code: None, api, ..
        } = event
        {
            api.prevent_exit();
        }
    });
}
