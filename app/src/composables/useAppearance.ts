// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

import { useColorMode } from '@vueuse/core';
import { watch, watchEffect } from 'vue';
import { setLocale } from '@/i18n';
import { useSettingsStore } from '@/stores/settings';

/**
 * Applies the color mode and the language of the settings to this window.
 * The backend applies the color mode to the window frames.
 */
export function useAppearance(): void {
  const settings = useSettingsStore();
  // The color mode Nuxt UI follows, as the `dark` class on <html>.
  const { store: colorMode } = useColorMode();

  watchEffect(() => {
    const mode = settings.settings?.color_mode;
    if (mode) {
      colorMode.value = mode === 'system' ? 'auto' : mode;
    }
  });
  watch(() => settings.locale, setLocale, { immediate: true });
}
