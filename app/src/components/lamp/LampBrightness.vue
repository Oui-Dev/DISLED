<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  Brightness, as on a phone: one slider that shows the level of the lamp,
  and an Auto switch. In automatic mode the level follows the room light
  and moving the slider corrects the curve; otherwise it sets a fixed
  level. These are settings: saved on this computer and applied to the
  lamp at every connection.
-->
<script setup lang="ts">
import { useThrottleFn } from '@vueuse/core';
import { computed, nextTick, ref } from 'vue';
import { useI18n } from 'vue-i18n';
import type { SettingsPatch } from '@/bindings/SettingsPatch';
import { useLampStore } from '@/stores/lamp';
import { useSettingsStore } from '@/stores/settings';
import { AUTO_BIAS_MAX, AUTO_BIAS_MIN, correctionFor } from '@/utils/lamp';

/** How often a moving slider updates the lamp. */
const MOVE_UPDATE_MS = 120;
/** How long the slider keeps the chosen level after a move, while the lamp follows. */
const SETTLE_MS = 600;

const { t, n } = useI18n();
const lamp = useLampStore();
const settingsStore = useSettingsStore();

const settings = computed(() => settingsStore.settings);
const auto = computed(() => settings.value?.brightness_mode !== 'manual');
const bias = computed(() => settings.value?.auto_bias ?? 0);
/** Unknown until INFO arrives: assume support rather than flash a warning. */
const biasSupported = computed(() => lamp.info?.auto_bias ?? true);

/** The level of the lamp, percent: in automatic mode, the one in use now. */
const lampLevel = computed(() =>
  auto.value ? (lamp.state?.applied_level ?? null) : (settings.value?.manual_level ?? null),
);

/** The level the user chose, kept while the slider moves and until the lamp follows. */
const held = ref<number | null>(null);
/** Where a move in automatic mode started from: the corrections derive from it. */
let origin: { applied: number; bias: number } | null = null;
let settle: number | undefined;

const shown = computed(() => held.value ?? lampLevel.value);
/** Unknown while no lamp answers in automatic mode: nothing is shown then. */
const levelText = computed(() =>
  shown.value === null ? undefined : n(shown.value / 100, 'percent'),
);

/** Automatic mode needs the level in use, and a lamp that takes a correction. */
const disabled = computed(
  () => !settings.value || (auto.value && (lamp.state === null || !biasSupported.value)),
);

function patchFor(level: number): SettingsPatch {
  if (!auto.value) {
    return { brightness_mode: 'manual', manual_level: level };
  }
  origin ??= { applied: lamp.state?.applied_level ?? level, bias: bias.value };
  return { auto_bias: correctionFor(level, origin.applied, origin.bias) };
}

const saveWhileMoving = useThrottleFn(
  (patch: SettingsPatch) => settingsStore.update(patch),
  MOVE_UPDATE_MS,
  true,
);

function move(value: number | number[] | undefined): void {
  if (typeof value !== 'number') {
    return;
  }
  window.clearTimeout(settle);
  held.value = value;
  void saveWhileMoving(patchFor(value));
}

async function release(): Promise<void> {
  // A key commits before it moves the slider: wait for the move.
  await nextTick();
  if (held.value === null) {
    return;
  }
  void settingsStore.update(patchFor(held.value));
  // Then show the lamp again: the level it reached, within the correction limits.
  window.clearTimeout(settle);
  settle = window.setTimeout(() => {
    held.value = null;
    origin = null;
  }, SETTLE_MS);
}

function setAuto(on: boolean): void {
  if (on) {
    void settingsStore.update({ brightness_mode: 'auto' });
  } else {
    // Keep the brightness the eye sees: the fixed level starts at the level in use.
    const level = Math.max(1, lamp.state?.applied_level ?? settings.value?.manual_level ?? 50);
    void settingsStore.update({ brightness_mode: 'manual', manual_level: level });
  }
}

const hint = computed(() => {
  if (!auto.value) {
    return t('lamp.brightness.fixed');
  }
  if (!biasSupported.value) {
    return t('lamp.brightness.needsFirmware');
  }
  let correction = t('lamp.brightness.noCorrection');
  if (bias.value !== 0) {
    const value = n(bias.value / 100, 'signedPercent');
    const atLimit = bias.value === AUTO_BIAS_MIN || bias.value === AUTO_BIAS_MAX;
    correction = t(atLimit ? 'lamp.brightness.correctionLimit' : 'lamp.brightness.correction', {
      value,
    });
  }
  return `${t('lamp.brightness.follows')} · ${correction}`;
});
</script>

<template>
  <fieldset class="flex min-w-0 flex-col gap-3">
    <legend class="sr-only">{{ t('lamp.brightness.title') }}</legend>
    <div class="flex items-center justify-between gap-3">
      <span aria-hidden="true" class="text-sm font-semibold text-highlighted">
        {{ t('lamp.brightness.title') }}
      </span>
      <div class="flex items-center gap-3">
        <span v-if="levelText" aria-hidden="true" class="text-sm text-muted tabular-nums">
          {{ levelText }}
        </span>
        <USwitch
          :model-value="auto"
          :label="t('lamp.brightness.auto')"
          :disabled="!settings"
          size="sm"
          @update:model-value="setAuto"
        />
      </div>
    </div>

    <div class="flex items-center gap-2.5">
      <UIcon name="i-lucide-sun-dim" class="size-4 shrink-0 text-muted" />
      <USlider
        :model-value="shown ?? 1"
        :min="1"
        :max="100"
        :disabled="disabled"
        :aria-label="t('lamp.brightness.title')"
        :aria-valuetext="levelText"
        class="flex-1"
        @update:model-value="move"
        @change="release"
      />
      <UIcon name="i-lucide-sun" class="size-5 shrink-0 text-muted" />
    </div>

    <p class="text-xs text-muted">{{ hint }}</p>
  </fieldset>
</template>
