<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  Minimal real-time line chart on a canvas: a few series, shared
  auto-scaled Y axis, newest sample on the right.
-->
<script setup lang="ts">
import { onBeforeUnmount, onMounted, ref, watch } from 'vue';

export interface Series {
  label: string;
  color: string;
  values: number[];
}

const props = defineProps<{ series: Series[]; height?: number; unit?: string }>();

const canvas = ref<HTMLCanvasElement | null>(null);
let frame = 0;
let observer: ResizeObserver | null = null;

function draw(): void {
  frame = 0;
  const element = canvas.value;
  const context = element?.getContext('2d');
  if (!element || !context) {
    return;
  }
  const ratio = window.devicePixelRatio || 1;
  const width = element.clientWidth;
  const height = props.height ?? 160;
  element.width = Math.round(width * ratio);
  element.height = Math.round(height * ratio);
  context.setTransform(ratio, 0, 0, ratio, 0, 0);
  context.clearRect(0, 0, width, height);

  const all = props.series.flatMap((s) => s.values);
  if (all.length === 0) {
    return;
  }
  let min = Math.min(...all);
  let max = Math.max(...all);
  if (max - min < 1e-9) {
    min -= 1;
    max += 1;
  }
  const pad = (max - min) * 0.08;
  min -= pad;
  max += pad;

  const style = getComputedStyle(element);
  const left = 52;
  const plotWidth = width - left - 8;
  const y = (v: number) => 8 + (1 - (v - min) / (max - min)) * (height - 16);

  context.font = '11px system-ui, sans-serif';
  context.fillStyle = style.getPropertyValue('--muted') || '#888';
  context.strokeStyle = style.getPropertyValue('--grid') || '#8883';
  context.lineWidth = 1;
  for (let i = 0; i <= 4; i++) {
    const value = min + ((max - min) * i) / 4;
    const py = y(value);
    context.beginPath();
    context.moveTo(left, py);
    context.lineTo(width - 8, py);
    context.stroke();
    context.fillText(value.toFixed(Math.abs(max - min) < 20 ? 1 : 0), 4, py + 4);
  }

  for (const s of props.series) {
    const count = s.values.length;
    if (count < 2) {
      continue;
    }
    context.strokeStyle = s.color;
    context.lineWidth = 1.5;
    context.beginPath();
    s.values.forEach((value, i) => {
      const px = left + (plotWidth * i) / (count - 1);
      if (i === 0) {
        context.moveTo(px, y(value));
      } else {
        context.lineTo(px, y(value));
      }
    });
    context.stroke();
  }
}

function schedule(): void {
  if (frame === 0) {
    frame = requestAnimationFrame(draw);
  }
}

watch(() => props.series, schedule, { deep: false });
onMounted(() => {
  observer = new ResizeObserver(schedule);
  if (canvas.value) {
    observer.observe(canvas.value);
  }
  schedule();
});
onBeforeUnmount(() => {
  observer?.disconnect();
  if (frame !== 0) {
    cancelAnimationFrame(frame);
  }
});
</script>

<template>
  <figure class="chart">
    <canvas ref="canvas" :style="{ height: `${height ?? 160}px` }" />
    <figcaption>
      <span v-for="s in series" :key="s.label" class="legend">
        <i :style="{ background: s.color }" />{{ s.label }}
      </span>
      <span v-if="unit" class="unit">{{ unit }}</span>
    </figcaption>
  </figure>
</template>

<style scoped>
.chart {
  margin: 0;
}
canvas {
  display: block;
  width: 100%;
}
figcaption {
  display: flex;
  gap: 14px;
  font-size: 12px;
  color: var(--muted);
  padding: 4px 0 0 52px;
}
.legend i {
  display: inline-block;
  width: 10px;
  height: 3px;
  margin-right: 5px;
  vertical-align: middle;
}
.unit {
  margin-left: auto;
}
</style>
