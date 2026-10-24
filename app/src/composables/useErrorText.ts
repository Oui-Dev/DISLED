// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

import type { AppError } from '@/stores/errors';

/** Words an error for the user. */
export function useErrorText(): (error: AppError) => string {
  return (error) => {
    switch (error.kind) {
      case 'backend':
        return error.detail;
      case 'rejected':
        return `the lamp rejected command 0x${error.command.toString(16)}: ${error.error}`;
      case 'unreadable':
        return `unreadable message: ${error.detail}`;
    }
  };
}
