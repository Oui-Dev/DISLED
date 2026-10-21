// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

//! Connection to the lamp on a worker thread: discovery and reconnection,
//! command writes, message reads. Results reach the frontend as events.

use std::sync::{Arc, Mutex, mpsc};
use std::thread;
use std::time::Duration;

use hidapi::{HidApi, HidDevice};
use serde::Serialize;
use tauri::{AppHandle, Emitter, Manager};

use disled_protocol::{
    self as protocol, Command, DecodeError, Info, Message, PAYLOAD_SIZE, PROTOCOL_MAJOR,
    REPORT_ID_COMMAND, REPORT_ID_MESSAGE,
};

use crate::settings::SettingsState;
use crate::tray::{self, Look};

/// pid.codes test IDs used during phase 1: release and debug builds.
pub const VENDOR_ID: u16 = 0x1209;
pub const PRODUCT_ID_RELEASE: u16 = 0x0001;
pub const PRODUCT_ID_DEBUG: u16 = 0x0002;
const USAGE_PAGE_VENDOR: u16 = 0xFF00;

const SCAN_INTERVAL: Duration = Duration::from_millis(1000);
const READ_TIMEOUT_MS: i32 = 20;

pub const EVENT_CONNECTION: &str = "device-connection";
pub const EVENT_MESSAGE: &str = "device-message";
pub const EVENT_PROTOCOL_ERROR: &str = "device-protocol-error";

#[derive(Debug, Clone, Default, Serialize)]
pub struct Connection {
    pub connected: bool,
    pub debug_build: bool,
    pub serial: Option<String>,
}

pub struct DeviceHandle {
    commands: mpsc::Sender<Command>,
    connection: Arc<Mutex<Connection>>,
    info: Arc<Mutex<Option<Info>>>,
}

impl DeviceHandle {
    pub fn spawn(app: AppHandle) -> Self {
        let (commands, receiver) = mpsc::channel();
        let connection = Arc::new(Mutex::new(Connection::default()));
        let info = Arc::new(Mutex::new(None));
        let (shared, shared_info) = (Arc::clone(&connection), Arc::clone(&info));
        thread::Builder::new()
            .name("disled-usb".into())
            .spawn(move || {
                Worker {
                    app,
                    receiver,
                    connection: shared,
                    info: shared_info,
                    seq: 0,
                }
                .run()
            })
            .expect("spawn the USB worker thread");
        Self {
            commands,
            connection,
            info,
        }
    }

    /// What the connected lamp said about itself (INFO), once it answered.
    pub fn info(&self) -> Option<Info> {
        self.info.lock().ok().and_then(|info| info.clone())
    }

    /// The connected lamp understands SET_AUTO_BIAS (protocol 1.1).
    pub fn auto_bias_supported(&self) -> bool {
        self.info().is_some_and(|info| info.auto_bias)
    }

    pub fn send(&self, command: Command) -> Result<(), String> {
        if !self.connection().connected {
            return Err("no DISLED lamp connected".into());
        }
        self.commands
            .send(command)
            .map_err(|_| "the USB worker stopped".to_string())
    }

    pub fn connection(&self) -> Connection {
        self.connection
            .lock()
            .map(|c| c.clone())
            .unwrap_or_default()
    }
}

struct Worker {
    app: AppHandle,
    receiver: mpsc::Receiver<Command>,
    connection: Arc<Mutex<Connection>>,
    info: Arc<Mutex<Option<Info>>>,
    seq: u8,
}

/// Why the connected loop ended.
enum Exit {
    Disconnected,
    AppClosed,
}

impl Worker {
    fn run(mut self) {
        let mut api = loop {
            match HidApi::new() {
                Ok(api) => break api,
                Err(_) => thread::sleep(SCAN_INTERVAL),
            }
        };
        loop {
            match self.open(&mut api) {
                Some(device) => {
                    if let Exit::AppClosed = self.serve(&device) {
                        return;
                    }
                    self.set_info(None);
                    self.set_connection(Connection::default());
                    tray::show(&self.app, Look::NoLamp);
                }
                None => {
                    // Commands sent while no lamp is connected are dropped.
                    loop {
                        match self.receiver.try_recv() {
                            Ok(_) => continue,
                            Err(mpsc::TryRecvError::Empty) => break,
                            Err(mpsc::TryRecvError::Disconnected) => return,
                        }
                    }
                    thread::sleep(SCAN_INTERVAL);
                }
            }
        }
    }

