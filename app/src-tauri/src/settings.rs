// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

//! Settings kept on this computer and applied to the lamp at every
//! connection: the lamp starts in automatic brightness after each power-up
//! and forgets its bias, so the app re-applies them. Power and color are
//! not settings: the lamp always starts green (available).

use std::sync::Mutex;

use disled_protocol::{AUTO_BIAS_MAX, AUTO_BIAS_MIN, BrightnessMode, Command, SetState};
use serde::{Deserialize, Serialize};
use tauri::{AppHandle, Manager, Runtime};
use tauri_plugin_store::StoreExt;

const STORE_FILE: &str = "settings.json";
const KEY: &str = "settings";

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(default)]
pub struct Settings {
    pub brightness_mode: BrightnessMode,
    /// Percent, 1 to 100.
    pub manual_level: u8,
    /// Automatic brightness bias, percent.
    pub auto_bias: i8,
    /// Start at login was turned on once, at the first launch.
    pub autostart_initialized: bool,
    /// Look for a new version of the app at start-up and once a day.
    pub check_updates: bool,
}

impl Default for Settings {
    fn default() -> Self {
        Self {
            brightness_mode: BrightnessMode::Auto,
            manual_level: 50,
            auto_bias: 0,
            autostart_initialized: false,
            check_updates: true,
        }
    }
}

/// Fields the front end may change; `None` keeps the current value.
#[derive(Debug, Clone, Default, Deserialize)]
pub struct SettingsPatch {
    pub brightness_mode: Option<BrightnessMode>,
    pub manual_level: Option<u8>,
    pub auto_bias: Option<i8>,
    pub check_updates: Option<bool>,
}

impl Settings {
    /// Applies a patch, keeping every value in its range.
    pub fn apply(&mut self, patch: &SettingsPatch) {
        if let Some(mode) = patch.brightness_mode {
            self.brightness_mode = mode;
        }
        if let Some(level) = patch.manual_level {
            self.manual_level = level.clamp(1, 100);
        }
        if let Some(bias) = patch.auto_bias {
            self.auto_bias = bias.clamp(AUTO_BIAS_MIN, AUTO_BIAS_MAX);
        }
        if let Some(check) = patch.check_updates {
            self.check_updates = check;
        }
    }

    /// Commands that bring a lamp in line with these settings. The bias
    /// needs protocol 1.1 (`auto_bias` capability).
    pub fn commands(&self, auto_bias_supported: bool) -> Vec<Command> {
        let mut commands = vec![Command::SetState(SetState {
            brightness_mode: Some(self.brightness_mode),
            manual_level: Some(self.manual_level.clamp(1, 100)),
            ..SetState::default()
        })];
        if auto_bias_supported {
            commands.push(Command::SetAutoBias {
                bias: self.auto_bias,
            });
        }
        commands
    }
}

/// The settings in memory, shared by the commands and the USB worker.
#[derive(Default)]
pub struct SettingsState(pub Mutex<Settings>);

impl SettingsState {
    pub fn get(&self) -> Settings {
        self.0.lock().map(|s| *s).unwrap_or_default()
    }
}

/// Reads the saved settings (defaults when there are none or they cannot
/// be read).
pub fn load<R: Runtime>(app: &AppHandle<R>) -> Settings {
    app.store(STORE_FILE)
        .ok()
        .and_then(|store| store.get(KEY))
        .and_then(|value| serde_json::from_value(value).ok())
        .unwrap_or_default()
}

/// Saves the settings and updates the shared copy.
pub fn save<R: Runtime>(app: &AppHandle<R>, settings: Settings) -> Result<(), String> {
    if let Ok(mut shared) = app.state::<SettingsState>().0.lock() {
        *shared = settings;
    }
    let store = app.store(STORE_FILE).map_err(|e| e.to_string())?;
    store.set(
        KEY,
        serde_json::to_value(settings).map_err(|e| e.to_string())?,
    );
    store.save().map_err(|e| e.to_string())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_patch_keeps_values_in_range() {
        let mut settings = Settings::default();
        settings.apply(&SettingsPatch {
            manual_level: Some(0),
            auto_bias: Some(-90),
            ..SettingsPatch::default()
        });
        assert_eq!(settings.manual_level, 1);
        assert_eq!(settings.auto_bias, AUTO_BIAS_MIN);
        assert_eq!(settings.brightness_mode, BrightnessMode::Auto);
    }

    #[test]
    fn the_bias_is_sent_only_to_lamps_that_know_it() {
        let settings = Settings {
            brightness_mode: BrightnessMode::Manual,
            manual_level: 30,
            auto_bias: 20,
            autostart_initialized: true,
            check_updates: true,
        };
        assert_eq!(settings.commands(false).len(), 1);
        let commands = settings.commands(true);
        assert_eq!(commands.len(), 2);
        assert_eq!(commands[1], Command::SetAutoBias { bias: 20 });
        match &commands[0] {
            Command::SetState(s) => {
                assert_eq!(s.brightness_mode, Some(BrightnessMode::Manual));
                assert_eq!(s.manual_level, Some(30));
                assert_eq!(s.on, None);
                assert_eq!(s.color, None);
            }
            other => panic!("unexpected {other:?}"),
        }
    }

    #[test]
    fn old_files_without_new_fields_still_load() {
        let settings: Settings =
            serde_json::from_str(r#"{"brightness_mode":"manual","manual_level":40}"#)
                .expect("settings");
        assert_eq!(settings.manual_level, 40);
        assert_eq!(settings.auto_bias, 0);
        assert!(!settings.autostart_initialized);
        assert!(settings.check_updates);
    }
}
