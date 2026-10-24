<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  Live telemetry for the tuning campaigns (touch, light sensor, thermal):
  the stream period, the last figures, three charts, and CSV recordings.
-->
<script setup lang="ts">
import { computed } from 'vue';
import { useI18n } from 'vue-i18n';
import { useLampStore } from '@/stores/lamp';
import { useTelemetryStore } from '@/stores/telemetry';
import type { ChartSeries } from '@/utils/chart';

/** Stream periods offered, ms; 0 stops the stream. */
const PERIODS = [0, 50, 100, 250, 1000];

const { t, n } = useI18n();
const lamp = useLampStore();
const telemetry = useTelemetryStore();

const periods = computed(() =>
  PERIODS.map((ms) => ({
    value: ms,
    label:
      ms === 0 ? t('telemetry.off') : ms < 1000 ? n(ms, 'milliseconds') : n(ms / 1000, 'seconds'),
  })),
);

const permille = (value: number) => t('telemetry.permille', { value: n(Math.round(value)) });
const lux = (value: number) => t('telemetry.lux', { value: n(value, 'decimal') });
const count = (value: number) => n(Math.round(value));

const figures = computed(() => {
  const s = telemetry.latest;
  if (!s) {
    return [];
  }
  return [
    {
      label: t('telemetry.touch'),
      value: `${t(`enums.touchState.${s.touch_state}`)} · ${permille(s.touch_delta_permille)}`,
    },
    { label: t('telemetry.light'), value: lux(s.lux_centi / 100) },
    {
      label: t('telemetry.ledCurrent'),
      value: t('telemetry.current', {
        current: n(s.led_current_ma_x10 / 10, 'decimal'),
        cap: n(s.cap_ma),
      }),
    },
    { label: t('telemetry.level'), value: n(s.level_permille / 1000, 'percent') },
    { label: t('telemetry.duty'), value: s.duty.join(' / ') },
  ];
});

const charts = computed<
  {
    title: string;
    description: string;
    height: number;
    series: ChartSeries[];
    format: (value: number) => string;
  }[]
>(() => [
  {
    title: t('telemetry.touchChart'),
    description: t('telemetry.touchDescription'),
    height: 150,
    format: count,
    series: [
      {
        key: 'touch',
        label: t('telemetry.touchAverage'),
        color: 'var(--ui-chart-2)',
        value: (s) => s.touch_value,
      },
      {
        key: 'baseline',
        label: t('telemetry.baseline'),
        color: 'var(--ui-text-dimmed)',
        value: (s) => s.touch_baseline,
      },
    ],
  },
  {
    title: t('telemetry.deltaChart'),
    description: t('telemetry.deltaDescription'),
    height: 110,
    format: permille,
    series: [
      {
        key: 'delta',
        label: t('telemetry.deltaSeries'),
        color: 'var(--ui-chart-4)',
        value: (s) => s.touch_delta_permille,
      },
    ],
  },
  {
    title: t('telemetry.lightChart'),
    description: t('telemetry.lightDescription'),
    height: 110,
    format: lux,
    series: [
      {
        key: 'lux',
        label: t('telemetry.lightSeries'),
        color: 'var(--color-yellow-500, #efb100)',
        value: (s) => s.lux_centi / 100,
      },
    ],
  },
]);
</script>

<template>
  <UCard>
    <template #header>
      <div class="flex items-center justify-between gap-3">
        <h2 class="font-semibold text-highlighted">{{ t('telemetry.title') }}</h2>
        <USelect
          :model-value="telemetry.periodMs"
          :items="periods"
          :disabled="!lamp.connection.connected"
          :aria-label="t('telemetry.period')"
          class="w-32"
          @update:model-value="telemetry.setPeriod($event as number)"
        />
      </div>
    </template>

    <p v-if="!telemetry.latest" class="text-sm text-muted">{{ t('telemetry.empty') }}</p>
    <div v-else class="flex flex-col gap-5">
      <dl class="grid grid-cols-[repeat(auto-fill,minmax(9rem,1fr))] gap-x-4 gap-y-2">
        <div v-for="figure in figures" :key="figure.label">
          <dt class="text-xs text-muted">{{ figure.label }}</dt>
          <dd class="text-sm tabular-nums">{{ figure.value }}</dd>
        </div>
      </dl>
      <DiagnosticsChart
        v-for="chart in charts"
        :key="chart.title"
        :title="chart.title"
        :description="chart.description"
        :height="chart.height"
        :series="chart.series"
        :format="chart.format"
      />
    </div>

    <template #footer>
      <div class="flex flex-wrap items-center gap-3">
        <UButton
          v-if="!telemetry.recording"
          color="neutral"
          variant="outline"
          icon="i-lucide-circle-dot"
          :disabled="!telemetry.latest"
          @click="telemetry.startRecording"
        >
          {{ t('telemetry.record') }}
        </UButton>
        <UButton v-else color="error" icon="i-lucide-square" @click="telemetry.stopRecording">
          {{ t('telemetry.stop') }}
        </UButton>
        <span class="text-sm text-muted tabular-nums">
          {{ t('telemetry.rows', telemetry.recordedRows) }}
        </span>
        <UButton
          class="ms-auto"
          color="neutral"
          variant="outline"
          icon="i-lucide-file-down"
          :disabled="telemetry.recording || telemetry.recordedRows === 0"
          @click="telemetry.exportRecording(t('telemetry.saveTitle'))"
        >
          {{ t('telemetry.export') }}
        </UButton>
      </div>
    </template>
  </UCard>
</template>
