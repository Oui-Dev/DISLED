// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

import { useI18n } from 'vue-i18n';
import { describeFault, type FaultRegisters } from '@/utils/fault';

/** Words a firmware fault: a failed assertion, or the trap registers. */
export function useFaultText(): (fault: FaultRegisters) => string {
  const { t } = useI18n();
  return (fault) => {
    const summary = describeFault(fault);
    if (summary.kind === 'assertion') {
      return t('diagnostics.assertion', { line: summary.line, caller: summary.caller });
    }
    const { mcause, mepc, mtval } = summary;
    return mtval === null
      ? t('diagnostics.trap', { mcause, mepc })
      : t('diagnostics.trapValue', { mcause, mepc, mtval });
  };
}
