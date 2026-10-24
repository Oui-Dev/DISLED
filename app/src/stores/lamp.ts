// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

// The lamp as one window sees it: the connection, its identity (INFO) and
// its state (STATE). The lamp owns its state: the window only shows the
// last STATE message and sends commands.

import { defineStore } from 'pinia';
import { ref } from 'vue';
import type { Command } from '@/bindings/Command';
import type { Connection } from '@/bindings/Connection';
import type { Info } from '@/bindings/Info';
import type { LampState } from '@/bindings/LampState';
import type { SetState } from '@/bindings/SetState';
import { deviceConnection, deviceSend } from '@/ipc/commands';
import { on } from '@/ipc/events';
import { useErrorStore } from './errors';

export const useLampStore = defineStore('lamp', () => {
  const errors = useErrorStore();

  const connection = ref<Connection>({ connected: false, debug_build: false, serial: null });
  const info = ref<Info | null>(null);
  const state = ref<LampState | null>(null);

  async function send(command: Command): Promise<void> {
    try {
      await deviceSend(command);
      errors.clear();
    } catch (error) {
      errors.reportFailure(error);
    }
  }

  /** Power and color: the lamp's own state, not saved on this computer. */
  function setLamp(changes: Pick<SetState, 'on' | 'color'>): Promise<void> {
    return send({ kind: 'set_state', ...changes });
  }

  function restart(): Promise<void> {
    return send({ kind: 'reset' });
  }

  function receiveState(next: LampState): void {
    const previous = state.value;
    state.value = next;
    // Each message with a change increments state_seq by one; a larger
    // jump means a lost message, so ask for the full state again.
    if (
      previous
      && next.state_seq !== previous.state_seq
      && next.state_seq !== ((previous.state_seq + 1) & 0xffff)
    ) {
      void send({ kind: 'get_state' });
    }
  }

  /** Follows the backend events. Call once per window. */
  async function start(): Promise<void> {
    await on('device-connection', (next) => {
      connection.value = next;
      if (next.connected) {
        errors.clear();
      } else {
        info.value = null;
        state.value = null;
      }
    });
    // Telemetry, diagnostics and self-test results go to their own stores.
    await on('device-message', ({ message }) => {
      switch (message.kind) {
        case 'info': {
          const { kind: _kind, ...rest } = message;
          info.value = rest;
          break;
        }
        case 'state': {
          const { kind: _kind, ...rest } = message;
          receiveState(rest);
          break;
        }
        case 'error':
          errors.report({ kind: 'rejected', command: message.command, error: message.error });
          break;
      }
    });
    await on('device-protocol-error', (detail) => {
      errors.report({ kind: 'unreadable', detail });
    });
    connection.value = await deviceConnection();
    if (connection.value.connected) {
      await send({ kind: 'get_info' });
      await send({ kind: 'get_state' });
    }
  }

  return { connection, info, state, send, setLamp, restart, start };
});
