// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

//! Lamp firmware updates. With the app updates, the app
//! looks for the newest firmware release on GitHub. On request it downloads
//! the release build, checks the update key's signature, waits for the
//! lamp's ROM bootloader (BOOT + RESET), flashes it with wchisp and checks
//! the version the lamp then reports. The update window follows each step.

use std::path::PathBuf;
use std::sync::Mutex;
use std::sync::atomic::{AtomicBool, Ordering};
use std::thread;
use std::time::{Duration, Instant};

use disled_flash::firmware::{self as files, file_name};
use disled_flash::wchisp::{self, Probe, Stage, Wchisp};
use disled_protocol::Command;
use semver::Version;
use serde::{Deserialize, Serialize};
use tauri::{AppHandle, Emitter, Manager, Runtime};

use crate::device::DeviceHandle;
use crate::updates::now;

const EVENT_STATUS: &str = "firmware-status";

/// Published releases only: GitHub hides the drafts from anonymous callers.
const RELEASES_URL: &str = "https://api.github.com/repos/Oui-Dev/DISLED/releases?per_page=100";
const TAG_PREFIX: &str = "fw-v";

/// How often the app looks for the bootloader.
const POLL: Duration = Duration::from_secs(1);
/// A lamp that restarts in its bootloader by itself gets this long before
/// the app asks for the buttons.
const COMMAND_GRACE: Duration = Duration::from_secs(10);
/// wchisp failures in a row before the update gives up.
const PROBE_FAILURES: u32 = 5;
/// After the flash the lamp restarts, through its bootloader first: it
/// takes several seconds.
const LAMP_TIMEOUT: Duration = Duration::from_secs(60);

/// A firmware the app can install.
#[derive(Debug, Clone, PartialEq, Eq)]
struct Release {
    version: Version,
    source: Source,
}

#[derive(Debug, Clone, PartialEq, Eq)]
enum Source {
    /// The release build attached to a GitHub release, and its signature.
    GitHub { hex: String, signature: String },
    /// Development builds only: a local file, unsigned.
    Local(PathBuf),
}

/// What matters here in an answer of the GitHub releases API.
#[derive(Deserialize)]
struct GitHubRelease {
    tag_name: String,
    draft: bool,
    prerelease: bool,
    assets: Vec<GitHubAsset>,
}

#[derive(Deserialize)]
struct GitHubAsset {
    name: String,
    browser_download_url: String,
}

/// Where a lamp update stands, shown by the update window.
#[derive(Debug, Clone, Default, PartialEq, Eq, Serialize)]
#[cfg_attr(test, derive(ts_rs::TS), ts(export))]
#[serde(tag = "step", rename_all = "snake_case")]
pub enum Flash {
    #[default]
    Idle,
    Downloading,
    /// Waiting for the ROM bootloader: the user holds BOOT and presses
    /// RESET, or the lamp restarts in it by itself (`by_command`).
    WaitingForBootloader {
        by_command: bool,
    },
    /// Windows: the bootloader needs the WinUSB driver (Zadig), once.
    DriverMissing,
    /// Linux: the user may not open the bootloader (udev rule).
    NoPermission,
    /// Several WCH bootloaders: which one is the lamp is unknown.
    SeveralBootloaders,
    /// A WCH bootloader that is not a DISLED lamp.
    OtherChip {
        chip: String,
    },
    Flashing {
        stage: Stage,
    },
    /// Flashed: the lamp restarts.
    WaitingForLamp,
    Done {
        version: String,
    },
    Failed {
        error: String,
    },
}

/// What the popup and the update window show about firmware updates.
#[derive(Debug, Clone, Default, Serialize)]
#[cfg_attr(test, derive(ts_rs::TS), ts(export))]
pub struct FirmwareStatus {
    /// A check is running.
    pub checking: bool,
    /// Newest firmware the app can install.
    pub latest: Option<String>,
    /// Why the last check failed; cleared by the next success.
    pub error: Option<String>,
    /// Last successful check, seconds since 1970-01-01 UTC.
    pub checked_at: Option<u64>,
    pub flash: Flash,
}

#[derive(Default)]
pub struct FirmwareState {
    status: Mutex<FirmwareStatus>,
    /// The firmware found by the last check, kept for the install.
    release: Mutex<Option<Release>>,
    /// An update is running.
    running: AtomicBool,
    /// The user cancelled; honored while the update waits for the
    /// bootloader.
    cancel: AtomicBool,
}

impl FirmwareState {
    pub fn status(&self) -> FirmwareStatus {
        self.status.lock().map(|s| s.clone()).unwrap_or_default()
    }
}

