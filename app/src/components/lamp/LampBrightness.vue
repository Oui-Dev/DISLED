<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  Brightness: automatic (with a "darker / brighter" correction of the
  curve) or a fixed level. These are settings: saved on this computer and
  applied to the lamp at every connection.
-->
<script setup lang="ts">
import { useThrottleFn } from '@vueuse/core';
import { computed, ref, watch } from 'vue';
import { useI18n } from 'vue-i18n';
import type { SettingsPatch } from '@/bindings/SettingsPatch';
import { useLampStore } from '@/stores/lamp';
import { useSettingsStore } from '@/stores/settings';

/** How often a dragged slider updates the lamp. */
const DRAG_UPDATE_MS = 120;

const { t, n } = useI18n();
const lamp = useLampStore();
const settingsStore = useSettingsStore();

const settings = computed(() => settingsStore.settings);
const auto = computed(() => settings.value?.brightness_mode !== 'manual');
/** Unknown until INFO arrives: assume support rather than flash a warning. */
const biasSupported = computed(() => lamp.info?.auto_bias ?? true);

/** The slider values, kept here while dragging so that the thumb does not jump back. */
const level = ref(50);
const bias = ref(0);
let dragging = false;
watch(
  settings,
  (s) => {
    if (s && !dragging) {
      level.value = s.manual_level;
      bias.value = s.auto_bias;
    }
  },
  { immediate: true },
);

const save = useThrottleFn(
  (patch: SettingsPatch) => settingsStore.update(patch),
  DRAG_UPDATE_MS,
  true,
);

function onLevel(value: number | undefined): void {
  if (value !== undefined) {
    dragging = true;
    level.value = value;
    void save({ brightness_mode: 'manual', manual_level: value });
  }
}

function onBias(value: number | undefined): void {
  if (value !== undefined) {
    dragging = true;
    bias.value = value;
    void save({ auto_bias: value });
  }
}

function setAuto(on: boolean): void {
  if (on) {
    void settingsStore.update({ brightness_mode: 'auto' });
  } else {
    // Keep the brightness the eye sees: manual starts at the applied level.
    const applied = lamp.state?.applied_level ?? level.value;
    void settingsStore.update({ brightness_mode: 'manual', manual_level: Math.max(1, applied) });
  }
}

const levelText = computed(() => n(level.value / 100, 'percent'));
const biasText = computed(() =>
  bias.value === 0 ? t('lamp.brightness.noCorrection') : n(bias.value / 100, 'signedPercent'),
);
const hint = computed(() => {
  if (!auto.value) {
    return t('lamp.brightness.fixed');
  }
  if (!biasSupported.value) {
    return t('lamp.brightness.needsFirmware');
  }
  const applied = lamp.state?.applied_level;
  const follows =
    applied === undefined
      ? t('lamp.brightness.follows')
      : t('lamp.brightness.followsNow', { level: n(applied / 100, 'percent') });
  const correction =
    bias.value === 0
      ? t('lamp.brightness.noCorrection')
      : t('lamp.brightness.correction', { value: biasText.value });
  return `${follows} · ${correction}`;
});
</script>

<template>
  <div class="flex flex-col gap-2">
    <div class="flex items-center justify-between gap-3">
      <span class="text-sm font-semibold text-highlighted">{{ t('lamp.brightness.title') }}</span>
      <USwitch
        :model-value="auto"
        :label="t('lamp.brightness.automatic')"
        :disabled="!settings"
        @update:model-value="setAuto"
      />
    </div>

    <div v-if="auto" class="flex items-center gap-2.5">
      <span class="text-xs text-muted">{{ t('lamp.brightness.darker') }}</span>
      <USlider
        :model-value="bias"
        :min="-50"
        :max="50"
        :step="5"
        :disabled="!settings || !biasSupported"
        :tooltip="{ text: biasText }"
        :aria-label="t('lamp.brightness.correctionLabel')"
        :aria-valuetext="biasText"
        class="flex-1"
        @update:model-value="onBias"
        @change="dragging = false"
      />
      <span class="text-xs text-muted">{{ t('lamp.brightness.brighter') }}</span>
    </div>

    <div v-else class="flex items-center gap-2.5">
      <USlider
        :model-value="level"
        :min="1"
        :max="100"
        :disabled="!settings"
        :aria-label="t('lamp.brightness.levelLabel')"
        :aria-valuetext="levelText"
        class="flex-1"
        @update:model-value="onLevel"
        @change="dragging = false"
      />
      <span class="w-12 text-end text-sm tabular-nums">{{ levelText }}</span>
    </div>

    <p class="text-xs text-muted">{{ hint }}</p>
  </div>
</template>
