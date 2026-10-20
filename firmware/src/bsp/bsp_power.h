/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Power saving: idle until the next interrupt, and a slow clock for USB
 * suspend (HSI 8 MHz, crystal and PLL off). The watchdog keeps running in
 * both, so the main loop keeps feeding it.
 */
#ifndef DISLED_BSP_POWER_H
#define DISLED_BSP_POWER_H

#include <stdbool.h>

/* Sleeps until the next interrupt (1 ms tick at the latest). */
void bsp_power_idle(void);

/* Switches the core to the internal 8 MHz oscillator and stops the crystal
 * and the PLL. USB, which needs the PLL, must be suspended. */
void bsp_power_slow_clock(void);

/* Back to the crystal and the PLL (144 MHz). Returns false if the crystal
 * did not start: the chip then stays on the slow clock. Safe to call when
 * already fast. The _from_isr variant expects interrupts to be off. */
bool bsp_power_full_clock(void);
bool bsp_power_full_clock_from_isr(void);

bool bsp_power_clock_is_slow(void);

#endif /* DISLED_BSP_POWER_H */
