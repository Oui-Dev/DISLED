/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "app_config.h"

#include "bsp/ch32.h"

#define LUX(x) ((uint32_t)(x) * 100u)

/* TouchKey timing, measured on the first board (bring-up): the ADC sample
 * time sets the range, the charge time the position in it. With 1.5 cycles
 * and charge 3 the reading sits mid-scale (about 2600 of 4095, noise about
 * +-6 per mille) and a finger on the bare electrode lowers it by about
 * 100 per mille. The WCH example values (charge 16, 7.5 cycles) saturate. */
const tkey_config_t app_tkey_config = {
    .charge_time = 3,
    .discharge_time = 8,
    .adc_sample_time = ADC_SampleTime_1Cycles5,
};

const touch_config_t app_touch_config = {
    .window_samples = 20,     /* 20 ms at 1 kHz: one 50 Hz mains period */
    .calibration_windows = 8, /* 160 ms */
    .baseline_shift = 8,      /* about 5 s time constant */
    .press_permille = 40,     /* tune (V4) */
    .release_permille = 25,   /* tune (V4) */
    .debounce_ms = 60,
    .long_press_ms = 2000,
    .object_ms = 10000,
    .inverted_permille = 30,
    .inverted_ms = 1000,
    .rising_touch = false, /* a finger lowers the TouchKey reading */
};

/* Gain 1/8 and 100 ms: 0.54 lx per count, saturating near 35 klx. */
const veml7700_config_t app_veml7700_config = {
    .gain = VEML7700_GAIN_X1_8,
    .integration = VEML7700_IT_100MS,
};

/* Ambient light to brightness. Tune (V1): the review expects 30-100 %
 * during the day and 2-20 % in the evening. */
/* clang-format off */
static const brightness_point_t brightness_curve[] = {
    {    LUX(1),   30 }, /* raised x1.5 at bring-up: too dim through the PLA */
    {   LUX(10),  120 },
    {  LUX(100),  300 },
    { LUX(1000),  800 },
    { LUX(3000), 1000 },
};
/* clang-format on */

const brightness_config_t app_brightness_config = {
    .curve = brightness_curve,
    .curve_points = sizeof brightness_curve / sizeof brightness_curve[0],
    .filter_shift = 5, /* 32 readings of 100 ms: about 3 s */
    .deadband_permille = 10,
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

/* Worst-case LED currents at 100 % duty (review N2: 5.25 V, lowest
 * forward voltage). Caps: 70 mA + about 25 mA for the MCU stays within the
 * 100 mA of an unconfigured USB device; 125 mA lets any single color reach
 * 100 % and holds warm white near half power (tune after V2). */
const power_config_t app_power_config = {
    .full_scale_ma = { 86, 63, 125 },
    .unconfigured_cap_ma = 70,
    .configured_cap_ma = 125,
};
