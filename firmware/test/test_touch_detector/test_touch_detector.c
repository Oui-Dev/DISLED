/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Touch detection on synthetic TouchKey signals sampled at 1 kHz.
 */
#include <unity.h>

#include "touch_detector.h"

#define BASELINE 2000u
/* A touch lowers the reading by 5 % (50 per mille). */
#define TOUCHED 1900u

static const touch_config_t config = {
    .window_samples = 20,
    .calibration_windows = 8,
    .baseline_shift = 6,
    .press_permille = 40,
    .release_permille = 25,
    .press_rise_permille = 18,
    .proximity_permille = 10,
    .debounce_ms = 60,
    .rearm_ms = 100,
    .long_press_ms = 2000,
    .object_ms = 10000,
    .inverted_permille = 30,
    .inverted_ms = 1000,
};

static touch_detector_t td;
static uint32_t now_ms;
static unsigned short_presses;
static unsigned long_presses;
static uint32_t last_event_ms;

static void reset_counts(void)
{
    short_presses = 0;
    long_presses = 0;
    last_event_ms = 0;
}

static void record(touch_event_t event)
{
    if (event == TOUCH_EVENT_SHORT_PRESS) {
        short_presses++;
        last_event_ms = now_ms;
    } else if (event == TOUCH_EVENT_LONG_PRESS) {
        long_presses++;
        last_event_ms = now_ms;
    }
}

/* One sample per millisecond. */
static void hold(uint16_t raw, uint32_t duration_ms)
{
    for (uint32_t i = 0; i < duration_ms; i++) {
        now_ms++;
        record(touch_detector_feed(&td, raw, now_ms));
    }
}

/* 50 Hz hum of the given amplitude (counts) on top of `raw`. */
static void hold_with_hum(uint16_t raw, int16_t amplitude, uint32_t duration_ms)
{
    /* sin(2*pi*k/20) * 1000, one 50 Hz period at 1 kHz. */
    static const int16_t sine[20] = { 0, 309,  588,  809,  951,  1000,  951,  809,  588,  309,
                                      0, -309, -588, -809, -951, -1000, -951, -809, -588, -309 };
    for (uint32_t i = 0; i < duration_ms; i++) {
        now_ms++;
        const int32_t hum = (amplitude * sine[now_ms % 20u]) / 1000;
        record(touch_detector_feed(&td, (uint16_t)((int32_t)raw + hum), now_ms));
    }
}

void setUp(void)
{
    now_ms = 0;
    reset_counts();
    touch_detector_init(&td, &config);
    hold(BASELINE, 200); /* 8 calibration windows take 160 ms */
}

void tearDown(void)
{
}

static void test_calibration_sets_the_baseline(void)
{
    TEST_ASSERT_EQUAL(TOUCH_STATE_IDLE, touch_detector_state(&td));
    TEST_ASSERT_EQUAL_UINT16(BASELINE, touch_detector_baseline(&td));
    TEST_ASSERT_EQUAL_INT16(0, touch_detector_delta_permille(&td));
}

static void test_recalibration_drops_the_old_baseline(void)
{
    /* As at the end of a USB suspend: the old baseline may no longer hold. */
    touch_detector_recalibrate(&td);
    TEST_ASSERT_EQUAL(TOUCH_STATE_CALIBRATING, touch_detector_state(&td));
    TEST_ASSERT_EQUAL_UINT16(0, touch_detector_baseline(&td));

    hold(BASELINE + 100u, 200);
    TEST_ASSERT_EQUAL(TOUCH_STATE_IDLE, touch_detector_state(&td));
    TEST_ASSERT_EQUAL_UINT16(BASELINE + 100u, touch_detector_baseline(&td));
}

static void test_flat_signal_gives_no_event(void)
{
    hold(BASELINE, 5000);
    TEST_ASSERT_EQUAL_UINT(0, short_presses + long_presses);
    TEST_ASSERT_EQUAL_UINT16(0, touch_detector_recalibrations(&td));
}

static void test_short_press_fires_once_on_release(void)
{
    hold(TOUCHED, 300);
    TEST_ASSERT_EQUAL(TOUCH_STATE_PRESSED, touch_detector_state(&td));
    TEST_ASSERT_EQUAL_UINT(0, short_presses);

    hold(BASELINE, 300);
    TEST_ASSERT_EQUAL_UINT(1, short_presses);
    TEST_ASSERT_EQUAL_UINT(0, long_presses);
    TEST_ASSERT_EQUAL(TOUCH_STATE_IDLE, touch_detector_state(&td));
}

static void test_blip_shorter_than_debounce_is_ignored(void)
{
    hold(TOUCHED, 40);
    hold(BASELINE, 500);
    TEST_ASSERT_EQUAL_UINT(0, short_presses + long_presses);
}

