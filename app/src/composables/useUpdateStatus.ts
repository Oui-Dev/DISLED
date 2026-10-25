// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

import { computed } from 'vue';
import { useI18n } from 'vue-i18n';
import { useFirmwareStore } from '@/stores/firmware';
import { useUpdatesStore } from '@/stores/updates';

/** Where the app and lamp updates stand, worded for the popup. */
export function useUpdateStatus() {
  const { t, d, n } = useI18n();
  const updates = useUpdatesStore();
  const firmware = useFirmwareStore();

  const app = computed(() => updates.status);
  const installing = computed(() => app.value.progress !== null);
  const checking = computed(() => app.value.checking || firmware.status.checking);

  /** The app: being installed, checked, failed, available or up to date; null before a check. */
  const appLine = computed(() => {
    const { available, progress, error, checked_at } = app.value;
    if (installing.value) {
      return t('updates.downloading', {
        version: available,
        progress: n((progress ?? 0) / 100, 'percent'),
      });
    }
    // An update on offer keeps its line, the text of the popup alert, while
    // a new check runs.
    if (checking.value && !available) {
      return t('updates.checking');
    }
    if (error) {
      return t('updates.error', { detail: error });
    }
    if (available) {
      return t('updates.available', { version: available });
    }
    if (checked_at) {
      return t('updates.upToDate', { date: d(new Date(checked_at * 1000), 'short') });
    }
    return null;
  });

  /** The lamp: a failed check or a newer firmware; null otherwise. */
  const lampLine = computed(() => {
    const { checking: lampChecking, error } = firmware.status;
    // A lamp update on offer keeps its line too.
    if (lampChecking && !firmware.updateForLamp) {
      return null;
    }
    if (error) {
      return t('updates.lampError', { detail: error });
    }
    return firmware.updateForLamp
      ? t('updates.lampAvailable', { version: firmware.updateForLamp })
      : null;
  });

  function checkAll(): Promise<unknown> {
    return Promise.all([updates.check(), firmware.check()]);
  }

  return { app, installing, checking, appLine, lampLine, checkAll };
}
