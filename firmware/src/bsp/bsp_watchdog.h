/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Independent watchdog (IWDG, clocked by the internal LSI, about 40 kHz). Once
 * started it cannot be stopped; the main loop feeds it only after every
 * task ran, so a stuck task resets the chip.
 */
#ifndef DISLED_BSP_WATCHDOG_H
#define DISLED_BSP_WATCHDOG_H

#include <stdint.h>

/* Starts the watchdog. `timeout_ms` up to 3000 (LSI tolerance: -30/+50 %). */
void bsp_watchdog_start(uint16_t timeout_ms);

void bsp_watchdog_feed(void);

#endif /* DISLED_BSP_WATCHDOG_H */
