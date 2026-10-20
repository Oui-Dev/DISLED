/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Power saving for USB suspend: a slow clock (internal 8 MHz oscillator
 * divided by 8, crystal and PLL off). The core never sleeps (WFI): in the
 * main loop it never woke up again on this board (ADR 0011). The loop keeps
 * running, slowly, and keeps feeding the watchdog.
 */
#ifndef DISLED_BSP_POWER_H
#define DISLED_BSP_POWER_H

#include <stdbool.h>

/* Switches the core to 1 MHz (internal oscillator / 8) and stops the
 * crystal and the PLL. USB, which needs the PLL, must be suspended. */
void bsp_power_slow_clock(void);

/* Back to the crystal and the PLL (144 MHz). Returns false if the crystal
 * did not start: the chip then stays on the internal oscillator (8 MHz).
 * Safe to call when already fast. The _from_isr variant expects interrupts
 * to be off. */
bool bsp_power_full_clock(void);
bool bsp_power_full_clock_from_isr(void);

bool bsp_power_clock_is_slow(void);

#endif /* DISLED_BSP_POWER_H */
