<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  Lamp controls: power, the four colors, automatic or manual brightness.
  The lamp owns the state; every control reflects the last STATE message.
-->
<script setup lang="ts">
import { computed, ref, watch } from "vue";
import { device, setLamp } from "../lib/device";
import { LAMP_COLORS, type LampColor } from "../lib/protocol";

const SWATCHES: Record<LampColor, { label: string; css: string }> = {
  green: { label: "Green", css: "#2ecc71" },
  orange: { label: "Orange", css: "#f39c12" },
  red: { label: "Red", css: "#e74c3c" },
  warm_white: { label: "Warm white", css: "#ffe2b8" },
};

const state = computed(() => device.state);
const auto = computed(() => state.value?.brightness_mode === "auto");

/* Slider value: the applied level in automatic mode, else the manual level
 * (kept locally while dragging so the thumb does not jump back). */
const level = ref(50);
let dragging = false;
watch(
  state,
  (s) => {
    if (s && !dragging) {
      level.value = s.brightness_mode === "auto" ? s.applied_level : s.manual_level;
    }
  },
  { immediate: true },
);

let timer: number | undefined;
function onSlide(): void {
  dragging = true;
  window.clearTimeout(timer);
  timer = window.setTimeout(() => {
    dragging = false;
    void setLamp({ brightness_mode: "manual", manual_level: level.value });
  }, 120);
}

function toggleAuto(): void {
  if (auto.value) {
    void setLamp({ brightness_mode: "manual", manual_level: state.value?.applied_level || 50 });
  } else {
    void setLamp({ brightness_mode: "auto" });
  }
}
</script>

<template>
  <section class="panel">
    <h2>Lamp</h2>
    <p v-if="!state" class="muted">Waiting for the lamp…</p>
    <template v-else>
      <div class="row">
        <button type="button" class="power" :class="{ active: state.on }" @click="setLamp({ on: !state.on })">
          {{ state.on ? "On" : "Off" }}
        </button>
        <div class="colors">
          <button
            v-for="color in LAMP_COLORS"
            :key="color"
            type="button"
            class="swatch"
            :class="{ selected: state.color === color }"
            :style="{ background: SWATCHES[color].css }"
            :title="SWATCHES[color].label"
            :aria-label="SWATCHES[color].label"
            @click="setLamp({ color })"
          />
        </div>
      </div>

      <div class="row brightness">
        <label class="toggle">
          <input type="checkbox" :checked="auto" @change="toggleAuto" />
          Automatic brightness
        </label>
        <input v-model.number="level" type="range" min="1" max="100" :disabled="auto" @input="onSlide" />
        <span class="value">{{ level }} %</span>
      </div>

      <p class="muted status">
        state #{{ state.state_seq }} · last change: {{ state.source.replace("_", " ") }} · USB
        {{ state.link }}
      </p>
    </template>
  </section>
</template>

<style scoped>
.row {
  display: flex;
  align-items: center;
  gap: 16px;
  margin: 12px 0;
}
.power {
  min-width: 72px;
}
.power.active {
  background: var(--accent);
  color: var(--on-accent);
}
.colors {
  display: flex;
  gap: 10px;
}
.swatch {
  width: 36px;
  height: 36px;
  border-radius: 50%;
  border: 2px solid transparent;
  padding: 0;
}
.swatch.selected {
  border-color: var(--text);
  box-shadow: 0 0 0 3px var(--bg);
}
.brightness input[type="range"] {
  flex: 1;
}
.toggle {
  display: flex;
  align-items: center;
  gap: 6px;
  white-space: nowrap;
}
.value {
  width: 48px;
  text-align: right;
  font-variant-numeric: tabular-nums;
}
.status {
  font-size: 12px;
}
</style>
