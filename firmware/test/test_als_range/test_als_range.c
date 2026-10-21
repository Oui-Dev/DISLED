/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include <unity.h>

#include "als_range.h"

#define LAST   ((uint8_t)(ALS_RANGE_STEPS - 1u))
#define LUX(x) ((uint32_t)(x) * 1000u)

void setUp(void)
{
}

void tearDown(void)
{
}

/* Light of one count, in units of 0.1 mlx. */
static uint32_t resolution(uint8_t step)
{
    return als_range_lux_milli(step, 10000) / 1000u;
}

/* What the sensor would read at a step, saturating like it does. */
static uint16_t count_at(uint8_t step, uint32_t lux_milli)
{
    const uint32_t count = (lux_milli * 10u) / resolution(step);
    return (uint16_t)((count > UINT16_MAX) ? UINT16_MAX : count);
}

/* Readings of a steady light until the step stops changing. */
static uint8_t settle(uint8_t step, uint32_t lux_milli, int *switches)
{
    *switches = 0;
    for (;;) {
        const uint8_t next = als_range_next(step, count_at(step, lux_milli));
        if (next == step) {
            return step;
        }
        step = next;
        (*switches)++;
        TEST_ASSERT_TRUE_MESSAGE(*switches <= 3, "the step keeps changing");
    }
}

static void test_steps_follow_the_datasheet(void)
{
    TEST_ASSERT_EQUAL_UINT8(1, als_range_setting(ALS_RANGE_FIRST).gain_eighths);
    TEST_ASSERT_EQUAL_UINT16(100, als_range_setting(ALS_RANGE_FIRST).integration_ms);
    TEST_ASSERT_EQUAL_UINT8(16, als_range_setting(LAST).gain_eighths);
    TEST_ASSERT_EQUAL_UINT16(400, als_range_setting(LAST).integration_ms);
    for (uint8_t step = 0; step < ALS_RANGE_STEPS; step++) {
        const als_range_setting_t s = als_range_setting(step);
        TEST_ASSERT_EQUAL_UINT32(42u * (16u / s.gain_eighths) * (800u / s.integration_ms),
                                 resolution(step));
        if (step > 0u) {
            TEST_ASSERT_TRUE(resolution(step) < resolution((uint8_t)(step - 1u)));
        }
    }
}

static void test_counts_become_lux(void)
{
    TEST_ASSERT_EQUAL_UINT32(53760, als_range_lux_milli(ALS_RANGE_FIRST, 100));
    TEST_ASSERT_EQUAL_UINT32(35231616, als_range_lux_milli(ALS_RANGE_FIRST, 65535));
    TEST_ASSERT_EQUAL_UINT32(8, als_range_lux_milli(LAST, 1)); /* 8.4 mlx */
    TEST_ASSERT_EQUAL_UINT32(840, als_range_lux_milli(LAST, 100));
}

static void test_darkness_goes_to_the_most_sensitive_step(void)
{
    TEST_ASSERT_EQUAL_UINT8(LAST, als_range_next(ALS_RANGE_FIRST, 0));
    TEST_ASSERT_EQUAL_UINT8(LAST, als_range_next(LAST, 3)); /* dark offset */
    TEST_ASSERT_EQUAL_UINT8(LAST, als_range_next(LAST, 0));
}

static void test_saturation_goes_to_a_step_that_is_not_saturated(void)
{
    TEST_ASSERT_EQUAL_UINT8(ALS_RANGE_FIRST, als_range_next(LAST, UINT16_MAX));
    TEST_ASSERT_EQUAL_UINT8(ALS_RANGE_FIRST, als_range_next(3, UINT16_MAX));
    TEST_ASSERT_EQUAL_UINT8(ALS_RANGE_FIRST, als_range_next(ALS_RANGE_FIRST, UINT16_MAX));
}

static void test_a_count_in_its_band_keeps_the_step(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, als_range_next(0, 500));
    TEST_ASSERT_EQUAL_UINT8(2, als_range_next(2, 300));
    TEST_ASSERT_EQUAL_UINT8(3, als_range_next(3, 51));
    TEST_ASSERT_EQUAL_UINT8(LAST, als_range_next(LAST, 150));
}

