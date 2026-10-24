// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

/** Whether `latest` ("X.Y.Z") is newer than the version a lamp reports. */
export function isNewer(latest: string, current: readonly number[]): boolean {
  const next = latest.split('.').map(Number);
  for (let i = 0; i < 3; i++) {
    if (next[i] !== current[i]) {
      return next[i] > current[i];
    }
  }
  return false;
}
