// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

// Lamp diagnostics (protocol 1.1 GET_DIAGNOSTICS), asked every 2 s while
// the Diagnostics window is open, and the production self-test (SELF_TEST).

import { useIntervalFn } from '@vueuse/core';
import { defineStore } from 'pinia';
import { computed, ref, watch } from 'vue';
import type { Diagnostics } from '@/bindings/Diagnostics';
import type { SelfTestResult } from '@/bindings/SelfTestResult';
import { on } from '@/ipc/events';
import { useLampStore } from './lamp';

/** The code of SELF_TEST, for the ERROR that rejects it. */
const COMMAND_SELF_TEST = 0x08;
/** How often the diagnostics are asked, ms. */
const REFRESH_MS = 2000;

export const useDiagnosticsStore = defineStore('diagnostics', () => {
  const lamp = useLampStore();

  const diagnostics = ref<Diagnostics | null>(null);
  const selfTestRunning = ref(false);
  const selfTestResult = ref<SelfTestResult | null>(null);

  /** The connected lamp reports diagnostics. */
  const supported = computed(() => lamp.connection.connected && (lamp.info?.diagnostics ?? false));
  /** Set by start(): only the Diagnostics window asks the lamp. */
  const following = ref(false);

  function request(): Promise<void> {
    return lamp.send({ kind: 'get_diagnostics' }, { automatic: true });
  }

  function startSelfTest(): Promise<void> {
    selfTestRunning.value = true;
    selfTestResult.value = null;
    return lamp.send({ kind: 'self_test' });
  }

  // At once, then every REFRESH_MS: the Health tab and its badge stay
  // current, whatever the tab on show.
  const polling = useIntervalFn(() => void request(), REFRESH_MS, { immediate: false });
  watch(
    () => following.value && supported.value,
    (active) => {
      if (active) {
        void request();
        polling.resume();
      } else {
        polling.pause();
      }
    },
  );

  /** Follows the backend events and the lamp. Call once, in the Diagnostics window. */
  async function start(): Promise<void> {
    await on('device-connection', ({ connected }) => {
      if (!connected) {
        diagnostics.value = null;
        selfTestRunning.value = false;
        selfTestResult.value = null;
      }
    });
    await on('device-message', ({ message }) => {
      switch (message.kind) {
        case 'diagnostics': {
          const { kind: _kind, ...rest } = message;
          diagnostics.value = rest;
          break;
        }
        case 'self_test_result': {
          const { kind: _kind, ...rest } = message;
          selfTestRunning.value = false;
          selfTestResult.value = rest;
          break;
        }
        case 'error':
          if (message.command === COMMAND_SELF_TEST) {
            selfTestRunning.value = false;
          }
          break;
      }
    });
    // Once the replies have a listener.
    following.value = true;
  }

  return { diagnostics, selfTestRunning, selfTestResult, supported, startSelfTest, start };
});
