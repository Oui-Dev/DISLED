/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "app_config.h"

#define LUX(x) ((uint32_t)(x) * 100u)

/* TouchKey charge/discharge times from the WCH example. Tune: longer
 * charge raises the reading and the sensitivity. */
const tkey_config_t app_tkey_config = {
    .charge_time = 0x10,
    .discharge_time = 0x08,
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
static const brightness_point_t brightness_curve[] = {
    { LUX(1), 20 },
    { LUX(10), 80 },
    { LUX(100), 300 },
    { LUX(1000), 800 },
    { LUX(3000), 1000 },
};

const brightness_config_t app_brightness_config = {
    .curve = brightness_curve,
    .curve_points = sizeof brightness_curve / sizeof brightness_curve[0],
    .filter_shift = 5, /* 32 readings of 100 ms: about 3 s */
    .deadband_permille = 10,
    .fallback_permille = 500,
};

/* Colors at full brightness, linear light. Tune the orange and the warm
 * white by eye through the PLA, then the per-channel balance. */
const color_config_t app_color_config = {
    .palette = {
        [LAMP_COLOR_GREEN] = { 0, 65535, 0 },
        [LAMP_COLOR_ORANGE] = { 65535, 16384, 0 },
        [LAMP_COLOR_RED] = { 65535, 0, 0 },
        [LAMP_COLOR_WARM_WHITE] = { 65535, 36000, 9000 },
    },
    .balance = { 65535, 65535, 65535 },
};

/* Worst-case LED currents at 100 % duty (review N2: 5.25 V, lowest
 * forward voltage). Caps: 70 mA + about 25 mA for the MCU stays within the
 * 100 mA of an unconfigured USB device; 125 mA lets any single color reach
 * 100 % and holds warm white near half power (tune after V2). */
const power_config_t app_power_config = {
    .full_scale_ma = { 86, 63, 125 },
    .unconfigured_cap_ma = 70,
    .configured_cap_ma = 125,
};
