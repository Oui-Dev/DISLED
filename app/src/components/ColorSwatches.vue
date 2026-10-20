<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  The four lamp colors. The lamp owns the state: the selection reflects the
  last STATE message.
-->
<script setup lang="ts">
import { computed } from "vue";
import { device, setLamp } from "../lib/device";
import { LAMP_COLORS, type LampColor } from "../lib/protocol";

defineProps<{ labels?: boolean }>();

const SWATCHES: Record<LampColor, { label: string; css: string }> = {
  green: { label: "Green", css: "#2ecc71" },
  orange: { label: "Orange", css: "#f39c12" },
  red: { label: "Red", css: "#e74c3c" },
  warm_white: { label: "Warm white", css: "#ffe2b8" },
};

const state = computed(() => device.state);
</script>

<template>
  <div class="colors">
    <button
      v-for="color in LAMP_COLORS"
      :key="color"
      type="button"
      class="color"
      :disabled="!state"
      :title="SWATCHES[color].label"
      :aria-label="SWATCHES[color].label"
      :aria-pressed="state?.color === color"
      @click="setLamp({ color, on: true })"
    >
      <span
        class="swatch"
        :class="{ selected: state?.color === color }"
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
.swatch {
  width: 36px;
  height: 36px;
  border-radius: 50%;
  border: 2px solid transparent;
}
.swatch.selected {
  border-color: var(--text);
  box-shadow: 0 0 0 3px var(--bg);
}
.name {
  font-size: 11px;
  color: var(--muted);
}
</style>
