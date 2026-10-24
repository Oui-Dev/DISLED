<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  Lamp diagnostics (protocol 1.1 GET_DIAGNOSTICS), asked every 2 s while
  the window is open.
-->
<script setup lang="ts">
import { useIntervalFn } from '@vueuse/core';
import { computed } from 'vue';
import { useI18n } from 'vue-i18n';
import { useFaultText } from '@/composables/useFaultText';
import { useDiagnosticsStore } from '@/stores/diagnostics';
import { useLampStore } from '@/stores/lamp';

const REFRESH_MS = 2000;

const { t, n } = useI18n();
const lamp = useLampStore();
const diagnostics = useDiagnosticsStore();
const faultText = useFaultText();

const supported = computed(() => lamp.info?.diagnostics ?? false);

useIntervalFn(() => {
  if (lamp.connection.connected && supported.value) {
    void diagnostics.request();
  }
}, REFRESH_MS);

const rows = computed(() => {
  const d = diagnostics.diagnostics;
  if (!d) {
    return [];
  }
  const list = (...parts: string[]) => parts.join(', ');
  return [
    {
      label: t('diagnostics.sincePowerUp'),
      value: list(
        t('diagnostics.boots', d.boots),
        t('diagnostics.watchdogResets', d.watchdog_resets),
        t('diagnostics.faults', d.faults),
      ),
    },
    {
      label: t('diagnostics.lastFault'),
      value: d.last_fault ? faultText(d.last_fault) : t('diagnostics.none'),
    },
    { label: t('diagnostics.stackMargin'), value: t('diagnostics.stackFree', d.stack_free_min) },
    {
      label: t('diagnostics.lightSensor'),
      value: list(
        d.light_sensor_present ? t('diagnostics.present') : t('diagnostics.absent'),
        t('diagnostics.probes', d.light_sensor_probes),
        t('diagnostics.i2cErrors', d.i2c_errors),
      ),
    },
    {
      label: t('diagnostics.touch'),
      value: list(
        t('diagnostics.samplesLost', d.touch_samples_lost),
        t('diagnostics.rebaselines', d.touch_recalibrations),
      ),
    },
    {
      label: t('diagnostics.usb'),
      value: list(
        t('diagnostics.suspends', d.usb_suspends),
        ...(d.crystal_running ? [] : [t('diagnostics.crystalFailed')]),
      ),
    },
    { label: t('diagnostics.correction'), value: n(d.auto_bias / 100, 'signedPercent') },
    { label: t('diagnostics.uptime'), value: n(d.uptime_ms / 60_000, 'minutes') },
  ];
});
</script>

<template>
  <UCard>
    <template #header>
      <h2 class="font-semibold text-highlighted">{{ t('diagnostics.counters') }}</h2>
    </template>

    <p v-if="!lamp.connection.connected" class="text-sm text-muted">
      {{ t('diagnostics.noLamp') }}
    </p>
    <p v-else-if="!supported" class="text-sm text-muted">{{ t('diagnostics.unsupported') }}</p>
    <p v-else-if="!rows.length" class="text-sm text-muted">{{ t('diagnostics.reading') }}</p>
    <dl v-else class="grid grid-cols-[max-content_1fr] gap-x-4 gap-y-1.5 text-sm">
      <template v-for="row in rows" :key="row.label">
        <dt class="text-muted">{{ row.label }}</dt>
        <dd class="tabular-nums">{{ row.value }}</dd>
      </template>
    </dl>
  </UCard>
</template>
