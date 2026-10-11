/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include <unity.h>

#include "brightness.h"

#define LUX(x) ((uint32_t)(x) * 100u)

static const brightness_point_t curve[] = {
    { LUX(1), 20 },
    { LUX(10), 80 },
    { LUX(100), 300 },
    { LUX(1000), 800 },
    { LUX(3000), 1000 },
};

static const brightness_config_t config = {
    .curve = curve,
    .curve_points = sizeof curve / sizeof curve[0],
    .filter_shift = 5,
    .deadband_permille = 10,
    .fallback_permille = 500,
};

static brightness_t b;

void setUp(void)
{
    brightness_init(&b, &config);
}

void tearDown(void)
{
}

static void test_curve_clamps_outside_its_range(void)
{
    TEST_ASSERT_EQUAL_UINT16(20, brightness_curve_permille(&config, 0));
    TEST_ASSERT_EQUAL_UINT16(20, brightness_curve_permille(&config, LUX(1)));
    TEST_ASSERT_EQUAL_UINT16(1000, brightness_curve_permille(&config, LUX(100000)));
}

static void test_curve_hits_its_points(void)
{
    TEST_ASSERT_EQUAL_UINT16(80, brightness_curve_permille(&config, LUX(10)));
    TEST_ASSERT_EQUAL_UINT16(300, brightness_curve_permille(&config, LUX(100)));
    TEST_ASSERT_EQUAL_UINT16(800, brightness_curve_permille(&config, LUX(1000)));
}

static void test_curve_interpolates_on_a_log_scale(void)
{
    /* 31.6 lx is halfway between 10 and 100 lx on a log scale. */
    TEST_ASSERT_UINT16_WITHIN(15, 190, brightness_curve_permille(&config, 3162u));
}

static void test_curve_is_monotonic(void)
{
    uint16_t previous = 0;
    for (uint32_t lux_centi = 1; lux_centi < LUX(5000); lux_centi = lux_centi * 11u / 10u + 1u) {
        const uint16_t level = brightness_curve_permille(&config, lux_centi);
        TEST_ASSERT_TRUE(level >= previous);
        previous = level;
    }
}

static void test_fallback_until_the_first_reading(void)
{
    TEST_ASSERT_EQUAL_UINT16(500, brightness_level_permille(&b));
    brightness_update_lux(&b, LUX(100));
    TEST_ASSERT_EQUAL_UINT16(300, brightness_level_permille(&b));
}

static void test_level_moves_slowly_and_monotonically(void)
{
    brightness_update_lux(&b, LUX(10)); /* 80 per mille */
    uint16_t previous = brightness_level_permille(&b);
    for (int i = 0; i < 5; i++) {
        brightness_update_lux(&b, LUX(1000)); /* target 800 */
        const uint16_t level = brightness_level_permille(&b);
        TEST_ASSERT_TRUE(level > previous);
        TEST_ASSERT_TRUE(level < 400); /* no jump */
        previous = level;
    }
    for (int i = 0; i < 300; i++) {
        brightness_update_lux(&b, LUX(1000));
    }
    TEST_ASSERT_UINT16_WITHIN(10, 800, brightness_level_permille(&b));
}

static void test_small_changes_stay_in_the_dead_band(void)
{
    brightness_update_lux(&b, LUX(100));
    const uint16_t level = brightness_level_permille(&b);
    for (int i = 0; i < 100; i++) {
        brightness_update_lux(&b, (i % 2) ? LUX(102) : LUX(98));
    }
    TEST_ASSERT_EQUAL_UINT16(level, brightness_level_permille(&b));
}

static void test_manual_mode_uses_the_host_level(void)
{
    brightness_update_lux(&b, LUX(100));
    brightness_set_mode(&b, BRIGHTNESS_MODE_MANUAL, 37);
    TEST_ASSERT_EQUAL_UINT16(370, brightness_level_permille(&b));
    TEST_ASSERT_EQUAL_UINT8(37, brightness_level_percent(&b));
}

static void test_back_to_auto_starts_from_the_manual_level(void)
{
    brightness_update_lux(&b, LUX(1000));
    brightness_set_mode(&b, BRIGHTNESS_MODE_MANUAL, 10);
    brightness_set_mode(&b, BRIGHTNESS_MODE_AUTO, 10);
    TEST_ASSERT_EQUAL_UINT16(100, brightness_level_permille(&b));
    brightness_update_lux(&b, LUX(1000));
    TEST_ASSERT_TRUE(brightness_level_permille(&b) > 100);
    TEST_ASSERT_TRUE(brightness_level_permille(&b) < 200);
}

static void test_lost_sensor_falls_back(void)
{
    brightness_update_lux(&b, LUX(1));
    brightness_lux_lost(&b);
    TEST_ASSERT_EQUAL_UINT16(500, brightness_level_permille(&b));
}

static void test_percent_is_rounded(void)
{
    brightness_set_mode(&b, BRIGHTNESS_MODE_MANUAL, 1);
    TEST_ASSERT_EQUAL_UINT8(1, brightness_level_percent(&b));
    brightness_set_mode(&b, BRIGHTNESS_MODE_MANUAL, 100);
    TEST_ASSERT_EQUAL_UINT8(100, brightness_level_percent(&b));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_curve_clamps_outside_its_range);
    RUN_TEST(test_curve_hits_its_points);
    RUN_TEST(test_curve_interpolates_on_a_log_scale);
    RUN_TEST(test_curve_is_monotonic);
    RUN_TEST(test_fallback_until_the_first_reading);
    RUN_TEST(test_level_moves_slowly_and_monotonically);
    RUN_TEST(test_small_changes_stay_in_the_dead_band);
    RUN_TEST(test_manual_mode_uses_the_host_level);
    RUN_TEST(test_back_to_auto_starts_from_the_manual_level);
    RUN_TEST(test_lost_sensor_falls_back);
    RUN_TEST(test_percent_is_rounded);
    return UNITY_END();
}
