<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  A live chart of the telemetry history: newest sample on the right, drawn
  at most ten times per second whatever the stream period. The tooltip
  gives the lamp's uptime and the values.
-->
<script setup lang="ts">
import { useThrottleFn } from '@vueuse/core';
import { computed, shallowRef, watch } from 'vue';
import { useTelemetryStore } from '@/stores/telemetry';
import { type ChartRow, type ChartSeries, chartRows } from '@/utils/chart';

const props = defineProps<{
  title: string;
  description: string;
  series: ChartSeries[];
  height: number;
  /** The values on the axis and in the tooltip, with their unit. */
  format: (value: number) => string;
}>();

const REFRESH_MS = 100;

const telemetry = useTelemetryStore();
const rows = shallowRef<ChartRow[]>([]);
const categories = computed(() =>
  props.series.map(({ key, label, color }) => ({ key, label, color })),
);

const refresh = useThrottleFn(
  () => {
    rows.value = chartRows(telemetry.history, props.series);
  },
  REFRESH_MS,
  true,
);
watch([() => telemetry.history, () => props.series], refresh, { immediate: true });
</script>

<template>
  <figure class="flex flex-col gap-1">
    <figcaption class="text-xs font-medium text-muted">{{ title }}</figcaption>
    <UChart
      :data="rows"
      index="uptime"
      :categories="categories"
      :height="height"
      :x-axis="false"
      :legend="series.length > 1"
      :format="format"
      :aria-label="title"
      :aria-description="description"
    />
  </figure>
</template>
