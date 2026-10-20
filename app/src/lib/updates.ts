// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

// App updates: the backend checks GitHub at start-up and then once a day;
// the popup shows the result and installs the update when asked.

import { reactive } from "vue";
import { invoke } from "@tauri-apps/api/core";
import { listen } from "@tauri-apps/api/event";

export interface UpdateStatus {
  /** A check is running. */
  checking: boolean;
  /** Version found by the last check, newer than this app. */
  available: string | null;
  /** Download progress of the update being installed, percent. */
  progress: number | null;
  /** Why the last check or install failed. */
  error: string | null;
  /** Last successful check, seconds since 1970-01-01 UTC. */
  checked_at: number | null;
}

export const updates = reactive<UpdateStatus>({
  checking: false,
  available: null,
  progress: null,
  error: null,
  checked_at: null,
});

export async function checkForUpdates(): Promise<void> {
  Object.assign(updates, await invoke<UpdateStatus>("update_check"));
}

/** Downloads and installs the update; the app restarts when it is done. */
export async function installUpdate(): Promise<void> {
  try {
    await invoke("update_install");
  } catch {
    // The status event carries the error.
  }
}

/** Subscribes to the backend events. Call once at start-up. */
export async function startUpdates(): Promise<void> {
  await listen<UpdateStatus>("update-status", (event) => {
    Object.assign(updates, event.payload);
  });
  Object.assign(updates, await invoke<UpdateStatus>("update_status"));
}
