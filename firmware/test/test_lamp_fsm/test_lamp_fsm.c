/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Lamp rules from docs/behavior.md.
 */
#include <unity.h>

#include "lamp_fsm.h"

static lamp_state_t lamp;

void setUp(void)
{
    lamp_fsm_init(&lamp);
}

void tearDown(void)
{
}

static void test_power_up_is_on_green_auto(void)
{
    TEST_ASSERT_TRUE(lamp.on);
    TEST_ASSERT_EQUAL(LAMP_COLOR_GREEN, lamp.color);
    TEST_ASSERT_EQUAL(BRIGHTNESS_MODE_AUTO, lamp.brightness_mode);
    TEST_ASSERT_EQUAL_UINT8(LAMP_DEFAULT_MANUAL_LEVEL, lamp.manual_level);
}

static void test_short_press_cycles_green_orange_red(void)
{
    TEST_ASSERT_TRUE(lamp_fsm_short_press(&lamp));
    TEST_ASSERT_EQUAL(LAMP_COLOR_ORANGE, lamp.color);
    TEST_ASSERT_TRUE(lamp_fsm_short_press(&lamp));
    TEST_ASSERT_EQUAL(LAMP_COLOR_RED, lamp.color);
    TEST_ASSERT_TRUE(lamp_fsm_short_press(&lamp));
    TEST_ASSERT_EQUAL(LAMP_COLOR_GREEN, lamp.color);
}

static void test_short_press_from_warm_white_goes_green(void)
{
    lamp.color = LAMP_COLOR_WARM_WHITE;
    TEST_ASSERT_TRUE(lamp_fsm_short_press(&lamp));
    TEST_ASSERT_EQUAL(LAMP_COLOR_GREEN, lamp.color);
}

static void test_short_press_is_ignored_while_off(void)
{
    lamp.on = false;
    lamp.color = LAMP_COLOR_RED;
    TEST_ASSERT_FALSE(lamp_fsm_short_press(&lamp));
    TEST_ASSERT_FALSE(lamp.on);
    TEST_ASSERT_EQUAL(LAMP_COLOR_RED, lamp.color);
}

static void test_long_press_turns_off_then_back_on_with_last_color(void)
{
    lamp.color = LAMP_COLOR_ORANGE;
    TEST_ASSERT_TRUE(lamp_fsm_long_press(&lamp));
    TEST_ASSERT_FALSE(lamp.on);
    TEST_ASSERT_TRUE(lamp_fsm_long_press(&lamp));
    TEST_ASSERT_TRUE(lamp.on);
    TEST_ASSERT_EQUAL(LAMP_COLOR_ORANGE, lamp.color);
}

static void test_request_applies_only_selected_fields(void)
{
    const lamp_request_t request = {
        .fields = LAMP_FIELD_COLOR,
        .on = false,
        .color = LAMP_COLOR_WARM_WHITE,
        .brightness_mode = BRIGHTNESS_MODE_MANUAL,
        .manual_level = 7,
    };
    TEST_ASSERT_TRUE(lamp_fsm_apply(&lamp, &request));
    TEST_ASSERT_TRUE(lamp.on);
    TEST_ASSERT_EQUAL(LAMP_COLOR_WARM_WHITE, lamp.color);
    TEST_ASSERT_EQUAL(BRIGHTNESS_MODE_AUTO, lamp.brightness_mode);
    TEST_ASSERT_EQUAL_UINT8(LAMP_DEFAULT_MANUAL_LEVEL, lamp.manual_level);
}

static void test_request_setting_the_current_values_reports_no_change(void)
{
    const lamp_request_t request = {
        .fields = LAMP_FIELD_POWER | LAMP_FIELD_COLOR,
        .on = true,
        .color = LAMP_COLOR_GREEN,
    };
    TEST_ASSERT_FALSE(lamp_fsm_apply(&lamp, &request));
}

static void test_color_set_while_off_stays_off(void)
{
    lamp.on = false;
    const lamp_request_t request = { .fields = LAMP_FIELD_COLOR, .color = LAMP_COLOR_RED };
    TEST_ASSERT_TRUE(lamp_fsm_apply(&lamp, &request));
    TEST_ASSERT_FALSE(lamp.on);
    TEST_ASSERT_EQUAL(LAMP_COLOR_RED, lamp.color);
}

static void test_invalid_request_changes_nothing(void)
{
    const lamp_state_t before = lamp;
    const lamp_request_t bad_level = {
        .fields = LAMP_FIELD_POWER | LAMP_FIELD_MANUAL_LEVEL,
        .on = false,
        .manual_level = 0,
    };
    TEST_ASSERT_FALSE(lamp_request_is_valid(&bad_level));
    TEST_ASSERT_FALSE(lamp_fsm_apply(&lamp, &bad_level));
    TEST_ASSERT_EQUAL_MEMORY(&before, &lamp, sizeof lamp);
}

static void test_request_validation_limits(void)
{
    lamp_request_t request = { .fields = LAMP_FIELD_MANUAL_LEVEL, .manual_level = 101 };
    TEST_ASSERT_FALSE(lamp_request_is_valid(&request));
    request.manual_level = 100;
    TEST_ASSERT_TRUE(lamp_request_is_valid(&request));
    request.manual_level = 1;
    TEST_ASSERT_TRUE(lamp_request_is_valid(&request));

    request.fields = LAMP_FIELD_COLOR;
    request.color = (lamp_color_t)4;
    TEST_ASSERT_FALSE(lamp_request_is_valid(&request));

    request.fields = LAMP_FIELD_BRIGHTNESS_MODE;
    request.brightness_mode = (brightness_mode_t)2;
    TEST_ASSERT_FALSE(lamp_request_is_valid(&request));

    request.fields = 0x10;
    TEST_ASSERT_FALSE(lamp_request_is_valid(&request));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_power_up_is_on_green_auto);
    RUN_TEST(test_short_press_cycles_green_orange_red);
    RUN_TEST(test_short_press_from_warm_white_goes_green);
    RUN_TEST(test_short_press_is_ignored_while_off);
    RUN_TEST(test_long_press_turns_off_then_back_on_with_last_color);
    RUN_TEST(test_request_applies_only_selected_fields);
    RUN_TEST(test_request_setting_the_current_values_reports_no_change);
    RUN_TEST(test_color_set_while_off_stays_off);
    RUN_TEST(test_invalid_request_changes_nothing);
    RUN_TEST(test_request_validation_limits);
    return UNITY_END();
}
