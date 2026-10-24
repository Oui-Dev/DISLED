<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  The popup under the tray icon: everyday control of the lamp, and a
  settings page. The window takes the height of its content; Escape hides
  it.
-->
<script setup lang="ts">
import { onKeyStroke } from '@vueuse/core';
import { computed, nextTick, ref, useTemplateRef } from 'vue';
import { useI18n } from 'vue-i18n';
import { useErrorText } from '@/composables/useErrorText';
import { usePopupAutoSize } from '@/composables/usePopupAutoSize';
import { popupHide, quit } from '@/ipc/commands';
import { useErrorStore } from '@/stores/errors';
import { useFirmwareStore } from '@/stores/firmware';
import { useLampStore } from '@/stores/lamp';
import { useUpdatesStore } from '@/stores/updates';

type Page = 'lamp' | 'settings';

const { t } = useI18n();
const lamp = useLampStore();
const updates = useUpdatesStore();
const firmware = useFirmwareStore();
const errors = useErrorStore();
const errorText = useErrorText();

usePopupAutoSize(useTemplateRef<HTMLElement>('root'));

const page = ref<Page>('lamp');
const pageHeading = useTemplateRef<HTMLElement>('pageHeading');

/** Moves the focus to the title of the new page, for keyboards and screen readers. */
async function show(next: Page): Promise<void> {
  page.value = next;
  await nextTick();
  pageHeading.value?.focus();
}

onKeyStroke('Escape', (event) => {
  // In an open list (a select), Escape closes the list first.
  const target = event.target instanceof Element ? event.target : null;
  if (!event.defaultPrevented && !target?.closest('[role="listbox"], [role="dialog"]')) {
    void popupHide();
  }
});

const status = computed(() => {
  if (!lamp.connection.connected) {
    return t('status.noLamp');
  }
  return lamp.state ? t('status.connected') : t('status.connecting');
});

/** The app first: a new firmware may need a new app. */
const footerUpdate = computed(() => {
  const available = updates.status.available;
  if (available) {
    return t('updates.toApp', { version: available });
  }
  return firmware.updateForLamp ? t('updates.toLamp') : null;
});
</script>

<template>
  <main ref="root" class="flex flex-col gap-4 p-4">
    <header>
      <h1 class="text-base font-bold tracking-wide text-highlighted">DISLED</h1>
      <p role="status" class="text-xs text-muted">{{ status }}</p>
    </header>

    <section v-if="page === 'lamp'" class="flex flex-col gap-4">
      <h2 ref="pageHeading" tabindex="-1" class="sr-only">{{ t('popup.lampPage') }}</h2>
      <p v-if="!lamp.connection.connected" class="text-xs text-muted">
        {{ t('lamp.unplugged') }}
      </p>
      <LampColorPicker labels />
      <LampBrightness />
    </section>

    <section v-else class="flex flex-col gap-3">
      <h2
        ref="pageHeading"
        tabindex="-1"
        class="text-sm font-semibold text-highlighted focus:outline-none"
      >
        {{ t('common.settings') }}
      </h2>
      <PopupSettings />
    </section>

    <UAlert
      v-if="errors.last"
      color="error"
      variant="subtle"
      icon="i-lucide-triangle-alert"
      :description="errorText(errors.last)"
      :ui="{ description: 'break-words' }"
    />

    <footer class="flex items-center justify-between gap-2">
      <UButton
        color="neutral"
        variant="link"
        class="px-0"
        :icon="page === 'lamp' ? 'i-lucide-settings' : 'i-lucide-arrow-left'"
        @click="show(page === 'lamp' ? 'settings' : 'lamp')"
      >
        {{ page === 'lamp' ? t('common.settings') : t('common.back') }}
      </UButton>
      <UButton
        v-if="footerUpdate && page === 'lamp'"
        variant="link"
        class="px-0 font-semibold"
        @click="show('settings')"
      >
        {{ footerUpdate }}
      </UButton>
      <UButton color="neutral" variant="link" class="px-0" icon="i-lucide-power" @click="quit">
        {{ t('common.quit') }}
      </UButton>
    </footer>
  </main>
</template>
