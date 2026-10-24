// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

// Types of the messages exchanged with the Rust backend. They mirror
// crates/disled-protocol (serde, snake_case), itself specified by
// protocol/README.md.

export type LampColor = 'green' | 'orange' | 'red' | 'warm_white';
export type BrightnessMode = 'auto' | 'manual';
export type UsbLink = 'unconfigured' | 'configured' | 'suspended';
export type ChangeSource = 'boot' | 'touch' | 'host' | 'auto_brightness' | 'usb';
export type TouchState = 'calibrating' | 'idle' | 'pressed';

export const LAMP_COLORS: readonly LampColor[] = ['green', 'orange', 'red', 'warm_white'];

export interface SetState {
  on?: boolean;
  color?: LampColor;
  brightness_mode?: BrightnessMode;
  /** Percent, 1 to 100. */
  manual_level?: number;
}

export type Command =
  | { kind: 'get_info' }
  | { kind: 'get_state' }
  | ({ kind: 'set_state' } & SetState)
  | { kind: 'set_telemetry'; period_ms: number }
  | { kind: 'reset' }
  /** Percent, -50 to +50 (protocol 1.1, capability `auto_bias`). */
  | { kind: 'set_auto_bias'; bias: number }
  | { kind: 'get_diagnostics' }
  | { kind: 'self_test' }
  | { kind: 'enter_bootloader' };

export interface Info {
  protocol: [number, number];
  firmware: [number, number, number];
  firmware_version: string;
  board_revision: number;
  uid: string;
  telemetry: boolean;
  debug_console: boolean;
  /** Optional commands of protocol 1.1. */
  auto_bias: boolean;
  diagnostics: boolean;
  self_test: boolean;
  enter_bootloader: boolean;
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

export interface Diagnostics {
  uptime_ms: number;
  /** Since power-up (kept across resets). */
  boots: number;
  watchdog_resets: number;
  faults: number;
  last_fault: { mcause: number; mepc: number; mtval: number } | null;
  /** Smallest stack margin since boot, bytes. */
  stack_free_min: number;
  i2c_errors: number;
  light_sensor_probes: number;
  touch_samples_lost: number;
  touch_recalibrations: number;
  usb_suspends: number;
  /** Percent. */
  auto_bias: number;
  light_sensor_present: boolean;
  crystal_running: boolean;
}

export interface SelfTestResult {
  light_sensor_ok: boolean;
  touch_idle_ok: boolean;
  touch_pressed: boolean;
  completed: boolean;
  lux_centi: number;
  touch_baseline: number;
  touch_peak_permille: number;
}

export type Message =
  | ({ kind: 'info' } & Info)
  | ({ kind: 'state' } & LampState)
  | ({ kind: 'telemetry' } & Telemetry)
  | ({ kind: 'diagnostics' } & Diagnostics)
  | ({ kind: 'self_test_result' } & SelfTestResult)
  | { kind: 'ack'; command: number }
  | { kind: 'error'; command: number; error: string };

export interface Frame {
  seq: number;
  message: Message;
}

export interface Connection {
  connected: boolean;
  debug_build: boolean;
  serial: string | null;
}
