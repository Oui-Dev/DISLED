/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "als_range.h"

#include <stdbool.h>

/* A new step must give more than this many counts (the decision level of
 * the application note)... */
#define COUNT_PICK 100u
/* ...and keeps them down to half as many, so that it does not leave at the
 * next reading. */
#define COUNT_STAY (COUNT_PICK / 2u)
/* Above this, a less sensitive step (application note). */
#define COUNT_HIGH 10000u
/* A longer integration ends once half the time would give more than twice
 * COUNT_PICK counts: a step picked in the dark has room to stay. */
#define COUNT_LONG (4u * COUNT_PICK)
/* Gains 1 and 2 are linear up to 100 lx (application note), here in units
 * of `resolution`. */
#define HIGH_GAIN_MAX_LIGHT 1000000u

typedef struct {
    uint8_t gain_eighths;
    uint16_t integration_ms;
    /* Light of one count, in units of 0.1 mlx: 0.0042 lx at gain 2 and
     * 800 ms (datasheet rev. 1.8, 2024; the 2019 application note still gives
     * 0.0036), times 16 / gain_eighths, times 800 / integration_ms. */
    uint16_t resolution;
} step_t;

static const step_t steps[ALS_RANGE_STEPS] = {
    { 1, 100, 5376 }, { 2, 100, 2688 }, { 8, 100, 672 },
    { 16, 100, 336 }, { 16, 200, 168 }, { 16, 400, 84 },
};

_Static_assert(65535ull * 5376u < (1ull << 32), "32-bit light");

als_range_setting_t als_range_setting(uint8_t step)
{
    const als_range_setting_t setting = { steps[step].gain_eighths, steps[step].integration_ms };
    return setting;
}

uint32_t als_range_lux_milli(uint8_t step, uint16_t count)
{
    return ((uint32_t)count * steps[step].resolution + 5u) / 10u;
}

/* Most counts a step keeps before a less sensitive one takes over. */
static uint32_t upper_limit(uint8_t step)
{
    if (step == ALS_RANGE_FIRST) {
        return UINT16_MAX; /* nothing less sensitive */
    }
    if (steps[step].integration_ms > 100u) {
        return COUNT_LONG;
    }
    if (steps[step].gain_eighths >= 8u) {
        return HIGH_GAIN_MAX_LIGHT / steps[step].resolution;
    }
    return COUNT_HIGH;
}

uint8_t als_range_next(uint8_t step, uint16_t count)
{
    const bool enough = count > COUNT_STAY || step == ALS_RANGE_STEPS - 1u;
    if (enough && count <= upper_limit(step)) {
        return step;
    }
    /* A saturated count is a lower bound of the light: it still picks a step
     * that is not saturated. */
    const uint32_t light = (uint32_t)count * steps[step].resolution;
    for (uint8_t s = 0; s < ALS_RANGE_STEPS; s++) {
        if (light > COUNT_PICK * steps[s].resolution) {
            return s;
        }
    }
    return ALS_RANGE_STEPS - 1u;
}
