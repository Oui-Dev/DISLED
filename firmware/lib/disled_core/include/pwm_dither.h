/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Temporal dithering of a PWM duty given in sub-ticks (1/16 of a tick).
 *
 * Called once per PWM period, it returns the whole ticks for the next
 * period: the fraction accumulates and adds one tick each time it carries,
 * so that over 16 periods the average duty is exact. At 20 kHz the pattern
 * repeats at 1.25 kHz or faster, well beyond visible flicker.
 *
 * Header-only: it runs in the PWM interrupt, 20000 times a second.
 */
#ifndef DISLED_PWM_DITHER_H
#define DISLED_PWM_DITHER_H

#include <stdint.h>

#define PWM_DITHER_SUBTICKS 16u
/* Largest period whose 100 % duty in sub-ticks fits 16 bits. */
#define PWM_DITHER_PERIOD_MAX 4095u

static inline uint16_t pwm_dither_next(uint8_t *accumulator, uint16_t subticks)
{
    const uint16_t ticks = (uint16_t)(subticks / PWM_DITHER_SUBTICKS);
    const uint8_t sum = (uint8_t)(*accumulator + (subticks % PWM_DITHER_SUBTICKS));
    if (sum >= PWM_DITHER_SUBTICKS) {
        *accumulator = (uint8_t)(sum - PWM_DITHER_SUBTICKS);
        return (uint16_t)(ticks + 1u);
    }
    *accumulator = sum;
    return ticks;
}

#endif /* DISLED_PWM_DITHER_H */
