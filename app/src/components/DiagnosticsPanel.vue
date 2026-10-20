<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  Lamp diagnostics (protocol 1.1 GET_DIAGNOSTICS), refreshed every 2 s.
-->
<script setup lang="ts">
import { computed, onMounted, onUnmounted } from "vue";
import { device, requestDiagnostics } from "../lib/device";

const REFRESH_MS = 2000;

const supported = computed(() => device.info?.diagnostics ?? false);
const d = computed(() => device.diagnostics);

function hex(value: number): string {
  return `0x${value.toString(16).padStart(8, "0")}`;
}

/** mcause 0x0A55nnnn: a failed assertion at source line nnnn. */
const lastFault = computed(() => {
  const fault = d.value?.last_fault;
  if (!fault) {
    return null;
  }
  if (fault.mcause >>> 16 === 0x0a55) {
    return `assertion failed at line ${fault.mcause & 0xffff} (called from ${hex(fault.mepc)})`;
  }
  return `mcause ${hex(fault.mcause)}, mepc ${hex(fault.mepc)}, mtval ${hex(fault.mtval)}`;
});

let timer: number | undefined;
onMounted(() => {
  timer = window.setInterval(() => {
    if (device.connection.connected && supported.value) {
      void requestDiagnostics();
    }
  }, REFRESH_MS);
});
onUnmounted(() => window.clearInterval(timer));
</script>

<template>
  <section class="panel">
    <h2>Diagnostics</h2>
    <p v-if="!device.connection.connected" class="muted">No lamp connected.</p>
    <p v-else-if="!supported" class="muted">This lamp firmware does not report diagnostics.</p>
    <p v-else-if="!d" class="muted">Reading…</p>
    <dl v-else class="grid">
      <dt>Since power-up</dt>
      <dd>{{ d.boots }} boots, {{ d.watchdog_resets }} watchdog resets, {{ d.faults }} faults</dd>
      <dt>Last fault</dt>
      <dd>{{ lastFault ?? "none" }}</dd>
      <dt>Stack margin</dt>
      <dd>{{ d.stack_free_min }} bytes never used</dd>
      <dt>Light sensor</dt>
      <dd>
        {{ d.light_sensor_present ? "present" : "absent" }}, {{ d.light_sensor_probes }} probes,
        {{ d.i2c_errors }} I2C errors
      </dd>
      <dt>Touch</dt>
      <dd>{{ d.touch_samples_lost }} samples lost, {{ d.touch_recalibrations }} re-baselines</dd>
      <dt>USB</dt>
      <dd>{{ d.usb_suspends }} suspends{{ d.crystal_running ? "" : ", crystal FAILED" }}</dd>
      <dt>Brightness correction</dt>
      <dd>{{ d.auto_bias }} %</dd>
      <dt>Uptime</dt>
      <dd>{{ (d.uptime_ms / 60000).toFixed(1) }} min</dd>
    </dl>
  </section>
</template>

<style scoped>
.grid {
  display: grid;
  grid-template-columns: max-content 1fr;
  gap: 6px 16px;
  margin: 12px 0 0;
}
dt {
  color: var(--muted);
}
dd {
  margin: 0;
  font-variant-numeric: tabular-nums;
}
</style>
