<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  The popup under the tray icon: everyday control of the lamp, and a small
  settings page.
-->
<script setup lang="ts">
import { computed, onMounted, ref } from "vue";
import { getVersion } from "@tauri-apps/api/app";
import { device, getAutostart, openDiagnostics, quitApp, setAutostart, setLamp } from "../lib/device";
import BrightnessControl from "./BrightnessControl.vue";
import ColorSwatches from "./ColorSwatches.vue";

const page = ref<"lamp" | "settings">("lamp");
const autostart = ref(false);
const appVersion = ref("");

const connected = computed(() => device.connection.connected);
const state = computed(() => device.state);
const status = computed(() => {
  if (!connected.value) {
    return "No lamp connected";
  }
  if (!state.value) {
    return "Connecting…";
  }
  return state.value.on ? "On" : "Off";
});
const firmware = computed(() => device.info?.firmware_version ?? "–");

onMounted(async () => {
  appVersion.value = await getVersion();
  autostart.value = await getAutostart();
});

async function toggleAutostart(event: Event): Promise<void> {
  autostart.value = await setAutostart((event.target as HTMLInputElement).checked);
}
</script>

<template>
  <main class="popup">
    <header class="head">
      <div class="title">
        <strong>DISLED</strong>
        <span class="muted">{{ status }}</span>
      </div>
      <button
        type="button"
        class="power"
        :class="{ active: state?.on }"
        :disabled="!state"
        :aria-pressed="state?.on ?? false"
        @click="state && setLamp({ on: !state.on })"
      >
        {{ state?.on ? "On" : "Off" }}
      </button>
    </header>

    <template v-if="page === 'lamp'">
      <p v-if="!connected" class="muted note">
        Plug the lamp into this computer. The settings below still apply then.
      </p>
      <ColorSwatches labels class="swatches" />
      <BrightnessControl />
    </template>

    <section v-else class="settings">
      <label class="option">
        <input type="checkbox" :checked="autostart" @change="toggleAutostart" />
        Start DISLED when I log in
      </label>
      <button type="button" @click="openDiagnostics">Open diagnostics…</button>
      <p class="muted versions">App {{ appVersion }} · lamp firmware {{ firmware }}</p>
    </section>

    <p v-if="device.lastError" class="warning">{{ device.lastError }}</p>

    <footer class="foot">
      <button type="button" class="link" @click="page = page === 'lamp' ? 'settings' : 'lamp'">
        {{ page === "lamp" ? "Settings" : "Back" }}
      </button>
      <button type="button" class="link" @click="quitApp">Quit</button>
    </footer>
  </main>
</template>

<style scoped>
.popup {
  box-sizing: border-box;
  height: 100vh;
  display: flex;
  flex-direction: column;
  gap: 16px;
  padding: 16px;
  background: var(--panel);
  overflow: hidden;
}
.head {
  display: flex;
  align-items: center;
  justify-content: space-between;
}
.title {
  display: flex;
  flex-direction: column;
}
.title strong {
  font-size: 16px;
}
.title .muted {
  font-size: 12px;
}
.power {
  min-width: 64px;
}
.power.active {
  background: var(--accent);
  color: var(--on-accent);
}
.note {
  margin: 0;
  font-size: 12px;
}
.swatches {
  justify-content: space-between;
}
.settings {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 12px;
}
.option {
  display: flex;
  align-items: center;
  gap: 8px;
}
.versions {
  margin: 0;
  font-size: 12px;
}
.warning {
  margin: 0;
  font-size: 12px;
  color: var(--warning);
}
.foot {
  margin-top: auto;
  display: flex;
  justify-content: space-between;
}
.link {
  border: none;
  background: none;
  padding: 4px 0;
  color: var(--accent);
}
</style>
