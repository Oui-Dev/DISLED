// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

//! DISLED USB HID protocol v1 (see `protocol/README.md`): command encoder
//! and message decoder. Pure functions, no I/O.

use serde::{Deserialize, Serialize};
use thiserror::Error;

pub const REPORT_ID_COMMAND: u8 = 1;
pub const REPORT_ID_MESSAGE: u8 = 2;
/// Payload bytes of one report, after the report ID.
pub const PAYLOAD_SIZE: usize = 63;
const HEADER_SIZE: usize = 3;
pub const PROTOCOL_MAJOR: u8 = 1;

const CMD_GET_INFO: u8 = 0x01;
const CMD_GET_STATE: u8 = 0x02;
const CMD_SET_STATE: u8 = 0x03;
const CMD_SET_TELEMETRY: u8 = 0x04;
const CMD_RESET: u8 = 0x05;

const MSG_INFO: u8 = 0x81;
const MSG_STATE: u8 = 0x82;
const MSG_TELEMETRY: u8 = 0x83;
const MSG_ACK: u8 = 0x8E;
const MSG_ERROR: u8 = 0x8F;

const FIELD_POWER: u8 = 0x01;
const FIELD_COLOR: u8 = 0x02;
const FIELD_BRIGHTNESS_MODE: u8 = 0x04;
const FIELD_MANUAL_LEVEL: u8 = 0x08;

