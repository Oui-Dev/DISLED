<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  Updates in the popup settings: the app first (a new firmware may need a
  new app), then the lamp; or a button to check for both.
-->
<script setup lang="ts">
import { computed } from 'vue';
import { useI18n } from 'vue-i18n';
import { openLampUpdate } from '@/ipc/commands';
import { useFirmwareStore } from '@/stores/firmware';
import { useLampStore } from '@/stores/lamp';
import { useUpdatesStore } from '@/stores/updates';

const { t, d, n } = useI18n();
const lamp = useLampStore();
const updates = useUpdatesStore();
const firmware = useFirmwareStore();

const app = computed(() => updates.status);
const installing = computed(() => app.value.progress !== null);
const checking = computed(() => app.value.checking || firmware.status.checking);

const appLine = computed(() => {
  const { available, progress, error, checked_at } = app.value;
  if (installing.value) {
    return t('updates.downloading', {
      version: available,
      progress: n((progress ?? 0) / 100, 'percent'),
    });
  }
  if (checking.value) {
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

const lampLine = computed(() => {
  const { checking, error } = firmware.status;
  if (checking) {
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
</script>

<template>
  <div class="flex flex-col items-start gap-1.5">
    <UButton
      v-if="app.available"
      size="sm"
      icon="i-lucide-download"
      :loading="installing"
      @click="updates.install"
    >
      {{ t('updates.install', { version: app.available }) }}
    </UButton>
    <UButton
      v-else-if="firmware.updateForLamp"
      size="sm"
      icon="i-lucide-lightbulb"
      @click="openLampUpdate"
    >
      {{ t('updates.updateLamp', { version: firmware.updateForLamp }) }}
    </UButton>
    <UButton
      v-else
      color="neutral"
      variant="outline"
      size="sm"
      icon="i-lucide-refresh-cw"
      :loading="checking"
      @click="checkAll"
    >
      {{ t('updates.check') }}
    </UButton>

    <div aria-live="polite" class="flex flex-col gap-1 text-xs text-muted">
      <p v-if="appLine" class="break-words">{{ appLine }}</p>
      <p v-if="lampLine" class="break-words">{{ lampLine }}</p>
    </div>

    <UButton
      v-if="!lamp.connection.connected && firmware.status.latest"
      variant="link"
      size="xs"
      class="px-0"
      @click="openLampUpdate"
    >
      {{ t('updates.reinstall') }}
    </UButton>
  </div>
</template>
