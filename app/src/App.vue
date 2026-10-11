<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors
-->
<script setup lang="ts">
import { onMounted } from "vue";
import DeviceHeader from "./components/DeviceHeader.vue";
import LampPanel from "./components/LampPanel.vue";
import TelemetryPanel from "./components/TelemetryPanel.vue";
import { startDevice } from "./lib/device";

onMounted(() => {
  void startDevice();
});
</script>

<template>
  <main class="layout">
    <DeviceHeader />
    <div class="columns">
      <LampPanel />
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
@media (max-width: 900px) {
  .columns {
    grid-template-columns: 1fr;
  }
}
</style>