static void test_long_press_fires_at_two_seconds_while_held(void)
{
    const uint32_t start = now_ms;
    hold(TOUCHED, 2100);
    TEST_ASSERT_EQUAL_UINT(1, long_presses);
    /* Timed from the first contact, within one window. */
    TEST_ASSERT_UINT32_WITHIN(25, start + 2000u, last_event_ms);

    hold(TOUCHED, 1000);
    hold(BASELINE, 300);
    TEST_ASSERT_EQUAL_UINT(1, long_presses);
    TEST_ASSERT_EQUAL_UINT(0, short_presses);
}

static void test_object_on_lid_fires_once_then_is_absorbed(void)
{
    hold(TOUCHED, 15000);
    TEST_ASSERT_EQUAL_UINT(1, long_presses);
    TEST_ASSERT_EQUAL_UINT(0, short_presses);
    TEST_ASSERT_EQUAL(TOUCH_STATE_IDLE, touch_detector_state(&td));
    TEST_ASSERT_UINT16_WITHIN(2, TOUCHED, touch_detector_baseline(&td));

    /* Object removed: the reading rises, the baseline recalibrates and
     * nothing fires. */
    hold(BASELINE, 2000);
    TEST_ASSERT_EQUAL_UINT(1, long_presses);
    TEST_ASSERT_EQUAL_UINT(0, short_presses);
    TEST_ASSERT_UINT16_WITHIN(2, BASELINE, touch_detector_baseline(&td));

    /* The touch works again. */
    hold(TOUCHED, 300);
    hold(BASELINE, 300);
    TEST_ASSERT_EQUAL_UINT(1, short_presses);
}

static void test_mains_hum_is_averaged_out(void)
{
    /* +-3 % hum, close to the press threshold, without any touch. */
    hold_with_hum(BASELINE, 60, 5000);
    TEST_ASSERT_EQUAL_UINT(0, short_presses + long_presses);

    /* A touch with the same hum is still detected. */
    hold_with_hum(TOUCHED, 60, 300);
    hold_with_hum(BASELINE, 60, 300);
    TEST_ASSERT_EQUAL_UINT(1, short_presses);
}

static void test_baseline_follows_slow_drift(void)
{
    /* +5 % over 50 s: as large as a touch, but far too slow. */
    for (uint16_t step = 0; step <= 100; step++) {
        hold((uint16_t)(BASELINE + step), 500);
    }
    TEST_ASSERT_EQUAL_UINT(0, short_presses + long_presses);
    TEST_ASSERT_UINT16_WITHIN(5, BASELINE + 100u, touch_detector_baseline(&td));
}

static void test_finger_at_power_up_recalibrates(void)
{
    touch_detector_init(&td, &config);
    hold(TOUCHED, 200);   /* calibrated with the finger on the lid */
    hold(BASELINE, 1500); /* finger lifted: reading above the baseline */
    TEST_ASSERT_EQUAL_UINT(0, short_presses + long_presses);
    TEST_ASSERT_UINT16_WITHIN(2, BASELINE, touch_detector_baseline(&td));

    hold(TOUCHED, 300);
    hold(BASELINE, 300);
    TEST_ASSERT_EQUAL_UINT(1, short_presses);
}

static void test_hysteresis_keeps_the_press_between_thresholds(void)
{
    hold(TOUCHED, 200);
    /* 3 % drop: below the press threshold but above the release one. */
    hold((uint16_t)(BASELINE - 60u), 500);
    TEST_ASSERT_EQUAL(TOUCH_STATE_PRESSED, touch_detector_state(&td));
    hold(BASELINE, 300);
    TEST_ASSERT_EQUAL_UINT(1, short_presses);
}

static void test_hand_near_the_lid_freezes_the_baseline(void)
{
    /* 2 % drop for 5 s: a hand close to the lid, no touch. */
    hold((uint16_t)(BASELINE - 40u), 5000);
    TEST_ASSERT_EQUAL_UINT(0, short_presses + long_presses);
    TEST_ASSERT_EQUAL_UINT16(BASELINE, touch_detector_baseline(&td));

    /* The baseline was not pulled down: the touch keeps its full drop. */
    hold(BASELINE, 300);
    hold(TOUCHED, 300);
    TEST_ASSERT_EQUAL_INT16(50, touch_detector_delta_permille(&td));
    hold(BASELINE, 300);
    TEST_ASSERT_EQUAL_UINT(1, short_presses);
}

static void test_object_near_the_lid_is_absorbed(void)
{
    hold((uint16_t)(BASELINE - 40u), 12000);
    TEST_ASSERT_EQUAL_UINT(0, short_presses + long_presses);
    TEST_ASSERT_UINT16_WITHIN(1, BASELINE - 40u, touch_detector_baseline(&td));
    TEST_ASSERT_EQUAL_UINT16(1, touch_detector_recalibrations(&td));
}

