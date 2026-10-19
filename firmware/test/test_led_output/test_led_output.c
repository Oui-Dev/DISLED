/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include <unity.h>

#include "led_output.h"

#define PERIOD 3600u
/* 100 % in sub-ticks. */
#define FULL (PERIOD * LED_DUTY_SUBTICKS)

static const color_config_t colors = {
    .palette =
        {
            [LAMP_COLOR_GREEN] = {0, 65535, 0},
            [LAMP_COLOR_ORANGE] = {65535, 16384, 0},
            [LAMP_COLOR_RED] = {65535, 0, 0},
            [LAMP_COLOR_WARM_WHITE] = {65535, 65535, 65535},
        },
    .balance = {65535, 65535, 65535},
};

static const power_config_t power = {
    .full_scale_ma = { 86, 63, 125 },
    .unconfigured_cap_ma = 70,
    .configured_cap_ma = 125,
};

static const led_output_config_t config = {
    .color = &colors,
    .power = &power,
    .fade_ms = 300,
    .pwm_period = PERIOD,
};

static led_output_t out;
static uint16_t duty[LED_CHANNEL_COUNT];
static led_output_inputs_t in;

void setUp(void)
{
    led_output_init(&out, &config);
    in.on = true;
    in.color = LAMP_COLOR_GREEN;
    in.level_permille = 1000;
    in.link = USB_LINK_CONFIGURED;
    in.cap_override_ma = 0;
}

void tearDown(void)
{
}

static void test_pulse_loss_is_added_to_lit_channels_only(void)
{
    static led_output_config_t lossy;
    lossy = config;
    lossy.fade_ms = 0;
    lossy.pulse_loss[LED_CHANNEL_GREEN] = 80;
    lossy.pulse_loss[LED_CHANNEL_BLUE] = 80;
    led_output_init(&out, &lossy);

    /* Green at a low level gains the loss; blue stays dark. */
    in.level_permille = 100;
    led_output_update(&out, &in, 0, duty);
    led_output_update(&out, &in, 10, duty);
    const uint16_t low = duty[LED_CHANNEL_GREEN];
    TEST_ASSERT_EQUAL_UINT16(0, duty[LED_CHANNEL_BLUE]);
    led_output_init(&out, &config);
    led_output_update(&out, &in, 0, duty);
    led_output_update(&out, &in, 400, duty);
    TEST_ASSERT_EQUAL_UINT16(duty[LED_CHANNEL_GREEN] + 80u, low);

    /* Full scale never exceeds the period. */
    led_output_init(&out, &lossy);
    in.level_permille = 1000;
    led_output_update(&out, &in, 0, duty);
    led_output_update(&out, &in, 10, duty);
    TEST_ASSERT_EQUAL_UINT16(FULL, duty[LED_CHANNEL_GREEN]);
}

static void test_fade_in_from_dark(void)
{
    led_output_update(&out, &in, 1000, duty);
    TEST_ASSERT_EQUAL_UINT16(0, duty[LED_CHANNEL_GREEN]);

    led_output_update(&out, &in, 1150, duty);
    TEST_ASSERT_UINT16_WITHIN(30u * LED_DUTY_SUBTICKS, FULL / 2u, duty[LED_CHANNEL_GREEN]);

    led_output_update(&out, &in, 1300, duty);
    TEST_ASSERT_EQUAL_UINT16(FULL, duty[LED_CHANNEL_GREEN]);
    TEST_ASSERT_EQUAL_UINT16(0, duty[LED_CHANNEL_RED]);
    TEST_ASSERT_EQUAL_UINT16(0, duty[LED_CHANNEL_BLUE]);
}

static void test_turning_off_fades_out(void)
{
    led_output_update(&out, &in, 0, duty);
    led_output_update(&out, &in, 400, duty);
    in.on = false;
    led_output_update(&out, &in, 1000, duty);
    TEST_ASSERT_EQUAL_UINT16(FULL, duty[LED_CHANNEL_GREEN]);
    led_output_update(&out, &in, 1300, duty);
    TEST_ASSERT_EQUAL_UINT16(0, duty[LED_CHANNEL_GREEN]);
}

static void test_retarget_during_a_fade_does_not_jump(void)
{
    led_output_update(&out, &in, 0, duty);
    led_output_update(&out, &in, 150, duty);
    const uint16_t halfway = duty[LED_CHANNEL_GREEN];

    in.color = LAMP_COLOR_RED;
    led_output_update(&out, &in, 151, duty);
    TEST_ASSERT_UINT16_WITHIN(60u * LED_DUTY_SUBTICKS, halfway, duty[LED_CHANNEL_GREEN]);
    led_output_update(&out, &in, 451, duty);
    TEST_ASSERT_EQUAL_UINT16(0, duty[LED_CHANNEL_GREEN]);
    TEST_ASSERT_EQUAL_UINT16(FULL, duty[LED_CHANNEL_RED]);
}

static void test_suspend_turns_off_immediately(void)
{
    led_output_update(&out, &in, 0, duty);
    led_output_update(&out, &in, 400, duty);
    in.link = USB_LINK_SUSPENDED;
    led_output_update(&out, &in, 401, duty);
    TEST_ASSERT_EQUAL_UINT16(0, duty[LED_CHANNEL_GREEN]);
    TEST_ASSERT_EQUAL_UINT16(0, led_output_cap_ma(&out));
}

static void test_white_is_capped_by_the_usb_state(void)
{
    in.color = LAMP_COLOR_WARM_WHITE; /* full white in this palette */
    led_output_update(&out, &in, 0, duty);
    led_output_update(&out, &in, 400, duty);
    TEST_ASSERT_TRUE(led_output_estimate_ma_x10(&out) <= 1250u);

    in.link = USB_LINK_UNCONFIGURED;
    led_output_update(&out, &in, 401, duty);
    TEST_ASSERT_TRUE(led_output_estimate_ma_x10(&out) <= 700u);
    TEST_ASSERT_EQUAL_UINT16(70, led_output_cap_ma(&out));
}

static void test_cap_override_for_the_thermal_test(void)
{
    in.color = LAMP_COLOR_WARM_WHITE;
    in.cap_override_ma = 300;
    led_output_update(&out, &in, 0, duty);
    led_output_update(&out, &in, 400, duty);
    TEST_ASSERT_EQUAL_UINT16(FULL, duty[LED_CHANNEL_BLUE]);
    TEST_ASSERT_UINT32_WITHIN(1, 2740, led_output_estimate_ma_x10(&out));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_pulse_loss_is_added_to_lit_channels_only);
    RUN_TEST(test_fade_in_from_dark);
    RUN_TEST(test_turning_off_fades_out);
    RUN_TEST(test_retarget_during_a_fade_does_not_jump);
    RUN_TEST(test_suspend_turns_off_immediately);
    RUN_TEST(test_white_is_capped_by_the_usb_state);
    RUN_TEST(test_cap_override_for_the_thermal_test);
    return UNITY_END();
}
