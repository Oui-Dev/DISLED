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
