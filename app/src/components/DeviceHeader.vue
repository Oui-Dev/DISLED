<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  Connection status and device identity.
-->
<script setup lang="ts">
import { computed } from "vue";
import { device, send } from "../lib/device";

const firmware = computed(() => device.info?.firmware_version || "…");
const fault = computed(() => {
  const f = device.info?.fault;
  return f ? `mcause 0x${f.mcause.toString(16)}, mepc 0x${f.mepc.toString(16).padStart(8, "0")}` : null;
});

function reset(): void {
  if (window.confirm("Restart the lamp firmware?")) {
    void send({ kind: "reset" });
  }
}
</script>

<template>
  <header class="header">
    <div class="title">
      <span class="dot" :class="{ on: device.connection.connected }" />
      <h1>DISLED</h1>
      <span v-if="!device.connection.connected" class="muted">no lamp connected</span>
      <template v-else>
        <span class="muted">firmware {{ firmware }}</span>
        <span v-if="device.connection.debug_build" class="badge">debug build</span>
      </template>
    </div>
    <div v-if="device.connection.connected" class="details">
      <span class="muted">serial {{ device.connection.serial ?? "?" }}</span>
      <span v-if="device.info" class="muted">reset cause: {{ device.info.reset_cause }}</span>
      <button type="button" @click="reset">Restart</button>
    </div>
    <p v-if="fault" class="warning">The firmware crashed before its last restart ({{ fault }}).</p>
    <p v-if="device.lastError" class="warning">{{ device.lastError }}</p>
  </header>
</template>

<style scoped>
.header {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  justify-content: space-between;
  gap: 8px 16px;
}
.title,
.details {
  display: flex;
  align-items: center;
  gap: 12px;
}
h1 {
  font-size: 20px;
  margin: 0;
  letter-spacing: 0.04em;
}
.dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  background: var(--muted);
}
.dot.on {
  background: #2ecc71;
}
.badge {
  font-size: 11px;
  padding: 2px 8px;
  border-radius: 10px;
  background: var(--accent-soft);
}
.warning {
  flex-basis: 100%;
  margin: 0;
  color: var(--warning);
  font-size: 13px;
}
</style>