/// Changes the status and tells every window.
fn change<R: Runtime>(app: &AppHandle<R>, edit: impl FnOnce(&mut FirmwareStatus)) {
    let status = match app.state::<FirmwareState>().status.lock() {
        Ok(mut status) => {
            edit(&mut status);
            status.clone()
        }
        Err(_) => return,
    };
    let _ = app.emit(EVENT_STATUS, status);
}

fn show<R: Runtime>(app: &AppHandle<R>, flash: Flash) {
    if app.state::<FirmwareState>().status().flash != flash {
        change(app, |s| s.flash = flash);
    }
}

/// The newest published firmware release whose release build is signed.
/// The releases from before the signatures (up to 0.2.1) never qualify.
fn newest(releases: &[GitHubRelease]) -> Option<Release> {
    releases
        .iter()
        .filter(|release| !release.draft && !release.prerelease)
        .filter_map(|release| {
            let version = Version::parse(release.tag_name.strip_prefix(TAG_PREFIX)?).ok()?;
            let url = |name: String| {
                release
                    .assets
                    .iter()
                    .find(|asset| asset.name == name)
                    .map(|asset| asset.browser_download_url.clone())
            };
            let hex = file_name(&version);
            let source = Source::GitHub {
                signature: url(format!("{hex}.sig"))?,
                hex: url(hex)?,
            };
            Some(Release { version, source })
        })
        .filter(|release| release.version.pre.is_empty())
        .max_by(|a, b| a.version.cmp(&b.version))
}

/// Development builds only: `DISLED_FIRMWARE_HEX=path/disled-fw-X.Y.Z.hex`
/// stands for the newest release, unsigned, to try an update with a local
/// build. Release builds ignore it.
fn local_release() -> Option<Release> {
    if !cfg!(debug_assertions) {
        return None;
    }
    let path = PathBuf::from(std::env::var_os("DISLED_FIRMWARE_HEX")?);
    let version = files::version_of(path.file_name()?.to_str()?)?;
    Some(Release {
        version,
        source: Source::Local(path),
    })
}

/// wchisp next to the app executable, where the installers put it. Debug
/// builds fall back to the one on the PATH (`cargo install wchisp`).
fn flasher() -> Result<Wchisp, String> {
    let bundled = std::env::current_exe()
        .map_err(|e| e.to_string())?
        .with_file_name(wchisp::PROGRAM);
    if bundled.is_file() {
        Ok(Wchisp::new(bundled))
    } else if cfg!(debug_assertions) {
        Ok(Wchisp::new(wchisp::PROGRAM))
    } else {
        Err("wchisp is missing next to the app: reinstall DISLED".into())
    }
}

/// Downloads a file; GitHub wants a user agent.
async fn get<R: Runtime>(app: &AppHandle<R>, url: &str) -> Result<Vec<u8>, String> {
    // The same TLS stack as the updater: rustls with ring.
    if rustls::crypto::CryptoProvider::get_default().is_none() {
        let _ = rustls::crypto::ring::default_provider().install_default();
    }
    let client = reqwest::Client::builder()
        .user_agent(format!("DISLED/{}", app.package_info().version))
        .connect_timeout(Duration::from_secs(15))
        .timeout(Duration::from_secs(60))
        .build()
        .map_err(|e| e.to_string())?;
    let response = client
        .get(url)
        .send()
        .await
        .and_then(|response| response.error_for_status())
        .map_err(|e| e.to_string())?;
    let body = response.bytes().await.map_err(|e| e.to_string())?;
    Ok(body.to_vec())
}

/// Looks for the newest firmware on GitHub.
pub async fn check<R: Runtime>(app: &AppHandle<R>) -> FirmwareStatus {
    change(app, |s| s.checking = true);
    let result = match local_release() {
        Some(release) => Ok(Some(release)),
        None => get(app, RELEASES_URL).await.and_then(|body| {
            serde_json::from_slice::<Vec<GitHubRelease>>(&body)
                .map(|releases| newest(&releases))
                .map_err(|e| format!("unexpected answer from GitHub: {e}"))
        }),
    };
    let state = app.state::<FirmwareState>();
    match result {
        Ok(release) => {
            let latest = release.as_ref().map(|r| r.version.to_string());
            if let Ok(mut kept) = state.release.lock() {
                *kept = release;
            }
            change(app, |s| {
                s.checking = false;
                s.latest = latest;
                s.error = None;
                s.checked_at = Some(now());
            });
        }
        Err(error) => change(app, |s| {
            s.checking = false;
            s.error = Some(error);
        }),
    }
    state.status()
}

