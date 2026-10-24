<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  One page for every window: the window label picks the view and the
  stores it follows. main.ts has started the settings.
-->
<script setup lang="ts">
import { getCurrentWebviewWindow } from '@tauri-apps/api/webviewWindow';
import { computed, onMounted } from 'vue';
import { useAppearance } from '@/composables/useAppearance';
import { nuxtUiLocales } from '@/i18n';
import { useDiagnosticsStore } from '@/stores/diagnostics';
import { useFirmwareStore } from '@/stores/firmware';
import { useLampStore } from '@/stores/lamp';
import { useSettingsStore } from '@/stores/settings';
import { useTelemetryStore } from '@/stores/telemetry';
import { useUpdatesStore } from '@/stores/updates';
import DiagnosticsView from './components/DiagnosticsView.vue';
import LampUpdateView from './components/LampUpdateView.vue';
import PopupView from './components/PopupView.vue';

const label = getCurrentWebviewWindow().label;
const settings = useSettingsStore();
const nuxtUiLocale = computed(() => nuxtUiLocales[settings.locale]);

useAppearance();

onMounted(() => {
  void useLampStore().start();
  if (label === 'popup' || label === 'lamp-update') {
    void useFirmwareStore().start();
  }
  if (label === 'popup') {
    void useUpdatesStore().start();
  }
  if (label === 'diagnostics') {
    void useTelemetryStore().start();
    void useDiagnosticsStore().start();
  }
});
</script>

<template>
  <UApp :locale="nuxtUiLocale" :toaster="null">
    <PopupView v-if="label === 'popup'" />
    <LampUpdateView v-else-if="label === 'lamp-update'" />
    <DiagnosticsView v-else />
  </UApp>
</template>
