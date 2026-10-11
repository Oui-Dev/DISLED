// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

// Application-wide device store: connection, lamp state, telemetry history
// and recordings. One instance, shared by every component.

import { reactive, shallowRef, triggerRef } from "vue";
import { invoke } from "@tauri-apps/api/core";
import { listen } from "@tauri-apps/api/event";
import { save } from "@tauri-apps/plugin-dialog";
import type { Command, Connection, Frame, Info, LampState, SetState, Telemetry } from "./protocol";

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
    case "error":
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

export function setLamp(changes: SetState): Promise<void> {
  return send({ kind: "set_state", ...changes });
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

/** Subscribes to the backend events. Call once at start-up. */
export async function startDevice(): Promise<void> {
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