/// The release build, checked, in a file for wchisp.
async fn download<R: Runtime>(app: &AppHandle<R>, release: &Release) -> Result<PathBuf, String> {
    let (hex, signature) = match &release.source {
        Source::Local(path) => return Ok(path.clone()),
        Source::GitHub { hex, signature } => (hex, signature),
    };
    let data = get(app, hex).await?;
    let signature = String::from_utf8(get(app, signature).await?)
        .map_err(|_| "the firmware signature is unreadable")?;
    files::verify(&data, &signature, &release.version)?;
    let folder = app
        .path()
        .app_cache_dir()
        .map_err(|e| e.to_string())?
        .join("firmware");
    std::fs::create_dir_all(&folder).map_err(|e| e.to_string())?;
    let file = folder.join(file_name(&release.version));
    std::fs::write(&file, &data).map_err(|e| e.to_string())?;
    Ok(file)
}

/// Why an update ended early.
enum Stop {
    Cancelled,
    Failed(String),
}

impl From<String> for Stop {
    fn from(error: String) -> Self {
        Stop::Failed(error)
    }
}

impl From<&str> for Stop {
    fn from(error: &str) -> Self {
        Stop::Failed(error.to_owned())
    }
}

/// Updates the lamp with the firmware found by the last check, on its own
/// thread; the update window follows the status.
pub fn install<R: Runtime>(app: &AppHandle<R>) -> Result<(), String> {
    let state = app.state::<FirmwareState>();
    let release = state
        .release
        .lock()
        .ok()
        .and_then(|release| release.clone())
        .ok_or("no firmware to install: check for updates first")?;
    if state.running.swap(true, Ordering::SeqCst) {
        return Err("a lamp update is already running".into());
    }
    state.cancel.store(false, Ordering::SeqCst);
    let owner = app.clone();
    let spawned = thread::Builder::new()
        .name("disled-firmware".into())
        .spawn(move || {
            let flash = match update(&owner, &release) {
                Ok(()) => Flash::Done {
                    version: release.version.to_string(),
                },
                Err(Stop::Cancelled) => Flash::Idle,
                Err(Stop::Failed(error)) => Flash::Failed { error },
            };
            // Free first: the window offers to try again once it shows the
            // result.
            owner
                .state::<FirmwareState>()
                .running
                .store(false, Ordering::SeqCst);
            show(&owner, flash);
        });
    if let Err(error) = spawned {
        state.running.store(false, Ordering::SeqCst);
        return Err(error.to_string());
    }
    Ok(())
}

/// Stops an update that waits for the bootloader.
pub fn cancel<R: Runtime>(app: &AppHandle<R>) {
    app.state::<FirmwareState>()
        .cancel
        .store(true, Ordering::SeqCst);
}

/// Forgets the result of the last update, when the update window opens
/// again.
pub fn clear_result<R: Runtime>(app: &AppHandle<R>) {
    let state = app.state::<FirmwareState>();
    let finished = matches!(
        state.status().flash,
        Flash::Done { .. } | Flash::Failed { .. }
    );
    if finished && !state.running.load(Ordering::SeqCst) {
        show(app, Flash::Idle);
    }
}

/// The update itself; returns once the lamp runs the new firmware.
fn update<R: Runtime>(app: &AppHandle<R>, release: &Release) -> Result<(), Stop> {
    show(app, Flash::Downloading);
    let flasher = flasher()?;
    let file = tauri::async_runtime::block_on(download(app, release))?;

    // Into the ROM bootloader. A lamp that announces ENTER_BOOTLOADER
    // restarts in it by itself; for the others the user presses the
    // buttons (the CH32V203 cannot start it from software).
    let device = app.state::<DeviceHandle>();
    let by_command = device.info().is_some_and(|info| info.enter_bootloader)
        && device.send(Command::EnterBootloader).is_ok();
    wait_for_bootloader(app, &flasher, by_command)?;

    flasher.flash(&file, |stage| show(app, Flash::Flashing { stage }))?;

    show(app, Flash::WaitingForLamp);
    let since = Instant::now();
    loop {
        if let Some(info) = device.info() {
            let [major, minor, patch] = info.firmware.map(u64::from);
            return if Version::new(major, minor, patch) == release.version {
                Ok(())
            } else {
                Err(Stop::Failed(format!(
                    "after the update the lamp reports firmware {}",
                    info.firmware_version
                )))
            };
        }
        if since.elapsed() > LAMP_TIMEOUT {
            return Err(
                "the lamp did not start again: press RESET, or unplug it and plug it back in"
                    .into(),
            );
        }
        thread::sleep(Duration::from_millis(250));
    }
}

