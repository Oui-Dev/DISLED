<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  The lamp update window: guides the user through a firmware update (the
  BOOT and RESET buttons, the Windows driver) while the backend downloads,
  checks and flashes the firmware.
-->
<script setup lang="ts">
import { getCurrentWebviewWindow } from '@tauri-apps/api/webviewWindow';
import { computed, ref, watch } from 'vue';
import type { Flash } from '@/bindings/Flash';
import { device } from '../lib/device';
import {
  cancelFirmware,
  checkFirmware,
  firmware,
  firmwareUpdateFor,
  installFirmware,
  openLink,
} from '../lib/firmware';

const STEPS = [
  'Download the firmware',
  'Put the lamp in update mode',
  'Install the firmware',
  'Restart the lamp',
] as const;

const STAGES = {
  erasing: 'Erasing the old firmware…',
  writing: 'Writing the new firmware…',
  verifying: 'Checking what was written…',
  restarting: 'Restarting the lamp…',
} as const;

/** Index in STEPS of the step an update is at; STEPS.length once done. */
function stepOf(flash: Flash): number | null {
  switch (flash.step) {
    case 'downloading':
      return 0;
    case 'waiting_for_bootloader':
    case 'driver_missing':
    case 'no_permission':
    case 'several_bootloaders':
    case 'other_chip':
      return 1;
    case 'flashing':
      return 2;
    case 'waiting_for_lamp':
      return 3;
    case 'done':
      return STEPS.length;
    default:
      return null;
  }
}

const flash = computed(() => firmware.flash);
const step = computed(() => stepOf(flash.value));
const failed = computed(() => flash.value.step === 'failed');
const idle = computed(() => flash.value.step === 'idle');

/** The step a failed update stopped at. */
const reached = ref(-1);
watch(
  () => firmware.flash.step,
  () => {
    const current = stepOf(firmware.flash);
    if (current !== null) {
      reached.value = current;
    } else if (idle.value) {
      reached.value = -1;
    }
  },
  { immediate: true },
);
const shown = computed(() => step.value ?? reached.value);

const lampVersion = computed(() => device.info?.firmware_version ?? null);
const newer = computed(() => firmwareUpdateFor(device.info));
const summary = computed(() => {
  if (!firmware.latest) {
    return firmware.error ? `No firmware found: ${firmware.error}` : 'No firmware to install.';
  }
  if (!device.connection.connected) {
    return `No lamp connected. A lamp that does not start can still get firmware ${firmware.latest} through its update mode.`;
  }
  if (!lampVersion.value) {
    return 'Connecting to the lamp…';
  }
  if (newer.value) {
    return `Firmware ${lampVersion.value} → ${newer.value}`;
  }
  return flash.value.step === 'done'
    ? `The lamp runs firmware ${lampVersion.value}.`
    : `The lamp already runs firmware ${lampVersion.value}.`;
});

const waiting = computed(() => step.value === 1);

function close(): Promise<void> {
  return getCurrentWebviewWindow().close();
}
</script>

