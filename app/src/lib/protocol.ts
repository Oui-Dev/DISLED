// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

// Types of the messages exchanged with the Rust backend. They mirror
// src-tauri/src/protocol.rs (serde, snake_case), itself specified by
// protocol/README.md.

export type LampColor = "green" | "orange" | "red" | "warm_white";
export type BrightnessMode = "auto" | "manual";
export type UsbLink = "unconfigured" | "configured" | "suspended";
export type ChangeSource = "boot" | "touch" | "host" | "auto_brightness" | "usb";
export type TouchState = "calibrating" | "idle" | "pressed";

export const LAMP_COLORS: readonly LampColor[] = ["green", "orange", "red", "warm_white"];

export interface SetState {
  on?: boolean;
  color?: LampColor;
  brightness_mode?: BrightnessMode;
  /** Percent, 1 to 100. */
  manual_level?: number;
}

export type Command =
  | { kind: "get_info" }
  | { kind: "get_state" }
  | ({ kind: "set_state" } & SetState)
  | { kind: "set_telemetry"; period_ms: number }
  | { kind: "reset" };

export interface Info {
  protocol: [number, number];
  firmware: [number, number, number];
  firmware_version: string;
  board_revision: number;
  uid: string;
  telemetry: boolean;
  debug_console: boolean;
  reset_cause: string;
  debug_build: boolean;
  fault: { mcause: number; mepc: number } | null;
}

export interface LampState {
  state_seq: number;
  on: boolean;
  color: LampColor;
  brightness_mode: BrightnessMode;
  manual_level: number;
  /** Brightness in use, percent. */
  applied_level: number;
  link: UsbLink;
  source: ChangeSource;
}

export interface Telemetry {
  uptime_ms: number;
  touch_raw: number;
  touch_value: number;
  touch_baseline: number;
  touch_delta_permille: number;
  touch_state: TouchState;
  lux_centi: number;
  als_raw: number;
  duty: [number, number, number];
  led_current_ma_x10: number;
  cap_ma: number;
  link: UsbLink;
  level_permille: number;
}

export type Message =
  | ({ kind: "info" } & Info)
  | ({ kind: "state" } & LampState)
  | ({ kind: "telemetry" } & Telemetry)
  | { kind: "ack"; command: number }
  | { kind: "error"; command: number; error: string };

export interface Frame {
  seq: number;
  message: Message;
}

export interface Connection {
  connected: boolean;
  debug_build: boolean;
  serial: string | null;
}
