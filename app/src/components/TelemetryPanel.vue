<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  Live telemetry for the tuning campaigns (touch V4, light sensor V1,
  thermal V2) and CSV recordings of it.
-->
<script setup lang="ts">
import { storeToRefs } from 'pinia';
import { computed } from 'vue';
import { useLampStore } from '@/stores/lamp';
import { useTelemetryStore } from '@/stores/telemetry';
import LineChart from './LineChart.vue';

const lamp = useLampStore();
const telemetry = useTelemetryStore();
const { history } = storeToRefs(telemetry);
const device = computed(() => ({
  connection: lamp.connection,
  telemetry: telemetry.latest,
  telemetryPeriodMs: telemetry.periodMs,
  recording: telemetry.recording,
  recordedRows: telemetry.recordedRows,
}));
const setTelemetry = telemetry.setPeriod;
const startRecording = telemetry.startRecording;
const stopRecording = telemetry.stopRecording;
const exportRecording = () => telemetry.exportRecording('Save the recording');

const PERIODS = [
  { label: 'Off', ms: 0 },
  { label: '50 ms', ms: 50 },
  { label: '100 ms', ms: 100 },
  { label: '250 ms', ms: 250 },
  { label: '1 s', ms: 1000 },
];

const t = computed(() => device.value.telemetry);

const touchSeries = computed(() => [
  { label: 'touch average', color: '#3498db', values: history.value.map((s) => s.touch_value) },
  { label: 'baseline', color: '#95a5a6', values: history.value.map((s) => s.touch_baseline) },
]);
const deltaSeries = computed(() => [
  {
    label: 'drop below baseline',
    color: '#e67e22',
    values: history.value.map((s) => s.touch_delta_permille),
  },
]);
const lightSeries = computed(() => [
  { label: 'ambient light', color: '#f1c40f', values: history.value.map((s) => s.lux_centi / 100) },
]);

function onPeriod(event: Event): void {
  void setTelemetry(Number((event.target as HTMLSelectElement).value));
}
</script>

<template>
  <section class="panel">
    <div class="heading">
      <h2>Telemetry</h2>
      <select
        :value="device.telemetryPeriodMs"
        :disabled="!device.connection.connected"
        @change="onPeriod"
      >
        <option v-for="p in PERIODS" :key="p.ms" :value="p.ms">{{ p.label }}</option>
      </select>
    </div>

    <p v-if="!t" class="muted">Choose a period to stream measurements from the lamp.</p>
    <template v-else>
      <dl class="figures">
        <div>
          <dt>Touch</dt>
          <dd>{{ t.touch_state }} · {{ t.touch_delta_permille }} ‰</dd>
        </div>
        <div>
          <dt>Light</dt>
          <dd>{{ (t.lux_centi / 100).toFixed(1) }} lx</dd>
        </div>
        <div>
          <dt>LED current</dt>
          <dd>{{ (t.led_current_ma_x10 / 10).toFixed(1) }} / {{ t.cap_ma }} mA</dd>
        </div>
        <div>
          <dt>Level</dt>
          <dd>{{ (t.level_permille / 10).toFixed(1) }} %</dd>
        </div>
        <div>
          <dt>Duty R/G/B</dt>
          <dd>{{ t.duty.join(' / ') }}</dd>
        </div>
      </dl>

      <LineChart :series="touchSeries" :height="150" unit="TouchKey counts" />
      <LineChart :series="deltaSeries" :height="110" unit="per mille" />
      <LineChart :series="lightSeries" :height="110" unit="lux" />
    </template>

    <div class="recording">
      <button v-if="!device.recording" type="button" :disabled="!t" @click="startRecording">
        Record
      </button>
      <button v-else type="button" class="stop" @click="stopRecording">Stop</button>
      <span class="muted">{{ device.recordedRows }} rows</span>
      <button
        type="button"
        :disabled="device.recording || device.recordedRows === 0"
        @click="exportRecording"
      >
        Export CSV…
      </button>
    </div>
  </section>
</template>

<style scoped>
.heading {
  display: flex;
  align-items: center;
  justify-content: space-between;
}
.figures {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(140px, 1fr));
  gap: 8px 16px;
  margin: 12px 0;
}
.figures dt {
  font-size: 12px;
  color: var(--muted);
}
.figures dd {
  margin: 0;
  font-variant-numeric: tabular-nums;
}
.recording {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-top: 12px;
}
.stop {
  background: var(--warning);
  color: var(--on-accent);
}
</style>
