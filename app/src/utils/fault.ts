// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

// Firmware faults as the lamp reports them: RISC-V trap registers, or a
// failed assertion that the firmware encodes in mcause.

/** A fault, from INFO (mcause, mepc) or from DIAGNOSTICS (with mtval). */
export interface FaultRegisters {
  mcause: number;
  mepc: number;
  mtval?: number;
}

export type FaultSummary =
  | { kind: 'assertion'; line: number; caller: string }
  | { kind: 'trap'; mcause: string; mepc: string; mtval: string | null };

/** mcause 0x0A55nnnn: the firmware's assertions, nnnn being the source line. */
const ASSERTION = 0x0a55;

/** A 32-bit register value, as the RISC-V tools print it. */
export function hex32(value: number): string {
  return `0x${value.toString(16).padStart(8, '0')}`;
}

export function describeFault({ mcause, mepc, mtval }: FaultRegisters): FaultSummary {
  if (mcause >>> 16 === ASSERTION) {
    return { kind: 'assertion', line: mcause & 0xffff, caller: hex32(mepc) };
  }
  return {
    kind: 'trap',
    mcause: hex32(mcause),
    mepc: hex32(mepc),
    mtval: mtval === undefined ? null : hex32(mtval),
  };
}
