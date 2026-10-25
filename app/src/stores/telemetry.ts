// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

// Live telemetry of the Diagnostics window: the stream period, the last
// samples for the charts, and CSV recordings.

import { save } from '@tauri-apps/plugin-dialog';
import { defineStore } from 'pinia';
import { computed, ref, shallowRef, watch } from 'vue';
import type { Telemetry } from '@/bindings/Telemetry';
import { saveCsv } from '@/ipc/commands';
import { on } from '@/ipc/events';
import { csvFile, telemetryRow } from '@/utils/csv';
import { useErrorStore } from './errors';
import { useLampStore } from './lamp';

/** Samples kept for the charts: 30 s at 50 ms, 10 min at 1 s. */
const HISTORY_LENGTH = 600;
/** The stream period when the window opens, ms: the charts hold the last 2.5 min. */
const DEFAULT_PERIOD_MS = 250;

export const useTelemetryStore = defineStore('telemetry', () => {
  const errors = useErrorStore();
  const lamp = useLampStore();

  /** 0: no stream. */
  const periodMs = ref(DEFAULT_PERIOD_MS);
  const latest = ref<Telemetry | null>(null);
  /**
   * The last samples, oldest first. A new array at each sample, outside
   * deep reactivity: it changes up to 20 times per second.
   */
  const history = shallowRef<readonly Telemetry[]>([]);
  const recording = ref(false);
  const recordedRows = ref(0);
  let rows: string[] = [];

  /** The connected lamp streams telemetry. */
  const supported = computed(() => lamp.connection.connected && (lamp.info?.telemetry ?? false));
  /** Set by start(): only the Diagnostics window streams. */
  const following = ref(false);

  /** The period the user picked. */
  async function setPeriod(ms: number): Promise<void> {
    periodMs.value = ms;
    await lamp.send({ kind: 'set_telemetry', period_ms: ms });
  }

  function receive(sample: Telemetry): void {
    latest.value = sample;
    history.value = [...history.value.slice(1 - HISTORY_LENGTH), sample];
    if (recording.value) {
      rows.push(telemetryRow(sample, lamp.state));
      recordedRows.value = rows.length;
    }
  }

  // The stream starts with the window, and again for a lamp that comes
  // back: a lamp streams nothing until asked. The backend stops it when the
  // window closes. A lamp that leaves takes its charts along: the next one
  // counts its uptime from zero.
  watch(
    () => following.value && supported.value,
    (streaming) => {
      if (!streaming) {
        latest.value = null;
        history.value = [];
      } else if (periodMs.value > 0) {
        void lamp.send({ kind: 'set_telemetry', period_ms: periodMs.value }, { automatic: true });
      }
    },
  );

  function startRecording(): void {
    rows = [];
    recordedRows.value = 0;
    recording.value = true;
  }

  function stopRecording(): void {
    recording.value = false;
  }

  /** Asks where to save the recording, with a dialog of this title. */
  async function exportRecording(dialogTitle: string): Promise<void> {
    const stamp = new Date().toISOString().slice(0, 19).replace(/[:T]/g, '-');
    const path = await save({
      title: dialogTitle,
      defaultPath: `disled-${stamp}.csv`,
      filters: [{ name: 'CSV', extensions: ['csv'] }],
    });
    if (path === null) {
      return;
    }
    try {
      await saveCsv(path, csvFile(rows));
    } catch (error) {
      errors.reportFailure(error);
    }
  }

  /** Follows the backend events and the lamp. Call once, in the Diagnostics window. */
  async function start(): Promise<void> {
    await on('device-message', ({ message }) => {
      if (message.kind === 'telemetry') {
        const { kind: _kind, ...sample } = message;
        receive(sample);
      }
    });
    // Once the samples have a listener.
    following.value = true;
  }

  return {
    periodMs,
    latest,
    history,
    recording,
    recordedRows,
    supported,
    setPeriod,
    startRecording,
    stopRecording,
    exportRecording,
    start,
  };
});
