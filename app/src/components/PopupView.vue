<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  The popup under the tray icon: everyday control of the lamp, and a small
  settings page.
-->
<script setup lang="ts">
import { computed, onMounted, ref } from "vue";
import { getVersion } from "@tauri-apps/api/app";
import {
  device,
  getAutostart,
  openDiagnostics,
  quitApp,
  setAutostart,
  setLamp,
  updateSettings,
} from "../lib/device";
import { checkFirmware, firmware, firmwareUpdateFor, openLampUpdate } from "../lib/firmware";
import { checkForUpdates, installUpdate, updates } from "../lib/updates";
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
const firmwareVersion = computed(() => device.info?.firmware_version ?? "–");
/** A newer firmware for the connected lamp. */
const lampUpdate = computed(() => firmwareUpdateFor(device.info));

const installing = computed(() => updates.progress !== null);
const checking = computed(() => updates.checking || firmware.checking);
const updateLine = computed(() => {
  if (installing.value) {
    return `Downloading ${updates.available}… ${updates.progress} %`;
  }
  if (checking.value) {
    return "Checking for updates…";
  }
  if (updates.error) {
    return `Update error: ${updates.error}`;
  }
  if (updates.available) {
    return `DISLED ${updates.available} is available.`;
  }
  if (updates.checked_at) {
    const at = new Date(updates.checked_at * 1000).toLocaleString([], {
      dateStyle: "short",
      timeStyle: "short",
    });
    return `Up to date (checked ${at}).`;
  }
  return "";
});
const lampLine = computed(() => {
  if (firmware.checking) {
    return "";
  }
  if (firmware.error) {
    return `Lamp firmware check error: ${firmware.error}`;
  }
  if (lampUpdate.value) {
    return `Lamp firmware ${lampUpdate.value} is available.`;
  }
  return "";
});
/** The app first: a new firmware may need a new app. */
const footerUpdate = computed(() => {
  if (updates.available) {
    return `Update to ${updates.available}`;
  }
  return lampUpdate.value ? "Update the lamp" : null;
});

function checkAll(): Promise<unknown> {
  return Promise.all([checkForUpdates(), checkFirmware()]);
}

onMounted(async () => {
  appVersion.value = await getVersion();
  autostart.value = await getAutostart();
});

async function toggleAutostart(event: Event): Promise<void> {
  autostart.value = await setAutostart((event.target as HTMLInputElement).checked);
}

function toggleUpdateChecks(event: Event): Promise<void> {
  return updateSettings({ check_updates: (event.target as HTMLInputElement).checked });
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
      <label class="option">
        <input
          type="checkbox"
          :checked="device.settings?.check_updates ?? true"
          :disabled="!device.settings"
          @change="toggleUpdateChecks"
        />
        Check for updates every day
      </label>
      <div class="updates">
        <button
          v-if="updates.available"
          type="button"
          class="install"
          :disabled="installing"
          @click="installUpdate"
        >
          Install {{ updates.available }} and restart
        </button>
        <button v-else-if="lampUpdate" type="button" class="install" @click="openLampUpdate">
          Update the lamp to {{ lampUpdate }}…
        </button>
        <button v-else type="button" :disabled="checking" @click="checkAll">Check for updates</button>
        <p v-if="updateLine" class="muted small">{{ updateLine }}</p>
        <p v-if="lampLine" class="muted small">{{ lampLine }}</p>
        <button v-if="!connected && firmware.latest" type="button" class="link small" @click="openLampUpdate">
          Lamp not detected? Reinstall its firmware…
        </button>
      </div>
      <button type="button" @click="openDiagnostics">Open diagnostics…</button>
      <p class="muted small">App {{ appVersion }} · lamp firmware {{ firmwareVersion }}</p>
    </section>

    <p v-if="device.lastError" class="warning">{{ device.lastError }}</p>

    <footer class="foot">
      <button type="button" class="link" @click="page = page === 'lamp' ? 'settings' : 'lamp'">
        {{ page === "lamp" ? "Settings" : "Back" }}
      </button>
      <button
        v-if="footerUpdate && page === 'lamp'"
        type="button"
        class="link update"
        @click="page = 'settings'"
      >
        {{ footerUpdate }}
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
.updates {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 6px;
}
.install {
  background: var(--accent);
  color: var(--on-accent);
  border-color: var(--accent);
}
.small {
  margin: 0;
  font-size: 12px;
  overflow-wrap: anywhere;
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
.link.update {
  font-weight: 600;
}
</style>