static void test_a_rise_after_a_hand_near_the_lid_waits_its_own_delay(void)
{
    /* The hand leaves 1 s short of an object, then the reading rises above
     * the baseline: the rise counts its own second, not the hand's time. */
    hold((uint16_t)(BASELINE - 40u), 9000);
    hold(BASELINE + 80u, 900);
    TEST_ASSERT_EQUAL_UINT16(0, touch_detector_recalibrations(&td));
    TEST_ASSERT_EQUAL_UINT16(BASELINE, touch_detector_baseline(&td));

    hold(BASELINE + 80u, 200);
    TEST_ASSERT_EQUAL_UINT16(1, touch_detector_recalibrations(&td));
    TEST_ASSERT_EQUAL_UINT16(BASELINE + 80u, touch_detector_baseline(&td));
}

static void test_a_press_restarts_the_hand_near_the_lid_delay(void)
{
    /* Without the restart, the hand would pass for an object 1 s after the tap. */
    hold((uint16_t)(BASELINE - 40u), 9000);
    hold(TOUCHED, 300);
    hold((uint16_t)(BASELINE - 40u), 2000);
    TEST_ASSERT_EQUAL_UINT(1, short_presses);
    TEST_ASSERT_EQUAL_UINT16(0, touch_detector_recalibrations(&td));
    TEST_ASSERT_EQUAL_UINT16(BASELINE, touch_detector_baseline(&td));
}

static void test_taps_with_the_hand_resting_near_the_lid(void)
{
    /* The resting hand (30 per mille) sits above the release threshold:
     * without the floor, the taps would merge into one long press. */
    hold((uint16_t)(BASELINE - 60u), 500);
    for (int i = 0; i < 5; i++) {
        hold((uint16_t)(BASELINE - 110u), 150); /* tap: 55 per mille */
        hold((uint16_t)(BASELINE - 60u), 400);
    }
    TEST_ASSERT_EQUAL_UINT(5, short_presses);
    TEST_ASSERT_EQUAL_UINT(0, long_presses);
}

static void test_long_press_with_the_hand_resting_near_the_lid(void)
{
    hold((uint16_t)(BASELINE - 60u), 500);
    hold((uint16_t)(BASELINE - 110u), 2100);
    TEST_ASSERT_EQUAL_UINT(1, long_presses);
    hold((uint16_t)(BASELINE - 60u), 400);
    TEST_ASSERT_EQUAL_UINT(0, short_presses);
}

static void test_blip_right_after_a_release_is_ignored(void)
{
    hold(TOUCHED, 300);
    hold(BASELINE, 80); /* released after the debounce */
    TEST_ASSERT_EQUAL_UINT(1, short_presses);
    hold(TOUCHED, 80); /* within the re-arm delay */
    hold(BASELINE, 300);
    TEST_ASSERT_EQUAL_UINT(1, short_presses);
}

static void test_a_consumed_touch_fires_nothing(void)
{
    /* Consumed once pressed. */
    hold(TOUCHED, 300);
    touch_detector_consume(&td);
    hold(BASELINE, 300);
    TEST_ASSERT_EQUAL_UINT(0, short_presses + long_presses);

    /* The next touch works again. */
    hold(TOUCHED, 300);
    hold(BASELINE, 300);
    TEST_ASSERT_EQUAL_UINT(1, short_presses);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_calibration_sets_the_baseline);
    RUN_TEST(test_recalibration_drops_the_old_baseline);
    RUN_TEST(test_flat_signal_gives_no_event);
    RUN_TEST(test_short_press_fires_once_on_release);
    RUN_TEST(test_blip_shorter_than_debounce_is_ignored);
    RUN_TEST(test_long_press_fires_at_two_seconds_while_held);
    RUN_TEST(test_object_on_lid_fires_once_then_is_absorbed);
    RUN_TEST(test_mains_hum_is_averaged_out);
    RUN_TEST(test_baseline_follows_slow_drift);
    RUN_TEST(test_finger_at_power_up_recalibrates);
    RUN_TEST(test_hysteresis_keeps_the_press_between_thresholds);
    RUN_TEST(test_hand_near_the_lid_freezes_the_baseline);
    RUN_TEST(test_object_near_the_lid_is_absorbed);
    RUN_TEST(test_a_rise_after_a_hand_near_the_lid_waits_its_own_delay);
    RUN_TEST(test_a_press_restarts_the_hand_near_the_lid_delay);
    RUN_TEST(test_taps_with_the_hand_resting_near_the_lid);
    RUN_TEST(test_long_press_with_the_hand_resting_near_the_lid);
    RUN_TEST(test_blip_right_after_a_release_is_ignored);
    RUN_TEST(test_a_consumed_touch_fires_nothing);
    return UNITY_END();
}
