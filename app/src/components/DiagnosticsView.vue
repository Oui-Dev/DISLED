<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  The Diagnostics window: device identity, lamp controls, diagnostics,
  self-test, live telemetry and recordings.
-->
<script setup lang="ts">
import DeviceHeader from './DeviceHeader.vue';
import DiagnosticsPanel from './DiagnosticsPanel.vue';
import LampPanel from './LampPanel.vue';
import SelfTestPanel from './SelfTestPanel.vue';
import TelemetryPanel from './TelemetryPanel.vue';
</script>

<template>
  <main class="layout legacy">
    <DeviceHeader />
    <div class="columns">
      <div class="stack">
        <LampPanel />
        <DiagnosticsPanel />
        <SelfTestPanel />
      </div>
      <TelemetryPanel />
    </div>
  </main>
</template>

<style scoped>
.layout {
  display: flex;
  flex-direction: column;
  gap: 16px;
  padding: 20px 24px;
  max-width: 1200px;
  margin: 0 auto;
}
.columns {
  display: grid;
  grid-template-columns: minmax(300px, 2fr) 3fr;
  gap: 16px;
  align-items: start;
}
.stack {
  display: flex;
  flex-direction: column;
  gap: 16px;
}
@media (max-width: 900px) {
  .columns {
    grid-template-columns: 1fr;
  }
}
</style>