    fn open(&mut self, api: &mut HidApi) -> Option<HidDevice> {
        api.refresh_devices().ok()?;
        let info = api.device_list().find(|d| {
            d.vendor_id() == VENDOR_ID
                && (d.product_id() == PRODUCT_ID_RELEASE || d.product_id() == PRODUCT_ID_DEBUG)
                && (d.usage_page() == USAGE_PAGE_VENDOR || d.usage_page() == 0)
        })?;
        let device = info.open_device(api).ok()?;
        self.set_connection(Connection {
            connected: true,
            debug_build: info.product_id() == PRODUCT_ID_DEBUG,
            serial: info.serial_number().map(str::to_owned),
        });
        Some(device)
    }

    fn set_connection(&self, connection: Connection) {
        if let Ok(mut shared) = self.connection.lock() {
            *shared = connection.clone();
        }
        let _ = self.app.emit(EVENT_CONNECTION, connection);
    }

    fn set_info(&self, info: Option<Info>) {
        if let Ok(mut shared) = self.info.lock() {
            *shared = info;
        }
    }

    fn next_seq(&mut self) -> u8 {
        // 0 marks unsolicited messages: never use it for a request.
        self.seq = if self.seq == u8::MAX { 1 } else { self.seq + 1 };
        self.seq
    }

    fn write(&mut self, device: &HidDevice, command: &Command) -> bool {
        let mut report = [0u8; PAYLOAD_SIZE + 1];
        report[0] = REPORT_ID_COMMAND;
        report[1..].copy_from_slice(&command.encode(self.next_seq()));
        device.write(&report).is_ok()
    }

    /// Warns when the firmware speaks another major protocol version.
    fn check_protocol_version(&self, message: &Message) {
        if let Message::Info(info) = message
            && info.protocol[0] != PROTOCOL_MAJOR
        {
            let warning = format!(
                "the lamp speaks protocol {}.{}, this app expects {}.x: update one of them",
                info.protocol[0], info.protocol[1], PROTOCOL_MAJOR
            );
            let _ = self.app.emit(EVENT_PROTOCOL_ERROR, warning);
        }
    }

    /// Reacts to the lamp's messages: settings re-applied once its
    /// capabilities are known, state shown in the tray icon. Returns false
    /// if the lamp is gone.
    fn follow(&mut self, device: &HidDevice, message: &Message) -> bool {
        match message {
            Message::Info(info) => {
                self.set_info(Some(info.clone()));
                let settings = self.app.state::<SettingsState>().get();
                for command in settings.commands(info.auto_bias) {
                    if !self.write(device, &command) {
                        return false;
                    }
                }
            }
            Message::State(state) => {
                let look = if state.on {
                    Look::On(state.color)
                } else {
                    Look::Off
                };
                tray::show(&self.app, look);
            }
            _ => {}
        }
        true
    }

    fn serve(&mut self, device: &HidDevice) -> Exit {
        // Learn who is there and what it shows.
        if !self.write(device, &Command::GetInfo) || !self.write(device, &Command::GetState) {
            return Exit::Disconnected;
        }
        let mut buffer = [0u8; PAYLOAD_SIZE + 1];
        loop {
            loop {
                match self.receiver.try_recv() {
                    Ok(command) => {
                        if !self.write(device, &command) {
                            return Exit::Disconnected;
                        }
                    }
                    Err(mpsc::TryRecvError::Empty) => break,
                    Err(mpsc::TryRecvError::Disconnected) => return Exit::AppClosed,
                }
            }
            match device.read_timeout(&mut buffer, READ_TIMEOUT_MS) {
                Ok(0) => {}
                Ok(length) if buffer[0] == REPORT_ID_MESSAGE => {
                    match protocol::decode_message(&buffer[1..length]) {
                        Ok(frame) => {
                            self.check_protocol_version(&frame.message);
                            if !self.follow(device, &frame.message) {
                                return Exit::Disconnected;
                            }
                            let _ = self.app.emit(EVENT_MESSAGE, frame);
                        }
                        // A newer lamp may send message types this app does
                        // not know: the protocol says to ignore them.
                        Err(DecodeError::UnknownType(_)) => {}
                        Err(error) => {
                            let _ = self.app.emit(EVENT_PROTOCOL_ERROR, error.to_string());
                        }
                    }
                }
                Ok(_) => {} // another report ID: not ours
                Err(_) => return Exit::Disconnected,
            }
        }
    }
}