/// Wire enums: the discriminant is the byte on the wire.
macro_rules! wire_enum {
    ($(#[$meta:meta])* $name:ident { $($variant:ident = $value:expr),+ $(,)? }) => {
        $(#[$meta])*
        #[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
        #[serde(rename_all = "snake_case")]
        pub enum $name { $($variant = $value),+ }

        impl $name {
            fn from_wire(value: u8) -> Result<Self, DecodeError> {
                match value {
                    $($value => Ok(Self::$variant),)+
                    _ => Err(DecodeError::BadValue(stringify!($name), value)),
                }
            }
        }
    };
}

wire_enum!(LampColor { Green = 0, Orange = 1, Red = 2, WarmWhite = 3 });
wire_enum!(BrightnessMode { Auto = 0, Manual = 1 });
wire_enum!(UsbLink { Unconfigured = 0, Configured = 1, Suspended = 2 });
wire_enum!(ChangeSource { Boot = 0, Touch = 1, Host = 2, AutoBrightness = 3, Usb = 4 });
wire_enum!(ResetCause {
    Unknown = 0,
    PowerOn = 1,
    Pin = 2,
    Software = 3,
    Watchdog = 4,
    WindowWatchdog = 5,
    LowPower = 6,
});
wire_enum!(TouchState { Calibrating = 0, Idle = 1, Pressed = 2 });
wire_enum!(ProtocolError { UnknownCommand = 1, BadLength = 2, BadValue = 3 });

/// Lamp settings to change; `None` leaves a setting as it is.
#[derive(Debug, Clone, Default, PartialEq, Eq, Serialize, Deserialize)]
pub struct SetState {
    pub on: Option<bool>,
    pub color: Option<LampColor>,
    pub brightness_mode: Option<BrightnessMode>,
    /// Percent, 1 to 100.
    pub manual_level: Option<u8>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(tag = "kind", rename_all = "snake_case")]
pub enum Command {
    GetInfo,
    GetState,
    SetState(SetState),
    SetTelemetry { period_ms: u16 },
    Reset,
}

impl Command {
    /// Encodes the command payload (without the report ID).
    pub fn encode(&self, seq: u8) -> [u8; PAYLOAD_SIZE] {
        let (kind, body): (u8, Vec<u8>) = match self {
            Command::GetInfo => (CMD_GET_INFO, vec![]),
            Command::GetState => (CMD_GET_STATE, vec![]),
            Command::SetState(s) => {
                let mut fields = 0;
                if s.on.is_some() {
                    fields |= FIELD_POWER;
                }
                if s.color.is_some() {
                    fields |= FIELD_COLOR;
                }
                if s.brightness_mode.is_some() {
                    fields |= FIELD_BRIGHTNESS_MODE;
                }
                if s.manual_level.is_some() {
                    fields |= FIELD_MANUAL_LEVEL;
                }
                (
                    CMD_SET_STATE,
                    vec![
                        fields,
                        u8::from(s.on.unwrap_or(false)),
                        s.color.map_or(0, |c| c as u8),
                        s.brightness_mode.map_or(0, |m| m as u8),
                        s.manual_level.unwrap_or(0),
                    ],
                )
            }
            Command::SetTelemetry { period_ms } => {
                (CMD_SET_TELEMETRY, period_ms.to_le_bytes().to_vec())
            }
            Command::Reset => (CMD_RESET, vec![]),
        };
        let mut payload = [0u8; PAYLOAD_SIZE];
        payload[0] = kind;
        payload[1] = seq;
        payload[2] = body.len() as u8;
        payload[HEADER_SIZE..HEADER_SIZE + body.len()].copy_from_slice(&body);
        payload
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct Fault {
    pub mcause: u32,
    pub mepc: u32,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct Info {
    pub protocol: [u8; 2],
    pub firmware: [u8; 3],
    pub firmware_version: String,
    pub board_revision: u8,
    /// Chip unique ID in hexadecimal, as in the USB serial number.
    pub uid: String,
    pub telemetry: bool,
    pub debug_console: bool,
    pub reset_cause: ResetCause,
    pub debug_build: bool,
    pub fault: Option<Fault>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct State {
    pub state_seq: u16,
    pub on: bool,
    pub color: LampColor,
    pub brightness_mode: BrightnessMode,
    pub manual_level: u8,
    /// Brightness in use, percent.
    pub applied_level: u8,
    pub link: UsbLink,
    pub source: ChangeSource,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct Telemetry {
    pub uptime_ms: u32,
    pub touch_raw: u16,
    pub touch_value: u16,
    pub touch_baseline: u16,
    pub touch_delta_permille: i16,
    pub touch_state: TouchState,
    pub lux_centi: u32,
    pub als_raw: u16,
    pub duty: [u16; 3],
    pub led_current_ma_x10: u16,
    pub cap_ma: u16,
    pub link: UsbLink,
    pub level_permille: u16,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
#[serde(tag = "kind", rename_all = "snake_case")]
pub enum Message {
    Info(Info),
    State(State),
    Telemetry(Telemetry),
    Ack { command: u8 },
    Error { command: u8, error: ProtocolError },
}

/// A decoded message and the sequence number it answers (0: unsolicited).
#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct Frame {
    pub seq: u8,
    pub message: Message,
}

#[derive(Debug, Error, PartialEq, Eq)]
pub enum DecodeError {
    #[error("payload too short ({0} bytes)")]
    TooShort(usize),
    #[error("body length {0} does not fit the report")]
    BadLength(usize),
    #[error("unknown message type 0x{0:02x}")]
    UnknownType(u8),
    #[error("message 0x{0:02x}: body of {1} bytes, expected {2}")]
    WrongBodySize(u8, usize, usize),
    #[error("invalid {0} value {1}")]
    BadValue(&'static str, u8),
}

/// Little-endian reader over a message body.
struct Reader<'a> {
    bytes: &'a [u8],
    pos: usize,
}

impl Reader<'_> {
    fn u8(&mut self) -> u8 {
        let value = self.bytes[self.pos];
        self.pos += 1;
        value
    }

    fn u16(&mut self) -> u16 {
        u16::from_le_bytes([self.u8(), self.u8()])
    }

    fn u32(&mut self) -> u32 {
        u32::from_le_bytes([self.u8(), self.u8(), self.u8(), self.u8()])
    }

    fn bytes(&mut self, count: usize) -> &[u8] {
        let slice = &self.bytes[self.pos..self.pos + count];
        self.pos += count;
        slice
    }
}

fn expect_size(kind: u8, body: &[u8], expected: usize) -> Result<(), DecodeError> {
    if body.len() == expected {
        Ok(())
    } else {
        Err(DecodeError::WrongBodySize(kind, body.len(), expected))
    }
}

/// Decodes a message payload (the bytes after the report ID).
pub fn decode_message(payload: &[u8]) -> Result<Frame, DecodeError> {
    if payload.len() < HEADER_SIZE {
        return Err(DecodeError::TooShort(payload.len()));
    }
    let (kind, seq, length) = (payload[0], payload[1], payload[2] as usize);
    if HEADER_SIZE + length > payload.len() || HEADER_SIZE + length > PAYLOAD_SIZE {
        return Err(DecodeError::BadLength(length));
    }
    let body = &payload[HEADER_SIZE..HEADER_SIZE + length];
    let mut r = Reader {
        bytes: body,
        pos: 0,
    };

    let message = match kind {
        MSG_INFO => {
            expect_size(kind, body, 56)?;
            let protocol = [r.u8(), r.u8()];
            let firmware = [r.u8(), r.u8(), r.u8()];
            let board_revision = r.u8();
            let uid = r.bytes(12).iter().map(|b| format!("{b:02X}")).collect();
            let capabilities = r.u32();
            let reset_cause = ResetCause::from_wire(r.u8())?;
            let flags = r.u8();
            let (mcause, mepc) = (r.u32(), r.u32());
            let text = r.bytes(24);
            let end = text.iter().position(|&b| b == 0).unwrap_or(text.len());
            Message::Info(Info {
                protocol,
                firmware,
                firmware_version: String::from_utf8_lossy(&text[..end]).into_owned(),
                board_revision,
                uid,
                telemetry: capabilities & 0x1 != 0,
                debug_console: capabilities & 0x2 != 0,
                reset_cause,
                debug_build: flags & 0x1 != 0,
                fault: (flags & 0x2 != 0).then_some(Fault { mcause, mepc }),
            })
        }
        MSG_STATE => {
            expect_size(kind, body, 9)?;
            Message::State(State {
                state_seq: r.u16(),
                on: r.u8() != 0,
                color: LampColor::from_wire(r.u8())?,
                brightness_mode: BrightnessMode::from_wire(r.u8())?,
                manual_level: r.u8(),
                applied_level: r.u8(),
                link: UsbLink::from_wire(r.u8())?,
                source: ChangeSource::from_wire(r.u8())?,
            })
        }
        MSG_TELEMETRY => {
            expect_size(kind, body, 32)?;
            Message::Telemetry(Telemetry {
                uptime_ms: r.u32(),
                touch_raw: r.u16(),
                touch_value: r.u16(),
                touch_baseline: r.u16(),
                touch_delta_permille: r.u16() as i16,
                touch_state: TouchState::from_wire(r.u8())?,
                lux_centi: r.u32(),
                als_raw: r.u16(),
                duty: [r.u16(), r.u16(), r.u16()],
                led_current_ma_x10: r.u16(),
                cap_ma: r.u16(),
                link: UsbLink::from_wire(r.u8())?,
                level_permille: r.u16(),
            })
        }
        MSG_ACK => {
            expect_size(kind, body, 1)?;
            Message::Ack { command: r.u8() }
        }
        MSG_ERROR => {
            expect_size(kind, body, 2)?;
            Message::Error {
                command: r.u8(),
                error: ProtocolError::from_wire(r.u8())?,
            }
        }
        other => return Err(DecodeError::UnknownType(other)),
    };
    Ok(Frame { seq, message })
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::Value;

    fn vectors() -> Value {
        let path = concat!(
            env!("CARGO_MANIFEST_DIR"),
            "/../../protocol/vectors/v1.json"
        );
        let text = std::fs::read_to_string(path).expect("protocol/vectors/v1.json");
        serde_json::from_str(&text).expect("valid JSON")
    }

    fn payload(hex: &str) -> [u8; PAYLOAD_SIZE] {
        let mut payload = [0u8; PAYLOAD_SIZE];
        for (i, pair) in hex.split_whitespace().enumerate() {
            payload[i] = u8::from_str_radix(pair, 16).expect("hex byte");
        }
        payload
    }

    fn u(v: &Value, key: &str) -> u64 {
        v[key].as_u64().unwrap_or_else(|| panic!("number {key}"))
    }

    #[test]
    fn protocol_version_matches_the_vectors() {
        assert_eq!(
            vectors()["protocol_version"][0].as_u64(),
            Some(u64::from(PROTOCOL_MAJOR))
        );
    }

    #[test]
    fn encodes_every_canonical_command_vector() {
        let doc = vectors();
        let mut checked = 0;
        for v in doc["commands"].as_array().expect("commands") {
            if v["status"] != "ok" || v["canonical"] == Value::Bool(false) {
                continue;
            }
            let command = match u(v, "type") as u8 {
                CMD_GET_INFO => Command::GetInfo,
                CMD_GET_STATE => Command::GetState,
                CMD_RESET => Command::Reset,
                CMD_SET_TELEMETRY => Command::SetTelemetry {
                    period_ms: u(v, "telemetry_period_ms") as u16,
                },
                CMD_SET_STATE => {
                    let s = &v["set_state"];
                    let fields = u(s, "fields") as u8;
                    Command::SetState(SetState {
                        on: (fields & FIELD_POWER != 0).then(|| s["on"].as_bool().expect("on")),
                        color: (fields & FIELD_COLOR != 0)
                            .then(|| LampColor::from_wire(u(s, "color") as u8).expect("color")),
                        brightness_mode: (fields & FIELD_BRIGHTNESS_MODE != 0).then(|| {
                            BrightnessMode::from_wire(u(s, "brightness_mode") as u8).expect("mode")
                        }),
                        manual_level: (fields & FIELD_MANUAL_LEVEL != 0)
                            .then(|| u(s, "manual_level") as u8),
                    })
                }
                other => panic!("unexpected command type {other}"),
            };
            let name = v["name"].as_str().unwrap_or("?");
            let expected = payload(v["bytes"].as_str().expect("bytes"));
            assert_eq!(command.encode(u(v, "seq") as u8), expected, "{name}");
            checked += 1;
        }
        assert!(checked >= 6, "too few command vectors checked: {checked}");
    }

    #[test]
    fn decodes_every_message_vector() {
        let doc = vectors();
        for v in doc["messages"].as_array().expect("messages") {
            let name = v["name"].as_str().unwrap_or("?");
            let frame = decode_message(&payload(v["bytes"].as_str().expect("bytes")))
                .unwrap_or_else(|e| panic!("{name}: {e}"));
            assert_eq!(u64::from(frame.seq), u(v, "seq"), "{name}");
            match (v["kind"].as_str(), &frame.message) {
                (Some("state"), Message::State(s)) => {
                    let e = &v["state"];
                    assert_eq!(u64::from(s.state_seq), u(e, "state_seq"), "{name}");
                    assert_eq!(s.on, e["on"].as_bool().unwrap_or(false), "{name}");
                    assert_eq!(s.color as u64, u(e, "color"), "{name}");
                    assert_eq!(s.brightness_mode as u64, u(e, "brightness_mode"), "{name}");
                    assert_eq!(u64::from(s.manual_level), u(e, "manual_level"), "{name}");
                    assert_eq!(
                        u64::from(s.applied_level),
                        u(e, "applied_percent"),
                        "{name}"
                    );
                    assert_eq!(s.link as u64, u(e, "link"), "{name}");
                    assert_eq!(s.source as u64, u(e, "source"), "{name}");
                }
                (Some("info"), Message::Info(i)) => {
                    let e = &v["info"];
                    assert_eq!(i.protocol, [1, 0], "{name}");
                    assert_eq!(
                        i.firmware,
                        [
                            u(e, "fw_major") as u8,
                            u(e, "fw_minor") as u8,
                            u(e, "fw_patch") as u8
                        ]
                    );
                    assert_eq!(u64::from(i.board_revision), u(e, "board_revision"));
                    assert_eq!(i.uid, e["uid"].as_str().unwrap_or("").to_uppercase());
                    assert_eq!(i.firmware_version, e["fw_version"].as_str().unwrap_or(""));
                    assert_eq!(i.reset_cause as u64, u(e, "reset_cause"));
                    let flags = u(e, "flags");
                    assert_eq!(i.debug_build, flags & 1 != 0);
                    assert_eq!(
                        i.fault,
                        (flags & 2 != 0).then(|| Fault {
                            mcause: u(e, "fault_mcause") as u32,
                            mepc: u(e, "fault_mepc") as u32
                        })
                    );
                }
                (Some("telemetry"), Message::Telemetry(t)) => {
                    let e = &v["telemetry"];
                    assert_eq!(u64::from(t.uptime_ms), u(e, "uptime_ms"));
                    assert_eq!(u64::from(t.touch_raw), u(e, "touch_raw"));
                    assert_eq!(u64::from(t.touch_value), u(e, "touch_value"));
                    assert_eq!(u64::from(t.touch_baseline), u(e, "touch_baseline"));
                    assert_eq!(
                        i64::from(t.touch_delta_permille),
                        e["touch_delta_permille"].as_i64().unwrap_or(0)
                    );
                    assert_eq!(t.touch_state as u64, u(e, "touch_state"));
                    assert_eq!(u64::from(t.lux_centi), u(e, "lux_centi"));
                    assert_eq!(u64::from(t.als_raw), u(e, "als_raw"));
                    let duty: Vec<u64> = e["duty"]
                        .as_array()
                        .expect("duty")
                        .iter()
                        .filter_map(Value::as_u64)
                        .collect();
                    assert_eq!(
                        t.duty.iter().map(|&d| u64::from(d)).collect::<Vec<_>>(),
                        duty
                    );
                    assert_eq!(u64::from(t.led_current_ma_x10), u(e, "led_current_ma_x10"));
                    assert_eq!(u64::from(t.cap_ma), u(e, "cap_ma"));
                    assert_eq!(t.link as u64, u(e, "link"));
                    assert_eq!(u64::from(t.level_permille), u(e, "level_permille"));
                }
                (Some("ack"), Message::Ack { command }) => {
                    assert_eq!(u64::from(*command), u(v, "command"), "{name}");
                }
                (Some("error"), Message::Error { command, error }) => {
                    assert_eq!(u64::from(*command), u(v, "command"), "{name}");
                    assert_eq!(*error as u64, u(v, "error"), "{name}");
                }
                (kind, message) => panic!("{name}: kind {kind:?} decoded as {message:?}"),
            }
        }
    }

    #[test]
    fn rejects_malformed_messages() {
        assert_eq!(decode_message(&[0x82, 0]), Err(DecodeError::TooShort(2)));
        let mut p = [0u8; PAYLOAD_SIZE];
        p[0] = 0x82;
        p[2] = 61;
        assert_eq!(decode_message(&p), Err(DecodeError::BadLength(61)));
        p[2] = 8;
        assert_eq!(
            decode_message(&p),
            Err(DecodeError::WrongBodySize(0x82, 8, 9))
        );
        p[0] = 0x42;
        assert_eq!(decode_message(&p), Err(DecodeError::UnknownType(0x42)));
        let state = [0x82, 0, 9, 0, 0, 1, 9, 0, 50, 50, 1, 1];
        let mut p = [0u8; PAYLOAD_SIZE];
        p[..state.len()].copy_from_slice(&state);
        assert_eq!(
            decode_message(&p),
            Err(DecodeError::BadValue("LampColor", 9))
        );
    }

    #[test]
    fn commands_deserialize_from_the_frontend_shape() {
        let command: Command =
            serde_json::from_str(r#"{"kind":"set_state","color":"warm_white","manual_level":40}"#)
                .expect("set_state");
        assert_eq!(
            command,
            Command::SetState(SetState {
                color: Some(LampColor::WarmWhite),
                manual_level: Some(40),
                ..SetState::default()
            })
        );
        let command: Command = serde_json::from_str(r#"{"kind":"set_telemetry","period_ms":100}"#)
            .expect("set_telemetry");
        assert_eq!(command, Command::SetTelemetry { period_ms: 100 });
    }
}
