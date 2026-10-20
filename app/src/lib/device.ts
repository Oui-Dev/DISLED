// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

// Device store of one window: connection, lamp state, settings, telemetry
// history, recordings, diagnostics and self-test. Each window (popup,
// diagnostics) has its own instance, fed by the same backend events.

import { reactive, shallowRef, triggerRef } from "vue";
import { invoke } from "@tauri-apps/api/core";
import { listen } from "@tauri-apps/api/event";
import { save } from "@tauri-apps/plugin-dialog";
import type {
  BrightnessMode,
  Command,
  Connection,
  Diagnostics,
  Frame,
  Info,
  LampState,
  SelfTestResult,
  SetState,
  Telemetry,
} from "./protocol";

/** Settings kept on this computer and applied at every connection. */
export interface Settings {
  brightness_mode: BrightnessMode;
  /** Percent, 1 to 100. */
  manual_level: number;
  /** Automatic brightness bias, percent (-50 to +50). */
  auto_bias: number;
  autostart_initialized: boolean;
}

export type SettingsPatch = Partial<Pick<Settings, "brightness_mode" | "manual_level" | "auto_bias">>;

/** Telemetry samples kept for the charts. */
const HISTORY_LENGTH = 600;

const CSV_HEADER = [
  "uptime_ms",
  "touch_raw",
  "touch_value",
  "touch_baseline",
  "touch_delta_permille",
  "touch_state",
  "lux",
  "als_raw",
  "duty_r",
  "duty_g",
  "duty_b",
  "led_ma",
  "cap_ma",
  "link",
  "level_permille",
  "lamp_on",
  "color",
  "brightness_mode",
  "applied_level",
].join(",");

export const device = reactive({
  connection: { connected: false, debug_build: false, serial: null } as Connection,
  info: null as Info | null,
  state: null as LampState | null,
  telemetry: null as Telemetry | null,
  telemetryPeriodMs: 0,
  lastError: null as string | null,
  recording: false,
  recordedRows: 0,
  settings: null as Settings | null,
  diagnostics: null as Diagnostics | null,
  selfTest: { running: false, result: null as SelfTestResult | null },
});

/** Chart data, outside deep reactivity: it changes many times per second. */
export const history = shallowRef<Telemetry[]>([]);

let rows: string[] = [];

function recordRow(t: Telemetry): void {
  const s = device.state;
  rows.push(
    [
      t.uptime_ms,
      t.touch_raw,
      t.touch_value,
      t.touch_baseline,
      t.touch_delta_permille,
      t.touch_state,
      (t.lux_centi / 100).toFixed(2),
      t.als_raw,
      ...t.duty,
      (t.led_current_ma_x10 / 10).toFixed(1),
      t.cap_ma,
      t.link,
      t.level_permille,
      s ? Number(s.on) : "",
      s?.color ?? "",
      s?.brightness_mode ?? "",
      s?.applied_level ?? "",
    ].join(","),
  );
  device.recordedRows = rows.length;
}

function onFrame(frame: Frame): void {
  const message = frame.message;
  switch (message.kind) {
    case "info": {
      const { kind: _kind, ...info } = message;
      device.info = info;
      break;
    }
    case "state": {
      const { kind: _kind, ...state } = message;
      const previous = device.state;
      device.state = state;
      // Each message with a change increments state_seq by one; a larger
      // jump means a lost message, so ask for the full state again.
      if (
        previous &&
        state.state_seq !== previous.state_seq &&
        state.state_seq !== ((previous.state_seq + 1) & 0xffff)
      ) {
        void send({ kind: "get_state" });
      }
      break;
    }
    case "telemetry": {
      const { kind: _kind, ...telemetry } = message;
      device.telemetry = telemetry;
      const samples = history.value;
      samples.push(telemetry);
      if (samples.length > HISTORY_LENGTH) {
        samples.splice(0, samples.length - HISTORY_LENGTH);
      }
      triggerRef(history);
      if (device.recording) {
        recordRow(telemetry);
      }
      break;
    }
    case "diagnostics": {
      const { kind: _kind, ...diagnostics } = message;
      device.diagnostics = diagnostics;
      break;
    }
    case "self_test_result": {
      const { kind: _kind, ...result } = message;
      device.selfTest = { running: false, result };
      break;
    }
    case "error":
      if (message.command === COMMAND_SELF_TEST) {
        device.selfTest.running = false;
      }
      device.lastError = `the lamp rejected command 0x${message.command.toString(16)}: ${message.error}`;
      break;
    case "ack":
      break;
  }
}

