<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  Production self-test (protocol 1.1 SELF_TEST): the lamp shows its colors,
  checks its light sensor and its touch, and reports.
-->
<script setup lang="ts">
import { computed } from "vue";
import { device, startSelfTest } from "../lib/device";

const supported = computed(() => device.info?.self_test ?? false);
const test = computed(() => device.selfTest);

const checks = computed(() => {
  const r = test.value.result;
  if (!r) {
    return [];
  }
  return [
    { label: "Light sensor", ok: r.light_sensor_ok, detail: `${(r.lux_centi / 100).toFixed(1)} lx` },
    { label: "Touch at rest", ok: r.touch_idle_ok, detail: `baseline ${r.touch_baseline}` },
    { label: "Touch on the lid", ok: r.touch_pressed, detail: `largest drop ${r.touch_peak_permille} ‰` },
    { label: "Test completed", ok: r.completed, detail: "" },
  ];
});
</script>

<template>
  <section class="panel">
    <h2>Self-test</h2>
    <p v-if="!device.connection.connected" class="muted">No lamp connected.</p>
    <p v-else-if="!supported" class="muted">This lamp firmware has no self-test.</p>
    <template v-else>
      <p class="muted">
        The lamp shows red, green, blue and warm white (check them), then glows blue: touch the lid within 10
        s.
      </p>
      <button type="button" :disabled="test.running" @click="startSelfTest">
        {{ test.running ? "Running…" : "Run the self-test" }}
      </button>
      <ul v-if="checks.length" class="checks">
        <li v-for="check in checks" :key="check.label" :class="check.ok ? 'ok' : 'fail'">
          <span class="mark">{{ check.ok ? "✓" : "✗" }}</span>
          {{ check.label }}
          <span class="muted">{{ check.detail }}</span>
        </li>
      </ul>
    </template>
  </section>
</template>

<style scoped>
.checks {
  list-style: none;
  padding: 0;
  margin: 12px 0 0;
  display: flex;
  flex-direction: column;
  gap: 4px;
}
.mark {
  display: inline-block;
  width: 1.2em;
  font-weight: 700;
}
.ok .mark {
  color: #16a34a;
}
.fail .mark {
  color: var(--warning);
}
</style>