/// Waits until the lamp's bootloader can be flashed, showing what holds it
/// up.
fn wait_for_bootloader<R: Runtime>(
    app: &AppHandle<R>,
    flasher: &Wchisp,
    by_command: bool,
) -> Result<(), Stop> {
    let state = app.state::<FirmwareState>();
    let since = Instant::now();
    let waiting = || Flash::WaitingForBootloader {
        by_command: by_command && since.elapsed() < COMMAND_GRACE,
    };
    show(app, waiting());
    let mut previous = None;
    let mut failures = 0;
    loop {
        if state.cancel.load(Ordering::SeqCst) {
            return Err(Stop::Cancelled);
        }
        let probe = match flasher.probe() {
            Ok(probe) => {
                failures = 0;
                probe
            }
            Err(error) => {
                failures += 1;
                if failures >= PROBE_FAILURES {
                    return Err(Stop::Failed(error));
                }
                thread::sleep(POLL);
                continue;
            }
        };
        let flash = match &probe {
            Probe::Ready(chip) if chip.starts_with(wchisp::CHIP) => return Ok(()),
            Probe::Nothing => Some(waiting()),
            // A bootloader that just appeared may not open at once: a
            // problem shows when two probes in a row see it.
            _ if previous.as_ref() != Some(&probe) => None,
            Probe::Ready(chip) => Some(Flash::OtherChip { chip: chip.clone() }),
            Probe::Several => Some(Flash::SeveralBootloaders),
            Probe::DriverMissing => Some(Flash::DriverMissing),
            Probe::NoPermission => Some(Flash::NoPermission),
        };
        if let Some(flash) = flash {
            show(app, flash);
        }
        previous = Some(probe);
        thread::sleep(POLL);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn release(tag: &str, assets: &[&str]) -> GitHubRelease {
        GitHubRelease {
            tag_name: tag.into(),
            draft: false,
            prerelease: false,
            assets: assets
                .iter()
                .map(|name| GitHubAsset {
                    name: (*name).into(),
                    browser_download_url: format!("https://example.com/{tag}/{name}"),
                })
                .collect(),
        }
    }

    fn signed(version: &str) -> GitHubRelease {
        let hex = format!("disled-fw-{version}.hex");
        release(
            &format!("fw-v{version}"),
            &[&hex, &format!("{hex}.sig"), "SHA256SUMS"],
        )
    }

    #[test]
    fn the_newest_signed_firmware_wins() {
        let releases = [
            release("app-v0.4.0", &["DISLED_0.4.0_x64-setup.exe", "latest.json"]),
            release("fw-v0.2.1", &["disled-fw-0.2.1.hex", "SHA256SUMS"]),
            signed("0.2.2"),
            signed("0.2.3-rc.1"),
            GitHubRelease {
                prerelease: true,
                ..signed("0.3.0")
            },
            GitHubRelease {
                draft: true,
                ..signed("0.4.0")
            },
        ];
        let newest = newest(&releases).expect("a firmware");
        assert_eq!(newest.version, Version::new(0, 2, 2));
        assert_eq!(
            newest.source,
            Source::GitHub {
                hex: "https://example.com/fw-v0.2.2/disled-fw-0.2.2.hex".into(),
                signature: "https://example.com/fw-v0.2.2/disled-fw-0.2.2.hex.sig".into(),
            }
        );
    }

    #[test]
    fn versions_compare_as_numbers() {
        let releases = [signed("0.9.0"), signed("0.10.0"), signed("0.2.9")];
        assert_eq!(
            newest(&releases).map(|r| r.version),
            Some(Version::new(0, 10, 0))
        );
    }

    #[test]
    fn unsigned_firmware_is_never_offered() {
        let releases = [release(
            "fw-v0.2.1",
            &[
                "disled-fw-0.2.1.hex",
                "disled-fw-0.2.1-debug.hex",
                "SHA256SUMS",
            ],
        )];
        assert_eq!(newest(&releases), None);
    }

    #[test]
    fn a_github_answer_reads() {
        let answer = r#"[{
            "tag_name": "fw-v0.2.2", "name": "fw-v0.2.2", "draft": false,
            "prerelease": false, "body": "Bug fixes",
            "assets": [
                {"name": "disled-fw-0.2.2.hex", "size": 58359,
                 "browser_download_url": "https://github.com/Oui-Dev/DISLED/releases/download/fw-v0.2.2/disled-fw-0.2.2.hex"},
                {"name": "disled-fw-0.2.2.hex.sig", "size": 412,
                 "browser_download_url": "https://github.com/Oui-Dev/DISLED/releases/download/fw-v0.2.2/disled-fw-0.2.2.hex.sig"}
            ]
        }]"#;
        let releases: Vec<GitHubRelease> = serde_json::from_str(answer).expect("releases");
        assert_eq!(
            newest(&releases).map(|r| r.version),
            Some(Version::new(0, 2, 2))
        );
    }
}
