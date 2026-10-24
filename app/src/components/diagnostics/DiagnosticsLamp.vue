<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  Lamp controls of the Diagnostics window: off and the four colors, the
  brightness, and the details of the last STATE.
-->
<script setup lang="ts">
import { useI18n } from 'vue-i18n';
import { useLampStore } from '@/stores/lamp';

const { t } = useI18n();
const lamp = useLampStore();
</script>

<template>
  <UCard>
    <template #header>
      <h2 class="font-semibold text-highlighted">{{ t('diagnostics.lamp') }}</h2>
    </template>

    <p v-if="!lamp.state" class="text-sm text-muted">{{ t('diagnostics.waiting') }}</p>
    <div v-else class="flex flex-col gap-4">
      <LampColorPicker class="max-w-72" />
      <LampBrightness />
      <p class="text-xs text-muted">
        {{
          t('diagnostics.state', {
            seq: lamp.state.state_seq,
            source: t(`enums.changeSource.${lamp.state.source}`),
            link: t(`enums.usbLink.${lamp.state.link}`),
          })
        }}
      </p>
    </div>
  </UCard>
</template>
