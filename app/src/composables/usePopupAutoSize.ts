// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

import { useResizeObserver } from '@vueuse/core';
import type { Ref } from 'vue';
import { popupResize } from '@/ipc/commands';

/**
 * Asks for a popup window as tall as its parts stacked (their natural
 * heights, margins aside), whatever the page, the language or the message
 * on show. The backend keeps the window within its minimum and the screen,
 * against the tray icon.
 */
export function usePopupAutoSize(...parts: Readonly<Ref<HTMLElement | null>>[]): void {
  let sent = 0;
  const fit = () => {
    const height = Math.ceil(
      parts.reduce((sum, part) => sum + (part.value?.getBoundingClientRect().height ?? 0), 0),
    );
    if (height > 0 && height !== sent) {
      sent = height;
      void popupResize(height);
    }
  };
  for (const part of parts) {
    useResizeObserver(part, fit);
  }
}
