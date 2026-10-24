// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

// Lamp firmware updates: the backend looks for a new firmware with the app
// updates and runs the update; the popup offers it, the update window
// guides it.

import { invoke } from '@tauri-apps/api/core';
import { listen } from '@tauri-apps/api/event';
import { reactive } from 'vue';
import type { FirmwareStatus } from '@/bindings/FirmwareStatus';
import type { Info } from '@/bindings/Info';
import type { Link } from '@/bindings/Link';

export const firmware = reactive<FirmwareStatus>({
  checking: false,
  latest: null,
  error: null,
  checked_at: null,
  flash: { step: 'idle' },
});

/** The newest firmware, if the lamp runs an older one. */
export function firmwareUpdateFor(info: Info | null): string | null {
  if (!info || !firmware.latest) {
    return null;
  }
  const latest = firmware.latest.split('.').map(Number);
  for (let i = 0; i < 3; i++) {
    if (latest[i] !== info.firmware[i]) {
      return latest[i] > info.firmware[i] ? firmware.latest : null;
    }
  }
  return null;
}

export async function checkFirmware(): Promise<void> {
  Object.assign(firmware, await invoke<FirmwareStatus>('firmware_check'));
}

export async function installFirmware(): Promise<void> {
  try {
    await invoke('firmware_install');
  } catch (error) {
    firmware.flash = { step: 'failed', error: String(error) };
  }
}

/** Stops an update that waits for the bootloader. */
export function cancelFirmware(): Promise<void> {
  return invoke('firmware_cancel');
}

export function openLampUpdate(): Promise<void> {
  return invoke('open_lamp_update');
}

export function openLink(link: Link): Promise<void> {
  return invoke('open_link', { link });
}

/** Subscribes to the backend events. Call once at start-up. */
export async function startFirmware(): Promise<void> {
  await listen<FirmwareStatus>('firmware-status', (event) => {
    Object.assign(firmware, event.payload);
  });
  Object.assign(firmware, await invoke<FirmwareStatus>('firmware_status'));
}