export async function send(command: Command): Promise<void> {
  try {
    await invoke("device_send", { command });
    device.lastError = null;
  } catch (error) {
    device.lastError = String(error);
  }
}

/** Power and color: the lamp's own state, not saved on this computer. */
export function setLamp(changes: Pick<SetState, "on" | "color">): Promise<void> {
  return send({ kind: "set_state", ...changes });
}

/** Brightness mode, manual level, bias: saved, then applied to the lamp. */
export async function updateSettings(patch: SettingsPatch): Promise<void> {
  try {
    device.settings = await invoke<Settings>("settings_set", { patch });
    device.lastError = null;
  } catch (error) {
    device.lastError = String(error);
  }
}

export function getAutostart(): Promise<boolean> {
  return invoke<boolean>("autostart_get");
}

export async function setAutostart(enabled: boolean): Promise<boolean> {
  try {
    return await invoke<boolean>("autostart_set", { enabled });
  } catch (error) {
    device.lastError = String(error);
    return getAutostart();
  }
}

export function requestDiagnostics(): Promise<void> {
  return send({ kind: "get_diagnostics" });
}

export function startSelfTest(): Promise<void> {
  device.selfTest = { running: true, result: null };
  return send({ kind: "self_test" });
}

export function openDiagnostics(): Promise<void> {
  return invoke("open_diagnostics");
}

export function quitApp(): Promise<void> {
  return invoke("quit");
}

export async function setTelemetry(periodMs: number): Promise<void> {
  device.telemetryPeriodMs = periodMs;
  await send({ kind: "set_telemetry", period_ms: periodMs });
}

export function startRecording(): void {
  rows = [];
  device.recordedRows = 0;
  device.recording = true;
}

export function stopRecording(): void {
  device.recording = false;
}

export async function exportRecording(): Promise<void> {
  const path = await save({
    title: "Save the recording",
    defaultPath: `disled-${new Date().toISOString().slice(0, 19).replace(/[:T]/g, "-")}.csv`,
    filters: [{ name: "CSV", extensions: ["csv"] }],
  });
  if (path === null) {
    return;
  }
  try {
    await invoke("save_csv", { path, contents: [CSV_HEADER, ...rows].join("\n") + "\n" });
  } catch (error) {
    device.lastError = String(error);
  }
}

const COMMAND_SELF_TEST = 0x08;

/** Subscribes to the backend events. Call once at start-up. */
export async function startDevice(): Promise<void> {
  await listen<Settings>("settings-changed", (event) => {
    device.settings = event.payload;
  });
  device.settings = await invoke<Settings>("settings_get");
  await listen<Connection>("device-connection", (event) => {
    device.connection = event.payload;
    if (event.payload.connected) {
      device.lastError = null;
      if (device.telemetryPeriodMs > 0) {
        void setTelemetry(device.telemetryPeriodMs);
      }
    } else {
      device.info = null;
      device.state = null;
      device.telemetry = null;
      device.diagnostics = null;
      device.selfTest = { running: false, result: null };
    }
  });
  await listen<Frame>("device-message", (event) => onFrame(event.payload));
  await listen<string>("device-protocol-error", (event) => {
    device.lastError = `unreadable message: ${event.payload}`;
  });
  device.connection = await invoke<Connection>("device_connection");
  if (device.connection.connected) {
    await send({ kind: "get_info" });
    await send({ kind: "get_state" });
  }
}