<template>
  <main class="update">
    <header>
      <h1>Lamp update</h1>
      <p class="muted">{{ summary }}</p>
    </header>

    <p v-if="idle && firmware.latest" class="intro">
      It takes about a minute. You will open the lamp: its update mode starts with two buttons on
      its board,
      <strong>BOOT</strong>
      and <strong>RESET</strong>. Keep the lamp plugged into this computer.
    </p>

    <ol class="steps">
      <li
        v-for="(title, index) in STEPS"
        :key="title"
        :class="{
          done: index < shown,
          active: index === shown && !failed,
          stopped: index === shown && failed,
        }"
      >
        <span class="mark">{{
          index < shown ? '✓' : index === shown && failed ? '✗' : index + 1
        }}</span>
        <div class="body">
          <strong>{{ title }}</strong>

          <template v-if="index === shown && !failed">
            <p v-if="flash.step === 'downloading'" class="muted">
              Downloading firmware {{ firmware.latest }} and checking its signature…
            </p>

            <template v-else-if="flash.step === 'waiting_for_bootloader' && flash.by_command">
              <p class="muted">The lamp restarts in update mode…</p>
            </template>
            <template v-else-if="flash.step === 'waiting_for_bootloader'">
              <p>
                Open the lamp. Hold <strong>BOOT</strong>, press and release <strong>RESET</strong>,
                then release <strong>BOOT</strong>.
              </p>
              <p class="muted waiting">Waiting for the lamp in update mode…</p>
            </template>

            <template v-else-if="flash.step === 'driver_missing'">
              <p>Windows needs a driver for the update mode, once:</p>
              <ol class="howto">
                <li>
                  Download
                  <button type="button" class="link" @click="openLink('zadig')">Zadig</button>
                  and run it.
                </li>
                <li>In <em>Options</em>, choose <em>List All Devices</em>.</li>
                <li>
                  Select the device whose USB ID is <code>4348 55E0</code> (or
                  <code>1A86 55E0</code>).
                </li>
                <li>
                  Choose <em>WinUSB</em>, then <em>Install Driver</em> (or <em>Replace Driver</em>).
                </li>
              </ol>
              <p class="muted waiting">
                The update goes on by itself once the driver is installed.
              </p>
            </template>

            <p v-else-if="flash.step === 'no_permission'">
              This computer does not let the app open the lamp in update mode. Reinstall the DISLED
              package (it adds the access rule), then start the update mode again.
            </p>

            <p v-else-if="flash.step === 'several_bootloaders'">
              Several devices are in update mode. Unplug them all but the lamp.
            </p>

            <p v-else-if="flash.step === 'other_chip'">
              A device that is not a DISLED lamp is in update mode ({{ flash.chip }}). Unplug it.
            </p>

            <template v-else-if="flash.step === 'flashing'">
              <p class="muted">{{ STAGES[flash.stage] }}</p>
              <p class="muted">Do not unplug the lamp.</p>
            </template>

            <p v-else-if="flash.step === 'waiting_for_lamp'" class="muted waiting">
              The lamp starts its new firmware: it takes a few seconds.
            </p>
          </template>
        </div>
      </li>
    </ol>

    <p v-if="flash.step === 'done'" class="result">
      Done: the lamp runs firmware {{ flash.version }}. You can put it back together.
    </p>

    <div v-if="flash.step === 'failed'" class="result failure">
      <p>The update stopped: {{ flash.error }}</p>
      <p class="muted">
        The lamp is not damaged: its update mode always works. Try again, or follow the
        <button type="button" class="link" @click="openLink('flashing_guide')">
          flashing guide
        </button
        >.
      </p>
    </div>

    <footer class="actions">
      <template v-if="idle">
        <button
          v-if="!firmware.latest"
          type="button"
          :disabled="firmware.checking"
          @click="checkFirmware"
        >
          {{ firmware.checking ? 'Checking…' : 'Check for updates' }}
        </button>
        <button v-else type="button" class="primary" @click="installFirmware">
          {{ device.connection.connected && !newer ? 'Install it again' : 'Start the update' }}
        </button>
      </template>
      <button v-if="waiting" type="button" @click="cancelFirmware">Cancel</button>
      <button v-if="failed" type="button" class="primary" @click="installFirmware">
        Try again
      </button>
      <button v-if="flash.step === 'done' || failed" type="button" @click="close">Close</button>
    </footer>
  </main>
</template>

<style scoped>
.update {
  box-sizing: border-box;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  gap: 16px;
  padding: 20px 24px;
  background: var(--panel);
}
h1 {
  font-size: 18px;
  margin: 0 0 4px;
}
header p,
.intro {
  margin: 0;
}
.steps {
  list-style: none;
  padding: 0;
  margin: 0;
  display: flex;
  flex-direction: column;
  gap: 12px;
}
.steps > li {
  display: flex;
  gap: 12px;
  color: var(--muted);
}
.steps > li.active,
.steps > li.stopped,
.steps > li.done {
  color: var(--text);
}
.mark {
  flex: none;
  width: 24px;
  height: 24px;
  border-radius: 50%;
  border: 1px solid var(--border);
  display: grid;
  place-items: center;
  font-size: 12px;
  font-weight: 700;
}
.active .mark {
  background: var(--accent);
  border-color: var(--accent);
  color: var(--on-accent);
}
.done .mark {
  color: #16a34a;
  border-color: #16a34a;
}
.stopped .mark {
  color: var(--warning);
  border-color: var(--warning);
}
.body {
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding-top: 3px;
}
.body p {
  margin: 0;
}
.howto {
  margin: 0;
  padding-left: 20px;
  display: flex;
  flex-direction: column;
  gap: 4px;
}
.waiting::before {
  content: '';
  display: inline-block;
  width: 8px;
  height: 8px;
  margin-right: 8px;
  border-radius: 50%;
  background: var(--accent);
  animation: pulse 1.2s ease-in-out infinite;
}
@keyframes pulse {
  50% {
    opacity: 0.2;
  }
}
.result {
  margin: 0;
}
.failure {
  display: flex;
  flex-direction: column;
  gap: 6px;
}
.failure p {
  margin: 0;
}
.failure p:first-child {
  color: var(--warning);
  overflow-wrap: anywhere;
}
.actions {
  margin-top: auto;
  display: flex;
  justify-content: flex-end;
  gap: 8px;
}
.primary {
  background: var(--accent);
  color: var(--on-accent);
  border-color: var(--accent);
}
.link {
  border: none;
  background: none;
  padding: 0;
  color: var(--accent);
  text-decoration: underline;
}
code {
  font-size: 12px;
  white-space: nowrap;
}
</style>
