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
 * Closed enclosure, Windows PC on an earthed monitor hub: a finger on the
 * lid lowers the reading by 44-57 per mille; a hand approaching the lid by
 * up to 11; holding the USB plug with the other hand doubles the drop (64-91),
 * as the body then shares the board ground. The thresholds sit between the
 * hand approaching and a one-hand touch.
 *
 * A hand resting near the lid between light taps holds the signal at
 * 20-30 per mille, with the taps peaking at 40-80 on top: the rise above
 * the floor separates them. Replayed on a 33 s recording of such taps
 * (20 ms windows), it found 22 taps where the fixed thresholds merged them
 * into 17, without any false long press. */
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
    .release_permille = 16,
    .press_rise_permille = 18, /* a hand resting near the lid reads 20-30 */
    .proximity_permille = 10,  /* rest noise and the LED on/off shift stay below */
    .debounce_ms = 40,         /* 60 missed light 60 ms taps; noise stays far below */
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
 * pink. Idle TouchKey reading: about 2600 counts on the first board, 4095
 * when saturated. */
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
