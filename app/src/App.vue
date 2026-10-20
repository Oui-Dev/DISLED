<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  One page for both windows: the window label picks the view.
-->
<script setup lang="ts">
import { onMounted } from "vue";
import { getCurrentWebviewWindow } from "@tauri-apps/api/webviewWindow";
import DiagnosticsView from "./components/DiagnosticsView.vue";
import PopupView from "./components/PopupView.vue";
import { startDevice } from "./lib/device";
import { startUpdates } from "./lib/updates";

const isPopup = getCurrentWebviewWindow().label === "popup";

onMounted(() => {
  void startDevice();
  if (isPopup) {
    void startUpdates();
  }
});
</script>

<template>
  <PopupView v-if="isPopup" />
  <DiagnosticsView v-else />
</template>
