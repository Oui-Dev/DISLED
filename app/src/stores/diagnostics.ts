// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

// Lamp diagnostics (protocol 1.1 GET_DIAGNOSTICS) and the production
// self-test (SELF_TEST), for the Diagnostics window.

import { defineStore } from 'pinia';
import { ref } from 'vue';
import type { Diagnostics } from '@/bindings/Diagnostics';
import type { SelfTestResult } from '@/bindings/SelfTestResult';
import { on } from '@/ipc/events';
import { useLampStore } from './lamp';

/** The code of SELF_TEST, for the ERROR that rejects it. */
const COMMAND_SELF_TEST = 0x08;

export const useDiagnosticsStore = defineStore('diagnostics', () => {
  const lamp = useLampStore();

  const diagnostics = ref<Diagnostics | null>(null);
  const selfTestRunning = ref(false);
  const selfTestResult = ref<SelfTestResult | null>(null);

  function request(): Promise<void> {
    return lamp.send({ kind: 'get_diagnostics' });
  }

  function startSelfTest(): Promise<void> {
    selfTestRunning.value = true;
    selfTestResult.value = null;
    return lamp.send({ kind: 'self_test' });
  }

  /** Follows the backend events. Call once, in the Diagnostics window. */
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
  }

  return { diagnostics, selfTestRunning, selfTestResult, request, startSelfTest, start };
});
