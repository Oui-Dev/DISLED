/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Measuring range of the VEML7700 light sensor, picked from its last reading
 * as Vishay application note 84323 describes. Six steps, from the least to
 * the most sensitive: gain 1/8, 1/4, 1 and 2 at 100 ms, then gain 2 at 200
 * and 400 ms, so one count is 0.54 lx at the first step and 0.0084 lx at the
 * last. The 100 ms steps give ten readings a second; the longer ones serve
 * only below about 3 lx, and gains 1 and 2 only below 100 lx, where they are
 * linear. An 800 ms step would only add counts below the floor of the
 * brightness curve and slow the reaction to a light switched on in the dark.
 * The first step reaches 35 klx, beyond full brightness, so shorter
 * integrations and the nonlinearity correction above 1000 lx bring nothing.
 */
#ifndef DISLED_ALS_RANGE_H
#define DISLED_ALS_RANGE_H

#include <stdint.h>

#define ALS_RANGE_STEPS 6u

/* The least sensitive step never saturates indoors, so the sensor starts
 * there when the light is unknown. */
#define ALS_RANGE_FIRST 0u

typedef struct {
    uint8_t gain_eighths;    /* 1 (x1/8), 2 (x1/4), 8 (x1) or 16 (x2) */
    uint16_t integration_ms; /* 100, 200 or 400 */
} als_range_setting_t;

/* Gain and integration time of a step (0 to ALS_RANGE_STEPS - 1). */
als_range_setting_t als_range_setting(uint8_t step);

/* Light of a count measured at a step, in lux x 1000. */
uint32_t als_range_lux_milli(uint8_t step, uint16_t count);

/* Step for the next readings. The step stays while its count is above 50
 * and at most its upper limit: 100 lx for gains 1 and 2, and for the longer
 * integrations the point where half the time would give 200 counts.
 * Otherwise the light just measured picks the least sensitive step that
 * gives more than 100 counts. A dark reading (0 counts) goes to the most
 * sensitive step, a saturated one to a less sensitive step that is not
 * saturated. */
uint8_t als_range_next(uint8_t step, uint16_t count);

#endif /* DISLED_ALS_RANGE_H */
