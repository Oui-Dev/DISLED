<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  One page for every window: the window label picks the view.
-->
<script setup lang="ts">
import { onMounted } from "vue";
import { getCurrentWebviewWindow } from "@tauri-apps/api/webviewWindow";
import DiagnosticsView from "./components/DiagnosticsView.vue";
import LampUpdateView from "./components/LampUpdateView.vue";
import PopupView from "./components/PopupView.vue";
import { startDevice } from "./lib/device";
import { startFirmware } from "./lib/firmware";
import { startUpdates } from "./lib/updates";

const label = getCurrentWebviewWindow().label;

onMounted(() => {
  void startDevice();
  if (label === "popup" || label === "lamp-update") {
    void startFirmware();
  }
  if (label === "popup") {
    void startUpdates();
  }
});
</script>

<template>
  <PopupView v-if="label === 'popup'" />
  <LampUpdateView v-else-if="label === 'lamp-update'" />
  <DiagnosticsView v-else />
</template>
