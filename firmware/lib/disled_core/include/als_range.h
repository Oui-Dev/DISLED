/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Measuring range of the VEML7700 light sensor, picked from its last reading
 * as Vishay application note 84323 describes: six steps, from gain 1/8 at
 * 100 ms, 0.54 lx a count, to gain 2 at 400 ms, 0.0084 lx.
 */
#ifndef DISLED_ALS_RANGE_H
#define DISLED_ALS_RANGE_H

#include <stdint.h>

#define ALS_RANGE_STEPS 6u

/* The least sensitive step never saturates indoors: the start while the light is unknown. */
#define ALS_RANGE_FIRST 0u

typedef struct {
    uint8_t gain_eighths;    /* 1 (x1/8), 2 (x1/4), 8 (x1) or 16 (x2) */
    uint16_t integration_ms; /* 100, 200 or 400 */
} als_range_setting_t;

/* `step` goes from 0 to ALS_RANGE_STEPS - 1. */
als_range_setting_t als_range_setting(uint8_t step);

/* Light of a count measured at a step, in lux x 1000. */
uint32_t als_range_lux_milli(uint8_t step, uint16_t count);

/* Step for the next readings, after `count` at `step`. A dark reading goes to
 * the most sensitive step, a saturated one to a less sensitive step. */
uint8_t als_range_next(uint8_t step, uint16_t count);

#endif /* DISLED_ALS_RANGE_H */
