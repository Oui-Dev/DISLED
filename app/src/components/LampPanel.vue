<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  Lamp controls of the Diagnostics window: power, the four colors and the
  brightness, with the state details.
-->
<script setup lang="ts">
import { computed } from "vue";
import { device, setLamp } from "../lib/device";
import BrightnessControl from "./BrightnessControl.vue";
import ColorSwatches from "./ColorSwatches.vue";

const state = computed(() => device.state);
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
        <ColorSwatches />
      </div>
      <BrightnessControl />
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
.status {
  margin: 12px 0 0;
  font-size: 12px;
}
</style>
