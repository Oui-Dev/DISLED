// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

// Lamp firmware updates: the backend looks for a new firmware with the app
// updates and runs the update; the popup offers it, the update window
// guides it.

import { reactive } from "vue";
import { invoke } from "@tauri-apps/api/core";
import { listen } from "@tauri-apps/api/event";
import type { Info } from "./protocol";

export type FlashStage = "erasing" | "writing" | "verifying" | "restarting";

/** Where a lamp update stands. */
export type Flash =
  | { step: "idle" }
  | { step: "downloading" }
  /** The user holds BOOT and presses RESET, or the lamp restarts in its bootloader by itself. */
  | { step: "waiting_for_bootloader"; by_command: boolean }
  /** Windows: the bootloader needs the WinUSB driver (Zadig), once. */
  | { step: "driver_missing" }
  /** Linux: no access to the bootloader (udev rule). */
  | { step: "no_permission" }
  | { step: "several_bootloaders" }
  | { step: "other_chip"; chip: string }
  | { step: "flashing"; stage: FlashStage }
  | { step: "waiting_for_lamp" }
  | { step: "done"; version: string }
  | { step: "failed"; error: string };

export interface FirmwareStatus {
  /** A check is running. */
  checking: boolean;
  /** Newest firmware the app can install. */
  latest: string | null;
  /** Why the last check failed. */
  error: string | null;
  /** Last successful check, seconds since 1970-01-01 UTC. */
  checked_at: number | null;
  flash: Flash;
}

export const firmware = reactive<FirmwareStatus>({
  checking: false,
  latest: null,
  error: null,
  checked_at: null,
  flash: { step: "idle" },
});

/** The newest firmware, if the lamp runs an older one. */
export function firmwareUpdateFor(info: Info | null): string | null {
  if (!info || !firmware.latest) {
    return null;
  }
  const latest = firmware.latest.split(".").map(Number);
  for (let i = 0; i < 3; i++) {
    if (latest[i] !== info.firmware[i]) {
      return latest[i] > info.firmware[i] ? firmware.latest : null;
    }
  }
  return null;
}

export async function checkFirmware(): Promise<void> {
  Object.assign(firmware, await invoke<FirmwareStatus>("firmware_check"));
}

export async function installFirmware(): Promise<void> {
  try {
    await invoke("firmware_install");
  } catch (error) {
    firmware.flash = { step: "failed", error: String(error) };
  }
}

/** Stops an update that waits for the bootloader. */
export function cancelFirmware(): Promise<void> {
  return invoke("firmware_cancel");
}

export function openLampUpdate(): Promise<void> {
  return invoke("open_lamp_update");
}

export function openLink(link: "zadig" | "flashing_guide"): Promise<void> {
  return invoke("open_link", { link });
}

/** Subscribes to the backend events. Call once at start-up. */
export async function startFirmware(): Promise<void> {
  await listen<FirmwareStatus>("firmware-status", (event) => {
    Object.assign(firmware, event.payload);
  });
  Object.assign(firmware, await invoke<FirmwareStatus>("firmware_status"));
}
