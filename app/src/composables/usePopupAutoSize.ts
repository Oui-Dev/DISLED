// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

import { useResizeObserver } from '@vueuse/core';
import type { Ref } from 'vue';
import { popupResize } from '@/ipc/commands';

/**
 * Asks for a popup window as tall as its parts stacked, whatever the page,
 * the language or the message on show; the backend keeps it within its
 * minimum and the screen, against the tray icon. The page scrolls only when
 * the screen is too short, or a scrollbar would flash while the window
 * catches up with taller content. `fit` measures again at once, for a page
 * changed while hidden.
 */
export function usePopupAutoSize(...parts: Readonly<Ref<HTMLElement | null>>[]): {
  fit: () => Promise<void>;
} {
  const page = document.documentElement;
  page.style.overflowY = 'hidden';
  let sent = 0;

  async function fit(): Promise<void> {
    const height = Math.ceil(
      parts.reduce((sum, part) => sum + (part.value?.getBoundingClientRect().height ?? 0), 0),
    );
    if (height <= 0 || height === sent) {
      return;
    }
    sent = height;
    const got = await popupResize(height);
    // A later request has the last word.
    if (height === sent) {
      page.style.overflowY = got < height ? 'auto' : 'hidden';
    }
  }

  for (const part of parts) {
    useResizeObserver(part, () => void fit());
  }
  return { fit };
}
