// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

import type { LampColor } from '@/bindings/LampColor';

/** The lamp colors, in the order of the color picker. */
export const LAMP_COLORS = [
  'green',
  'orange',
  'red',
  'warm_white',
] as const satisfies readonly LampColor[];

/**
 * How the swatches show the lamp colors, as the tray icon does
 * (src-tauri/src/tray.rs): close to the LEDs, whatever the color mode.
 */
export const SWATCH_COLORS: Record<LampColor, string> = {
  green: '#2ecc71',
  orange: '#f39c12',
  red: '#e74c3c',
  warm_white: '#ffe2b8',
};

/** Limits of the automatic brightness correction, percent (AUTO_BIAS_MIN/MAX of the protocol crate). */
export const AUTO_BIAS_MIN = -50;
export const AUTO_BIAS_MAX = 50;

/**
 * The automatic brightness correction that brings the lamp to `target`
 * percent. The lamp shows the level of its curve times (100 + bias) / 100
 * (protocol SET_AUTO_BIAS), here `applied` percent under `bias`; the
 * correction stays within its limits.
 */
export function correctionFor(target: number, applied: number, bias: number): number {
  // The curve, at least the 0.1 % floor of the lamp.
  const curve = Math.max((applied * 100) / (100 + bias), 0.1);
  const wanted = Math.round((target / curve) * 100 - 100);
  return Math.min(AUTO_BIAS_MAX, Math.max(AUTO_BIAS_MIN, wanted));
}
