/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Millisecond time base (SysTick).
 */
#ifndef DISLED_BSP_TIME_H
#define DISLED_BSP_TIME_H

#include <stdint.h>

void bsp_time_init(void);

/* Milliseconds since boot. Wraps after about 49.7 days: compare times with
 * time_elapsed_ms() from time_utils.h, never with < or >. */
uint32_t bsp_time_ms(void);

/* Busy-waits at least the given number of microseconds (up to 1000). */
void bsp_time_delay_us(uint32_t us);

#endif /* DISLED_BSP_TIME_H */
