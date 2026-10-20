/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Fault capture. A CPU exception or a failed assertion saves its cause and
 * address in the backup registers (they survive a system reset, not a
 * power cycle) and resets the chip; the LED turns off during the reset
 * thanks to the gate pull-downs. The next boot reports the fault to the
 * host, which makes field failures diagnosable without a debugger.
 *
 * The backup registers also count boots, watchdog resets and faults since
 * power-up, for the DIAGNOSTICS message.
 */
#ifndef DISLED_BSP_FAULT_H
#define DISLED_BSP_FAULT_H

#include <stdbool.h>
#include <stdint.h>

/* mcause of a failed DISLED_ASSERT: 0x0A55nnnn, nnnn = source line. */
#define BSP_FAULT_ASSERT_CAUSE 0x0A550000u

typedef struct {
    bool valid;
    uint32_t mcause; /* exception cause (RISC-V mcause) */
    uint32_t mepc;   /* address of the faulting instruction */
    uint32_t mtval;  /* faulting address or instruction */
} fault_record_t;

typedef struct {
    uint16_t boots;          /* since power-up, this one included */
    uint8_t watchdog_resets; /* since power-up, saturates at 255 */
    uint8_t faults;          /* since power-up, saturates at 255 */
    fault_record_t last;     /* last fault since power-up (valid if faults > 0) */
} fault_stats_t;

/* Reads the record left by a fault just before this boot (valid only then)
 * and updates the counters. Call once at boot, after bsp_init(). */
fault_record_t bsp_fault_take(void);

fault_stats_t bsp_fault_stats(void);

#endif /* DISLED_BSP_FAULT_H */
