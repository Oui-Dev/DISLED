// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

//! DISLED desktop app: Tauri commands over the USB device worker.

mod device;
mod protocol;

use device::{Connection, DeviceHandle};
use protocol::Command;
use tauri::{Manager, State};

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

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .setup(|app| {
            app.manage(DeviceHandle::spawn(app.handle().clone()));
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            device_connection,
            device_send,
            save_csv
        ])
        .run(tauri::generate_context!())
        .expect("error while running the DISLED app");
}
