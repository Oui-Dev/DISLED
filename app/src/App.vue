<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  One page for every window: the window label picks the view, loaded on its
  own, and the stores it follows. main.ts has started the settings.
-->
<script setup lang="ts">
import { getCurrentWebviewWindow } from '@tauri-apps/api/webviewWindow';
import { type Component, computed, defineAsyncComponent, onMounted } from 'vue';
import { useAppearance } from '@/composables/useAppearance';
import { nuxtUiLocales } from '@/i18n';
import { useDiagnosticsStore } from '@/stores/diagnostics';
import { useFirmwareStore } from '@/stores/firmware';
import { useLampStore } from '@/stores/lamp';
import { useSettingsStore } from '@/stores/settings';
import { useTelemetryStore } from '@/stores/telemetry';
import { useUpdatesStore } from '@/stores/updates';

interface WindowSetup {
  view: Component;
  stores: (() => { start(): Promise<void> })[];
}

/** The windows of src-tauri/src/windows.rs. */
const WINDOWS: Record<string, WindowSetup> = {
  popup: {
    view: defineAsyncComponent(() => import('@/views/PopupView.vue')),
    stores: [useLampStore, useFirmwareStore, useUpdatesStore],
  },
  diagnostics: {
    view: defineAsyncComponent(() => import('@/views/DiagnosticsView.vue')),
    stores: [useLampStore, useTelemetryStore, useDiagnosticsStore],
  },
  'lamp-update': {
    view: defineAsyncComponent(() => import('@/views/LampUpdateView.vue')),
    stores: [useLampStore, useFirmwareStore],
  },
};

const current = WINDOWS[getCurrentWebviewWindow().label] ?? WINDOWS.diagnostics;
const settings = useSettingsStore();
const nuxtUiLocale = computed(() => nuxtUiLocales[settings.locale]);

useAppearance();

onMounted(() => {
  for (const useStore of current.stores) {
    void useStore().start();
  }
});
</script>

<template>
  <UApp :locale="nuxtUiLocale" :toaster="null">
    <component :is="current.view" />
  </UApp>
</template>
