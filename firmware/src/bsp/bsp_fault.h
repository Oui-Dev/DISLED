/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Fault capture. A CPU exception saves its cause and address in the backup
 * registers (they survive a system reset) and resets the chip; the LED
 * turns off during the reset thanks to the gate pull-downs. The next boot
 * reports the fault to the host, which makes field failures diagnosable
 * without a debugger.
 */
#ifndef DISLED_BSP_FAULT_H
#define DISLED_BSP_FAULT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool valid;
    uint32_t mcause; /* exception cause (RISC-V mcause) */
    uint32_t mepc;   /* address of the faulting instruction */
    uint32_t mtval;  /* faulting address or instruction */
} fault_record_t;

/* Reads the record left by a fault before the last reset, then clears it.
 * Call once at boot. */
fault_record_t bsp_fault_take(void);

#endif /* DISLED_BSP_FAULT_H */
