/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Single entry point to the WCH SDK device header. The vendor headers do
 * not build cleanly with our strict warnings, so they are silenced here and
 * nowhere else.
 *
 * The SDK startup sets mstatus to 0x88 before its mret, which leaves main()
 * in user mode. Code that needs machine mode, such as a write to mtvec or
 * mstatus or a jump into the ROM bootloader, has to run in a trap handler.
 */
#ifndef DISLED_BSP_CH32_H
#define DISLED_BSP_CH32_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wshadow"
#include "ch32v20x.h"
#pragma GCC diagnostic pop

/*
 * Interrupt handler attribute. The startup code enables the QingKe
 * hardware prologue/epilogue (INTSYSCR = 0x3), which saves the caller-saved
 * registers in hardware; the WCH GCC "fast" interrupt attribute relies on
 * it and only emits the final mret. All interrupts run at the same priority,
 * so they never nest and the hardware stack never overflows.
 */
#define BSP_ISR __attribute__((interrupt("WCH-Interrupt-fast")))

#endif /* DISLED_BSP_CH32_H */
