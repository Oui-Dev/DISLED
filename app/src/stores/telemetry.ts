// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

// Live telemetry of the Diagnostics window: the stream period, the last
// samples for the charts, and CSV recordings.

import { save } from '@tauri-apps/plugin-dialog';
import { defineStore } from 'pinia';
import { ref, shallowRef } from 'vue';
import type { Telemetry } from '@/bindings/Telemetry';
import { saveCsv } from '@/ipc/commands';
import { on } from '@/ipc/events';
import { csvFile, telemetryRow } from '@/utils/csv';
import { useErrorStore } from './errors';
import { useLampStore } from './lamp';

/** Samples kept for the charts: 30 s at 50 ms, 10 min at 1 s. */
const HISTORY_LENGTH = 600;

export const useTelemetryStore = defineStore('telemetry', () => {
  const errors = useErrorStore();
  const lamp = useLampStore();

  /** 0: no stream. */
  const periodMs = ref(0);
  const latest = ref<Telemetry | null>(null);
  /**
   * The last samples, oldest first. A new array at each sample, outside
   * deep reactivity: it changes up to 20 times per second.
   */
  const history = shallowRef<readonly Telemetry[]>([]);
  const recording = ref(false);
  const recordedRows = ref(0);
  let rows: string[] = [];

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

  /** Follows the backend events. Call once, in the Diagnostics window. */
  async function start(): Promise<void> {
    await on('device-connection', ({ connected }) => {
      if (!connected) {
        latest.value = null;
      } else if (periodMs.value > 0) {
        // A lamp that comes back streams nothing until asked again.
        void setPeriod(periodMs.value);
      }
    });
    await on('device-message', ({ message }) => {
      if (message.kind === 'telemetry') {
        const { kind: _kind, ...sample } = message;
        receive(sample);
      }
    });
  }

  return {
    periodMs,
    latest,
    history,
    recording,
    recordedRows,
    setPeriod,
    startRecording,
    stopRecording,
    exportRecording,
    start,
  };
});
