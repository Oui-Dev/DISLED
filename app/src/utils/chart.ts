// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

import type { Telemetry } from '@/bindings/Telemetry';

/** A line of a telemetry chart. */
export interface ChartSeries {
  key: string;
  label: string;
  /** A theme color or any CSS color. */
  color: string;
  value: (sample: Telemetry) => number;
}

export type ChartRow = Record<string, number | string>;

/** The lamp's uptime as h:mm:ss.s, the x value of the charts. */
function formatUptime(ms: number): string {
  const tenths = Math.floor(ms / 100);
  const seconds = Math.floor(tenths / 10) % 60;
  const minutes = Math.floor(tenths / 600) % 60;
  const hours = Math.floor(tenths / 36000);
  const pad = (value: number) => String(value).padStart(2, '0');
  return `${hours}:${pad(minutes)}:${pad(seconds)}.${tenths % 10}`;
}

/** One row per sample: its uptime, then the value of each series. */
export function chartRows(
  samples: readonly Telemetry[],
  series: readonly ChartSeries[],
): ChartRow[] {
  return samples.map((sample) => {
    const row: ChartRow = { uptime: formatUptime(sample.uptime_ms) };
    for (const { key, value } of series) {
      row[key] = value(sample);
    }
    return row;
  });
}
