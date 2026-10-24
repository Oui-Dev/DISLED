// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

import { useResizeObserver } from '@vueuse/core';
import type { Ref } from 'vue';
import { popupResize } from '@/ipc/commands';

/**
 * Makes the popup window as tall as `element`, whatever the page, the
 * language or the message on show. The backend keeps the window within the
 * screen and against the tray icon.
 */
export function usePopupAutoSize(element: Readonly<Ref<HTMLElement | null>>): void {
  let sent = 0;
  useResizeObserver(element, ([entry]) => {
    const height = Math.ceil(entry.borderBoxSize[0]?.blockSize ?? entry.contentRect.height);
    if (height > 0 && height !== sent) {
      sent = height;
      void popupResize(height);
    }
  });
}
