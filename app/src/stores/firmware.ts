// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

// Lamp firmware updates. The backend looks for a new firmware along with the
// app updates and runs the update; the popup offers it, and the lamp update
// window guides it.

import { defineStore } from 'pinia';
import { computed, ref } from 'vue';
import type { FirmwareStatus } from '@/bindings/FirmwareStatus';
import { firmwareCancel, firmwareCheck, firmwareInstall, firmwareStatus } from '@/ipc/commands';
import { on } from '@/ipc/events';
import { isNewer } from '@/utils/version';
import { useLampStore } from './lamp';

export const useFirmwareStore = defineStore('firmware', () => {
  const lamp = useLampStore();

  const status = ref<FirmwareStatus>({
    checking: false,
    latest: null,
    error: null,
    checked_at: null,
    flash: { step: 'idle' },
  });

  /** The newest firmware, when the connected lamp runs an older one. */
  const updateForLamp = computed(() => {
    const { latest } = status.value;
    const info = lamp.info;
    return latest && info && isNewer(latest, info.firmware) ? latest : null;
  });

  async function check(): Promise<void> {
    status.value = await firmwareCheck();
  }

  async function install(): Promise<void> {
    try {
      await firmwareInstall();
    } catch (error) {
      status.value = { ...status.value, flash: { step: 'failed', error: String(error) } };
    }
  }

  /** Stops an update that waits for the bootloader. */
  function cancel(): Promise<void> {
    return firmwareCancel();
  }

  /** Follows the backend events. Call once, in the popup or the update window. */
  async function start(): Promise<void> {
    await on('firmware-status', (next) => {
      status.value = next;
    });
    status.value = await firmwareStatus();
  }

  return { status, updateForLamp, check, install, cancel, start };
});
