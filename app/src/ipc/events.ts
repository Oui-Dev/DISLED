// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

// The events the backend sends to every window, and their payloads.

import { listen, type UnlistenFn } from '@tauri-apps/api/event';
import type { Connection } from '@/bindings/Connection';
import type { FirmwareStatus } from '@/bindings/FirmwareStatus';
import type { Frame } from '@/bindings/Frame';
import type { Settings } from '@/bindings/Settings';
import type { UpdateStatus } from '@/bindings/UpdateStatus';

interface Events {
  /** The lamp was found or lost (device.rs). */
  'device-connection': Connection;
  /** A message from the lamp. */
  'device-message': Frame;
  /** A report the backend could not decode. */
  'device-protocol-error': string;
  /** Any window changed the settings. */
  'settings-changed': Settings;
  'update-status': UpdateStatus;
  'firmware-status': FirmwareStatus;
}

export type EventName = keyof Events;

/** Calls `handler` with the payload of each `event`. */
export function on<E extends EventName>(
  event: E,
  handler: (payload: Events[E]) => void,
): Promise<UnlistenFn> {
  return listen<Events[E]>(event, ({ payload }) => handler(payload));
}
