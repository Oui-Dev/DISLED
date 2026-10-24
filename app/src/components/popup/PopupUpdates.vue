<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  The update on offer, on top of the popup settings: the app first (a new
  firmware may need a new app), then the lamp. Nothing when all is up to
  date: checking by hand is in the settings menu.
-->
<script setup lang="ts">
import { useI18n } from 'vue-i18n';
import { useUpdateStatus } from '@/composables/useUpdateStatus';
import { openLampUpdate } from '@/ipc/commands';
import { useFirmwareStore } from '@/stores/firmware';
import { useUpdatesStore } from '@/stores/updates';

const { t } = useI18n();
const updates = useUpdatesStore();
const firmware = useFirmwareStore();
const { app, installing, appLine, lampLine } = useUpdateStatus();

/** Compact, as the popup's other alert, its button under the text. */
const alert = { root: 'gap-2 p-3', icon: 'size-4', description: 'text-xs', actions: 'mt-2' };
</script>

<template>
  <UAlert
    v-if="app.available"
    color="primary"
    variant="subtle"
    icon="i-lucide-download"
    orientation="vertical"
    :description="appLine ?? undefined"
    :actions="[{ label: t('updates.install'), loading: installing, onClick: updates.install }]"
    :ui="alert"
  />
  <UAlert
    v-else-if="firmware.updateForLamp"
    color="primary"
    variant="subtle"
    icon="i-lucide-lightbulb"
    orientation="vertical"
    :description="lampLine ?? undefined"
    :actions="[{ label: t('updates.updateLamp'), onClick: openLampUpdate }]"
    :ui="alert"
  />
</template>
