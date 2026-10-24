/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "app_config.h"

#include "bsp/ch32.h"

#define LUX(x)  ((uint32_t)(x) * 1000u)
#define MLUX(x) ((uint32_t)(x))

/* TouchKey timing, measured on the first board (bring-up): the ADC sample
 * time sets the range, the charge time the position in it. With 1.5 cycles
 * and charge 3 the reading sits mid-scale (about 2600 of 4095, noise about
 * +-6 per mille). The WCH example values (charge 16, 7.5 cycles) saturate.
 *
 * Thresholds, measured on the final board (second board, ring 28.5/17.5 mm,
 * closed enclosure, desktop PC USB, 2026-10-24). Idle reading about 2100,
 * noise about 4.5 per mille (single windows up to 25); the LED, off to
 * 125 mA, shifts nothing. A light touch lowers the reading by 77-188 per
 * mille at its peak (least on the center, over the hole of the ring), a
 * firm one by 120-200, a held one by about 170. A hand
 * held 1 cm above the lid lowers it by 60-80 (peaks to 100), at 2 cm by
 * 40-55, at 5 cm by about 3; a hand passing 1 cm above peaks at 33-58.
 *
 * The first board's thresholds (press 28, rise 18) took a hand at 1-2 cm
 * and every pass for a touch, and a hand held 2 s over the lid for a long
 * press. A hand nears the lid more slowly than a finger lands on it, so the
 * floor follows the hand and its rise stays small: the rise above the floor
 * now does most of the sorting. Replayed on two labelled recordings (20 ms
 * windows: the calibration, then a check after flashing; 21 taps, 3 long
 * presses, hands held at 1 and 2 cm, 6 passes), these settings found every
 * tap and long press, and no hand held or passing; idle and the LED steps
 * gave nothing. Press 55-70 and rise 45-55 gave the same result; a 60 ms
 * debounce missed light taps on the center, where the ring has its hole. */
const tkey_config_t app_tkey_config = {
    .charge_time = 3,
    .discharge_time = 8,
    .adc_sample_time = ADC_SampleTime_1Cycles5,
};

const touch_config_t app_touch_config = {
    .window_samples = 20,                       /* 20 ms at 1 kHz: one 50 Hz mains period */
    .calibration_windows = 8,                   /* 160 ms */
    .baseline_shift = 8,                        /* about 5 s time constant */
    .press_permille = APP_TOUCH_PRESS_PERMILLE, /* see the TouchKey note; recheck on the Mac (V4) */
    .release_permille = 34,
    .press_rise_permille = 50, /* a hand nearing the lid rises less: see the note */
    .proximity_permille = 10,  /* rest noise and the LED on/off shift stay below */
    .debounce_ms = 40,         /* 60 missed light taps on the center; noise stays far below */
    .rearm_ms = 100,           /* fast taps came 260 ms apart */
    .long_press_ms = 2000,
    .object_ms = 10000,
    .inverted_permille = 30,
    .inverted_ms = 1000,
    .rising_touch = false, /* a finger lowers the TouchKey reading */
};

/* Ambient light to brightness. The lux are those the sensor reads behind
 * its notch in the side wall, a small fraction of the light of the room (a
 * dim room reads under 0.01 lx there). First curve, tuned live on the first
 * board with the console command `curve`, judged by eye with a phone
 * flashlight at several distances (2026-10-21). The floor of 1.5 % is the
 * level the user chose at night with the previous curve. Refine with daily
 * use. */
/* clang-format off */
const brightness_point_t app_brightness_curve[APP_BRIGHTNESS_CURVE_POINTS] = {
    { MLUX(100),   15 },
    { MLUX(500),   95 },
    {    LUX(2),  285 },
    {    LUX(8),  550 },
    {   LUX(25),  750 },
    {  LUX(100), 1000 },
};
/* clang-format on */

/* Speed chosen by the user: a light switched on or off is followed in about
 * 1.5 s (0.4 s to confirm it, then a time constant of 0.25 s). */
const brightness_config_t app_brightness_config = {
    .curve = app_brightness_curve,
    .curve_points = APP_BRIGHTNESS_CURVE_POINTS,
    .tick_ms = 100,    /* the shortest integration of the sensor */
    .slow_divisor = 8, /* small changes: time constant about 0.75 s */
    .fast_divisor = 3, /* large ones: about 0.25 s */
    .large_q8 = 256,   /* large: a factor of 2 */
    .confirm_ms = 400, /* a hand or a shadow passes faster */
    .settled_q8 = 32,  /* the fast filter ends within 9 % of the light */
    .hold_q8 = 8,      /* about 2 %: the sensor noise never moves the lamp */
    .fallback_permille = 500,
};

/* Colors at full brightness, linear light. The orange and the warm white
 * were tuned by eye through the PLA at bring-up (the first orange, green at
 * 16384, was hard to tell from the red). */
/* clang-format off: keep the table readable */
const color_config_t app_color_config = {
    .palette = {
        [LAMP_COLOR_GREEN]      = {     0, 65535,    0 },
        [LAMP_COLOR_ORANGE]     = { 65535, 29000,    0 }, /* tuned by eye through the PLA */
        [LAMP_COLOR_RED]        = { 65535,     0,    0 },
        [LAMP_COLOR_WARM_WHITE] = { 63000, 58000, 1600 }, /* tuned by eye through the PLA */
    },
    .balance = { 65535, 65535, 65535 },
};
/* clang-format on */

/* Pulse loss, tuned by eye on warm white at bring-up (72 MHz ticks of
 * 14 ns): at 10 % its blue was a 1-tick pulse and the color turned yellow.
 * Whole ticks gave 2/3/5 for 3 % and 2/3/4 for 10 %; with dithering, a
 * blue of 4 3/16 ticks suits both. */
const uint16_t app_led_pulse_loss_ns[LED_CHANNEL_COUNT] = { 28, 42, 58 };

/* Worst-case LED currents at 100 % duty (review N2: 5.25 V, lowest
 * forward voltage). Caps: 70 mA + about 25 mA for the MCU stays within the
 * 100 mA of an unconfigured USB device; 125 mA lets any single color reach
 * 100 % and holds warm white near half power (tune after V2). */
const power_config_t app_power_config = {
    .full_scale_ma = { 86, 63, 125 },
    .unconfigured_cap_ma = 70,
    .configured_cap_ma = 125,
};

/* Self-test: colors at 50 % and the blue glow at 20 % perceived, in linear
 * light (the CIE table of color.c). The white step is the palette's warm
 * white at 50 % (63000/58000/1600 x 12071/65535): equal channels looked
 * pink. Idle TouchKey reading: about 2600 counts on the first board, 2100
 * on the final one, 4095 when saturated. */
const self_test_config_t app_self_test_config = {
    .color_ms = 1500,
    .touch_wait_ms = 10000,
    .color_linear = 12071,
    .white = { 11604, 10683, 295 },
    .wait_linear = 1959,
    .touch_idle_min = 1000,
    .touch_idle_max = 3800,
    .touch_press_permille = APP_TOUCH_PRESS_PERMILLE,
};