static void test_dimmer_light_picks_a_step_with_more_than_100_counts(void)
{
    /* 26.9 lx: 50 counts at gain 1/8, 100 at 1/4, 400 at 1. */
    TEST_ASSERT_EQUAL_UINT8(2, als_range_next(0, 50));
    /* 5 lx still reads at 100 ms, 0.5 lx at 400 ms (59 counts). */
    int switches = 0;
    TEST_ASSERT_EQUAL_UINT16(100, als_range_setting(settle(0, LUX(5), &switches)).integration_ms);
    TEST_ASSERT_EQUAL_UINT16(400, als_range_setting(settle(0, 500, &switches)).integration_ms);
    /* 2 lx: 3 counts at the first step pick 400 ms, where it reads 238. */
    const uint8_t step = settle(0, LUX(2), &switches);
    TEST_ASSERT_EQUAL_UINT16(400, als_range_setting(step).integration_ms);
    TEST_ASSERT_EQUAL_UINT16(238, count_at(step, LUX(2)));
}

static void test_gains_1_and_2_stay_below_100_lx(void)
{
    TEST_ASSERT_EQUAL_UINT8(2, als_range_next(2, 1488)); /* 99.99 lx */
    TEST_ASSERT_EQUAL_UINT8(0, als_range_next(2, 1489));
    TEST_ASSERT_EQUAL_UINT8(3, als_range_next(3, 2976));
    TEST_ASSERT_EQUAL_UINT8(0, als_range_next(3, 2977));
}

static void test_longer_integrations_end_when_shorter_ones_are_enough(void)
{
    TEST_ASSERT_EQUAL_UINT8(LAST, als_range_next(LAST, 400));
    /* 401 counts at 400 ms are 100.25 at 100 ms. */
    TEST_ASSERT_EQUAL_UINT8(LAST - 2u, als_range_next(LAST, 401));
    /* Lights on in a dark room: 30 lx (3571 counts at 400 ms) go straight to
     * gain 1/4 at 100 ms, which reads 111. */
    TEST_ASSERT_EQUAL_UINT8(1, als_range_next(LAST, count_at(LAST, LUX(30))));
}

static void test_every_light_settles_at_once_and_stays(void)
{
    for (uint8_t start = 0; start < ALS_RANGE_STEPS; start++) {
        for (uint32_t lux = 1; lux < LUX(40000); lux = lux * 21u / 20u + 1u) {
            int switches = 0;
            uint8_t step = settle(start, lux, &switches);
            TEST_ASSERT_TRUE(switches <= 1);
            const uint16_t count = count_at(step, lux);
            TEST_ASSERT_TRUE(count < UINT16_MAX || step == ALS_RANGE_FIRST);
            if (als_range_setting(step).gain_eighths >= 8u) {
                TEST_ASSERT_TRUE(lux <= LUX(100));
            }
            /* A light flickering 5 % either way changes the step at most
             * once (from the edge of a band), then never again. */
            int changes = 0;
            for (int i = 0; i < 6; i++) {
                const uint32_t flicker = (i % 2) ? lux * 21u / 20u : lux * 20u / 21u;
                const uint8_t next = als_range_next(step, count_at(step, flicker));
                if (next != step) {
                    changes++;
                    step = next;
                }
            }
            TEST_ASSERT_TRUE(changes <= 1);
        }
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_steps_follow_the_datasheet);
    RUN_TEST(test_counts_become_lux);
    RUN_TEST(test_darkness_goes_to_the_most_sensitive_step);
    RUN_TEST(test_saturation_goes_to_a_step_that_is_not_saturated);
    RUN_TEST(test_a_count_in_its_band_keeps_the_step);
    RUN_TEST(test_dimmer_light_picks_a_step_with_more_than_100_counts);
    RUN_TEST(test_gains_1_and_2_stay_below_100_lx);
    RUN_TEST(test_longer_integrations_end_when_shorter_ones_are_enough);
    RUN_TEST(test_every_light_settles_at_once_and_stays);
    return UNITY_END();
}
