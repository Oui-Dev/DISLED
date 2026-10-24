// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

// Settings saved on this computer by the backend, which applies the
// brightness settings to the lamp at every connection, and start at login.
// Every window keeps a copy, refreshed by `settings-changed`.

import { defineStore } from 'pinia';
import { ref } from 'vue';
import type { Settings } from '@/bindings/Settings';
import type { SettingsPatch } from '@/bindings/SettingsPatch';
import { autostartGet, autostartSet, settingsGet, settingsSet } from '@/ipc/commands';
import { on } from '@/ipc/events';
import { useErrorStore } from './errors';

export const useSettingsStore = defineStore('settings', () => {
  const errors = useErrorStore();

  /** Null until the backend answers. */
  const settings = ref<Settings | null>(null);
  const autostart = ref(false);

  async function update(patch: SettingsPatch): Promise<void> {
    try {
      settings.value = await settingsSet(patch);
      errors.clear();
    } catch (error) {
      errors.reportFailure(error);
    }
  }

  async function setAutostart(enabled: boolean): Promise<void> {
    try {
      autostart.value = await autostartSet(enabled);
    } catch (error) {
      errors.reportFailure(error);
      autostart.value = await autostartGet();
    }
  }

  /** Follows the backend events. Call once per window. */
  async function start(): Promise<void> {
    await on('settings-changed', (next) => {
      settings.value = next;
    });
    settings.value = await settingsGet();
    autostart.value = await autostartGet();
  }

  return { settings, autostart, update, setAutostart, start };
});
