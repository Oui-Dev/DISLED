<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  Production self-test (protocol 1.1 SELF_TEST): the lamp shows its colors,
  checks its light sensor and its touch, and reports.
-->
<script setup lang="ts">
import { computed } from 'vue';
import { useI18n } from 'vue-i18n';
import { useDiagnosticsStore } from '@/stores/diagnostics';
import { useLampStore } from '@/stores/lamp';

const { t, n } = useI18n();
const lamp = useLampStore();
const diagnostics = useDiagnosticsStore();

const supported = computed(() => lamp.info?.self_test ?? false);

const checks = computed(() => {
  const r = diagnostics.selfTestResult;
  if (!r) {
    return [];
  }
  return [
    {
      label: t('selfTest.lightSensor'),
      ok: r.light_sensor_ok,
      detail: t('telemetry.lux', { value: n(r.lux_centi / 100, 'decimal') }),
    },
    {
      label: t('selfTest.touchIdle'),
      ok: r.touch_idle_ok,
      detail: t('selfTest.baseline', { value: r.touch_baseline }),
    },
    {
      label: t('selfTest.touchPressed'),
      ok: r.touch_pressed,
      detail: t('selfTest.peak', {
        value: t('telemetry.permille', { value: r.touch_peak_permille }),
      }),
    },
    { label: t('selfTest.completed'), ok: r.completed, detail: '' },
  ];
});
</script>

<template>
  <UCard>
    <template #header>
      <h2 class="font-semibold text-highlighted">{{ t('selfTest.title') }}</h2>
    </template>

    <p v-if="!lamp.connection.connected" class="text-sm text-muted">
      {{ t('diagnostics.noLamp') }}
    </p>
    <p v-else-if="!supported" class="text-sm text-muted">{{ t('selfTest.unsupported') }}</p>
    <div v-else class="flex flex-col items-start gap-3">
      <p class="text-sm text-muted">{{ t('selfTest.instructions') }}</p>
      <UButton
        icon="i-lucide-play"
        :loading="diagnostics.selfTestRunning"
        @click="diagnostics.startSelfTest"
      >
        {{ diagnostics.selfTestRunning ? t('selfTest.running') : t('selfTest.run') }}
      </UButton>
      <ul aria-live="polite" class="flex flex-col gap-1.5">
        <li v-for="check in checks" :key="check.label" class="flex items-center gap-2 text-sm">
          <UIcon
            :name="check.ok ? 'i-lucide-circle-check' : 'i-lucide-circle-x'"
            class="size-4 shrink-0"
            :class="check.ok ? 'text-success' : 'text-error'"
          />
          <span class="sr-only">{{ check.ok ? t('selfTest.passed') : t('selfTest.failed') }}</span>
          <span>{{ check.label }}</span>
          <span class="text-muted">{{ check.detail }}</span>
        </li>
      </ul>
    </div>
  </UCard>
</template>
