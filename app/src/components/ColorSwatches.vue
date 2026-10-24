<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  Off, then the four lamp colors. The lamp owns the state: the selection
  reflects the last STATE message.
-->
<script setup lang="ts">
import { computed } from 'vue';
import type { LampColor } from '@/bindings/LampColor';
import { LAMP_COLORS } from '@/utils/lamp';
import { device, setLamp } from '../lib/device';

defineProps<{ labels?: boolean }>();

const SWATCHES: Record<LampColor, { label: string; css: string }> = {
  green: { label: 'Green', css: '#2ecc71' },
  orange: { label: 'Orange', css: '#f39c12' },
  red: { label: 'Red', css: '#e74c3c' },
  warm_white: { label: 'Warm white', css: '#ffe2b8' },
};

const state = computed(() => device.state);
</script>

<template>
  <div class="colors" :class="{ labelled: labels }">
    <button
      type="button"
      class="color"
      :disabled="!state"
      title="Off"
      aria-label="Off"
      :aria-pressed="state?.on === false"
      @click="setLamp({ on: false })"
    >
      <span class="swatch off" :class="{ selected: state?.on === false }" />
      <span v-if="labels" class="name">Off</span>
    </button>
    <button
      v-for="color in LAMP_COLORS"
      :key="color"
      type="button"
      class="color"
      :disabled="!state"
      :title="SWATCHES[color].label"
      :aria-label="SWATCHES[color].label"
      :aria-pressed="state?.on === true && state.color === color"
      @click="setLamp({ color, on: true })"
    >
      <span
        class="swatch"
        :class="{ selected: state?.on === true && state.color === color }"
        :style="{ background: SWATCHES[color].css }"
      />
      <span v-if="labels" class="name">{{ SWATCHES[color].label }}</span>
    </button>
  </div>
</template>

<style scoped>
.colors {
  display: flex;
  gap: 10px;
}
.color {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 4px;
  border: none;
  background: none;
  padding: 0;
}
/* With their names, the swatches share the width in equal columns: the
   gaps between the squares stay even whatever the length of the names. */
.labelled .color {
  flex: 1 1 0;
  min-width: 0;
}
.swatch {
  width: 36px;
  height: 36px;
  border-radius: 20%;
  border: 2px solid transparent;
}
/* Off: black, struck through in the grey of its outline. */
.swatch.off {
  border-color: var(--border);
  background:
    linear-gradient(
      to top right,
      transparent calc(50% - 0.75px),
      var(--border) calc(50% - 0.75px) calc(50% + 0.75px),
      transparent calc(50% + 0.75px)
    ),
    #16181c;
}
.swatch.selected {
  border-color: var(--text);
  box-shadow: 0 0 0 3px var(--bg);
}
.name {
  font-size: 11px;
  color: var(--muted);
  white-space: nowrap;
}
</style>
