<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  Brightness: automatic (with a "darker / brighter" correction of the
  curve) or a fixed level. These are settings: saved on this computer and
  applied to the lamp at every connection.
-->
<script setup lang="ts">
import { computed, ref, watch } from 'vue';
import { device, type SettingsPatch, updateSettings } from '../lib/device';

const settings = computed(() => device.settings);
const auto = computed(() => settings.value?.brightness_mode !== 'manual');
/* Unknown until INFO arrives: assume support rather than flash a warning. */
const biasSupported = computed(() => device.info?.auto_bias ?? true);

/* Slider values, kept locally while dragging so the thumb does not jump. */
const level = ref(50);
const bias = ref(0);
let editing = false;
watch(
  settings,
  (s) => {
    if (s && !editing) {
      level.value = s.manual_level;
      bias.value = s.auto_bias;
    }
  },
  { immediate: true },
);

let timer: number | undefined;
function later(patch: () => SettingsPatch): void {
  editing = true;
  window.clearTimeout(timer);
  timer = window.setTimeout(() => {
    editing = false;
    void updateSettings(patch());
  }, 120);
}

function toggleAuto(): void {
  if (auto.value) {
    // Keep the brightness the eye sees: manual starts at the applied level.
    const applied = device.state?.applied_level ?? level.value;
    void updateSettings({ brightness_mode: 'manual', manual_level: Math.max(1, applied) });
  } else {
    void updateSettings({ brightness_mode: 'auto' });
  }
}

const biasLabel = computed(() =>
  bias.value === 0 ? 'none' : `${bias.value > 0 ? '+' : ''}${bias.value} %`,
);
</script>

<template>
  <div class="brightness">
    <div class="line">
      <span class="label">Brightness</span>
      <label class="toggle">
        <input type="checkbox" :checked="auto" :disabled="!settings" @change="toggleAuto" />
        Automatic
      </label>
    </div>
    <div v-if="auto" class="line">
      <span class="end muted">Darker</span>
      <input
        v-model.number="bias"
        type="range"
        min="-50"
        max="50"
        step="5"
        aria-label="Correction of the automatic brightness"
        :disabled="!settings || !biasSupported"
        @input="later(() => ({ auto_bias: bias }))"
      />
      <span class="end muted">Brighter</span>
    </div>
    <div v-else class="line">
      <input
        v-model.number="level"
        type="range"
        min="1"
        max="100"
        aria-label="Brightness"
        :disabled="!settings"
        @input="later(() => ({ brightness_mode: 'manual', manual_level: level }))"
      />
      <span class="value">{{ level }} %</span>
    </div>
    <p class="hint muted">
      <template v-if="!auto">Fixed level</template>
      <template v-else-if="!biasSupported"
        >Follows the room light; correction needs a firmware update</template
      >
      <template v-else>
        Follows the room light<template v-if="device.state"
          >, now {{ device.state.applied_level }} %</template
        >
        · correction {{ biasLabel }}
      </template>
    </p>
  </div>
</template>

<style scoped>
.brightness {
  display: flex;
  flex-direction: column;
  gap: 8px;
}
.line {
  display: flex;
  align-items: center;
  gap: 10px;
}
.label {
  flex: 1;
  font-weight: 600;
}
.toggle {
  display: flex;
  align-items: center;
  gap: 6px;
}
input[type='range'] {
  flex: 1;
  min-width: 0;
}
.end {
  font-size: 12px;
}
.value {
  width: 44px;
  text-align: right;
  font-variant-numeric: tabular-nums;
}
.hint {
  margin: 0;
  font-size: 12px;
}
</style>
