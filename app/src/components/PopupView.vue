<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  The popup under the tray icon: everyday control of the lamp, and a small
  settings page.
-->
<script setup lang="ts">
import { getVersion } from '@tauri-apps/api/app';
import { storeToRefs } from 'pinia';
import { computed, onMounted, ref } from 'vue';
import { useErrorText } from '@/composables/useErrorText';
import { openDiagnostics, openLampUpdate, quit } from '@/ipc/commands';
import { useErrorStore } from '@/stores/errors';
import { useFirmwareStore } from '@/stores/firmware';
import { useLampStore } from '@/stores/lamp';
import { useSettingsStore } from '@/stores/settings';
import { useUpdatesStore } from '@/stores/updates';
import BrightnessControl from './BrightnessControl.vue';
import ColorSwatches from './ColorSwatches.vue';

const lamp = useLampStore();
const settingsStore = useSettingsStore();
const updatesStore = useUpdatesStore();
const firmwareStore = useFirmwareStore();
const errors = useErrorStore();
const errorText = useErrorText();
const { settings, autostart } = storeToRefs(settingsStore);
const updates = computed(() => updatesStore.status);
const firmware = computed(() => firmwareStore.status);

const page = ref<'lamp' | 'settings'>('lamp');
const appVersion = ref('');

const connected = computed(() => lamp.connection.connected);
const status = computed(() => {
  if (!connected.value) {
    return 'No lamp connected';
  }
  return lamp.state ? 'Lamp connected' : 'Connecting…';
});
const firmwareVersion = computed(() => lamp.info?.firmware_version ?? '–');
/** A newer firmware for the connected lamp. */
const lampUpdate = computed(() => firmwareStore.updateForLamp);

const installing = computed(() => updates.value.progress !== null);
const checking = computed(() => updates.value.checking || firmware.value.checking);
const updateLine = computed(() => {
  const u = updates.value;
  if (installing.value) {
    return `Downloading ${u.available}… ${u.progress} %`;
  }
  if (checking.value) {
    return 'Checking for updates…';
  }
  if (u.error) {
    return `Update error: ${u.error}`;
  }
  if (u.available) {
    return `DISLED ${u.available} is available.`;
  }
  if (u.checked_at) {
    const at = new Date(u.checked_at * 1000).toLocaleString([], {
      dateStyle: 'short',
      timeStyle: 'short',
    });
    return `Up to date (checked ${at}).`;
  }
  return '';
});
const lampLine = computed(() => {
  if (firmware.value.checking) {
    return '';
  }
  if (firmware.value.error) {
    return `Lamp firmware check error: ${firmware.value.error}`;
  }
  if (lampUpdate.value) {
    return `Lamp firmware ${lampUpdate.value} is available.`;
  }
  return '';
});
/** The app first: a new firmware may need a new app. */
const footerUpdate = computed(() => {
  if (updates.value.available) {
    return `Update to ${updates.value.available}`;
  }
  return lampUpdate.value ? 'Update the lamp' : null;
});
const errorLine = computed(() => (errors.last ? errorText(errors.last) : null));

function checkAll(): Promise<unknown> {
  return Promise.all([updatesStore.check(), firmwareStore.check()]);
}

onMounted(async () => {
  appVersion.value = await getVersion();
});

function toggleAutostart(event: Event): Promise<void> {
  return settingsStore.setAutostart((event.target as HTMLInputElement).checked);
}

function toggleUpdateChecks(event: Event): Promise<void> {
  return settingsStore.update({ check_updates: (event.target as HTMLInputElement).checked });
}
</script>

<template>
  <main class="popup">
    <header class="head">
      <div class="title">
        <strong>DISLED</strong>
        <span class="muted">{{ status }}</span>
      </div>
    </header>

    <template v-if="page === 'lamp'">
      <p v-if="!connected" class="muted note">
        Plug the lamp into this computer. The settings below still apply then.
      </p>
      <ColorSwatches labels />
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
          :checked="settings?.check_updates ?? true"
          :disabled="!settings"
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
          @click="updatesStore.install"
        >
          Install {{ updates.available }} and restart
        </button>
        <button v-else-if="lampUpdate" type="button" class="install" @click="openLampUpdate">
          Update the lamp to {{ lampUpdate }}…
        </button>
        <button v-else type="button" :disabled="checking" @click="checkAll">
          Check for updates
        </button>
        <p v-if="updateLine" class="muted small">{{ updateLine }}</p>
        <p v-if="lampLine" class="muted small">{{ lampLine }}</p>
        <button
          v-if="!connected && firmware.latest"
          type="button"
          class="link small"
          @click="openLampUpdate"
        >
          Lamp not detected? Reinstall its firmware…
        </button>
      </div>
      <button type="button" @click="openDiagnostics">Open diagnostics…</button>
      <p class="muted small">App {{ appVersion }} · lamp firmware {{ firmwareVersion }}</p>
    </section>

    <p v-if="errorLine" class="warning">{{ errorLine }}</p>

    <footer class="foot">
      <button type="button" class="link" @click="page = page === 'lamp' ? 'settings' : 'lamp'">
        {{ page === 'lamp' ? 'Settings' : 'Back' }}
      </button>
      <button
        v-if="footerUpdate && page === 'lamp'"
        type="button"
        class="link update"
        @click="page = 'settings'"
      >
        {{ footerUpdate }}
      </button>
      <button type="button" class="link" @click="quit">Quit</button>
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
.note {
  margin: 0;
  font-size: 12px;
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
