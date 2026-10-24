<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  The settings page of the popup: start at login, update checks, color
  mode, language, updates, diagnostics, versions.
-->
<script setup lang="ts">
import { getVersion } from '@tauri-apps/api/app';
import { computed, onMounted, ref } from 'vue';
import { useI18n } from 'vue-i18n';
import type { ColorMode } from '@/bindings/ColorMode';
import type { Language } from '@/bindings/Language';
import { openDiagnostics } from '@/ipc/commands';
import { useLampStore } from '@/stores/lamp';
import { useSettingsStore } from '@/stores/settings';

const { t } = useI18n();
const lamp = useLampStore();
const settingsStore = useSettingsStore();
const settings = computed(() => settingsStore.settings);

const appVersion = ref('');
onMounted(async () => {
  appVersion.value = await getVersion();
});

const colorModes = computed(() => [
  { value: 'system', label: t('settings.colorModes.system'), icon: 'i-lucide-monitor' },
  { value: 'light', label: t('settings.colorModes.light'), icon: 'i-lucide-sun' },
  { value: 'dark', label: t('settings.colorModes.dark'), icon: 'i-lucide-moon' },
]);
// Each language under its own name, as language menus do.
const languages = computed(() => [
  { value: 'system', label: t('settings.systemLanguage') },
  { value: 'en', label: 'English' },
  { value: 'fr', label: 'Français' },
]);

/** A few short choices side by side, as cards sharing the width. */
const cards = { legend: 'text-sm', fieldset: 'flex-nowrap gap-2', item: 'min-w-0 flex-1' };
</script>

<template>
  <div class="flex flex-col gap-4">
    <div class="flex flex-col gap-3">
      <USwitch
        :model-value="settingsStore.autostart"
        :label="t('settings.autostart')"
        size="sm"
        @update:model-value="settingsStore.setAutostart"
      />
      <USwitch
        :model-value="settings?.check_updates ?? true"
        :label="t('settings.checkUpdates')"
        :disabled="!settings"
        size="sm"
        @update:model-value="settingsStore.update({ check_updates: $event })"
      />
    </div>

    <URadioGroup
      :model-value="settings?.color_mode"
      :items="colorModes"
      :legend="t('settings.colorMode')"
      variant="card"
      orientation="horizontal"
      indicator="hidden"
      size="sm"
      :disabled="!settings"
      :ui="cards"
      @update:model-value="settingsStore.update({ color_mode: $event as ColorMode })"
    />
    <URadioGroup
      :model-value="settings?.language"
      :items="languages"
      :legend="t('settings.language')"
      variant="card"
      orientation="horizontal"
      indicator="hidden"
      size="sm"
      :disabled="!settings"
      :ui="cards"
      @update:model-value="settingsStore.update({ language: $event as Language })"
    />

    <PopupUpdates />

    <div class="flex flex-col items-start gap-1.5">
      <UButton
        color="neutral"
        variant="outline"
        size="sm"
        icon="i-lucide-activity"
        @click="openDiagnostics"
      >
        {{ t('settings.diagnostics') }}
      </UButton>
      <p class="text-xs text-muted">
        {{
          t('settings.versions', {
            app: appVersion,
            firmware: lamp.info?.firmware_version ?? '–',
          })
        }}
      </p>
    </div>
  </div>
</template>
