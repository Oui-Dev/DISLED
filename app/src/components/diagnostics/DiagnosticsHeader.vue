<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  Top of the Diagnostics window: connection, firmware and identity of the
  lamp, its restart, the last crash and the last error.
-->
<script setup lang="ts">
import { computed, ref } from 'vue';
import { useI18n } from 'vue-i18n';
import { useErrorText } from '@/composables/useErrorText';
import { useFaultText } from '@/composables/useFaultText';
import { useErrorStore } from '@/stores/errors';
import { useLampStore } from '@/stores/lamp';

const { t } = useI18n();
const lamp = useLampStore();
const errors = useErrorStore();
const errorText = useErrorText();
const faultText = useFaultText();

const connected = computed(() => lamp.connection.connected);
const crash = computed(() => (lamp.info?.fault ? faultText(lamp.info.fault) : null));

const confirmingRestart = ref(false);

function restart(): Promise<void> {
  confirmingRestart.value = false;
  return lamp.restart();
}
</script>

<template>
  <header class="flex flex-col gap-3">
    <div class="flex flex-wrap items-center justify-between gap-x-6 gap-y-2">
      <div class="flex flex-wrap items-center gap-3">
        <h1 class="text-xl font-bold tracking-wide text-highlighted">DISLED</h1>
        <UBadge
          :color="connected ? 'success' : 'neutral'"
          variant="subtle"
          :icon="connected ? 'i-lucide-usb' : 'i-lucide-unplug'"
          :label="connected ? t('diagnostics.connected') : t('status.noLamp')"
        />
        <template v-if="connected">
          <span class="text-sm text-muted">
            {{ t('diagnostics.firmware', { version: lamp.info?.firmware_version ?? '…' }) }}
          </span>
          <UBadge
            v-if="lamp.connection.debug_build"
            color="warning"
            variant="subtle"
            :label="t('diagnostics.debugBuild')"
          />
        </template>
      </div>

      <div v-if="connected" class="flex flex-wrap items-center gap-x-4 gap-y-2 text-sm text-muted">
        <span>{{ t('diagnostics.serial', { serial: lamp.connection.serial ?? '?' }) }}</span>
        <span v-if="lamp.info">
          {{
            t('diagnostics.resetCause', { cause: t(`enums.resetCause.${lamp.info.reset_cause}`) })
          }}
        </span>
        <UButton
          color="neutral"
          variant="outline"
          icon="i-lucide-rotate-ccw"
          @click="confirmingRestart = true"
        >
          {{ t('diagnostics.restart') }}
        </UButton>
      </div>
    </div>

    <UAlert
      v-if="crash"
      color="warning"
      variant="subtle"
      icon="i-lucide-triangle-alert"
      :description="t('diagnostics.crashed', { fault: crash })"
    />
    <UAlert
      v-if="errors.last"
      color="error"
      variant="subtle"
      icon="i-lucide-circle-x"
      :description="errorText(errors.last)"
      :ui="{ description: 'break-words' }"
    />

    <UModal
      v-model:open="confirmingRestart"
      :title="t('diagnostics.restartTitle')"
      :description="t('diagnostics.restartDescription')"
    >
      <template #footer>
        <div class="flex w-full justify-end gap-2">
          <UButton color="neutral" variant="outline" @click="confirmingRestart = false">
            {{ t('common.cancel') }}
          </UButton>
          <UButton icon="i-lucide-rotate-ccw" @click="restart">
            {{ t('diagnostics.restart') }}
          </UButton>
        </div>
      </template>
    </UModal>
  </header>
</template>
