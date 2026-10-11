/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "brightness.h"

#include <stddef.h>

/* log2(x) in Q8 (x >= 1), Mitchell's approximation: exact at powers of two,
 * linear in between. The error stays below 0.09 (6 % in lux), which is
 * invisible on a brightness curve. */
static int32_t log2_q8(uint32_t x)
{
    if (x == 0u) {
        return 0;
    }
    uint32_t msb = 0;
    while ((x >> (msb + 1u)) != 0u) {
        msb++;
    }
    const uint32_t fraction = (msb >= 8u) ? ((x >> (msb - 8u)) & 0xFFu) : ((x << (8u - msb)) & 0xFFu);
    return (int32_t)((msb << 8) | fraction);
}

uint16_t brightness_curve_permille(const brightness_config_t *config, uint32_t lux_centi)
{
    const brightness_point_t *curve = config->curve;
    const size_t last = (size_t)config->curve_points - 1u;

    if (lux_centi <= curve[0].lux_centi) {
        return curve[0].level_permille;
    }
    if (lux_centi >= curve[last].lux_centi) {
        return curve[last].level_permille;
    }

    size_t i = 0;
    while (lux_centi > curve[i + 1u].lux_centi) {
        i++;
    }
    const int32_t x = log2_q8(lux_centi);
    const int32_t x0 = log2_q8(curve[i].lux_centi);
    const int32_t x1 = log2_q8(curve[i + 1u].lux_centi);
    const int32_t y0 = curve[i].level_permille;
    const int32_t y1 = curve[i + 1u].level_permille;
    if (x1 <= x0) {
        return (uint16_t)y1;
    }
    return (uint16_t)(y0 + ((y1 - y0) * (x - x0)) / (x1 - x0));
}

void brightness_init(brightness_t *b, const brightness_config_t *config)
{
    b->config = config;
    b->mode = BRIGHTNESS_MODE_AUTO;
    b->manual_percent = BRIGHTNESS_MANUAL_MAX;
    b->have_lux = false;
    b->auto_q8 = (uint32_t)config->fallback_permille << 8;
}

void brightness_set_mode(brightness_t *b, brightness_mode_t mode, uint8_t manual_percent)
{
    if (mode == BRIGHTNESS_MODE_AUTO && b->mode == BRIGHTNESS_MODE_MANUAL) {
        /* Start the automatic filter from the level in use, so that the
         * switch fades instead of jumping. */
        b->auto_q8 = (uint32_t)brightness_level_permille(b) << 8;
    }
    b->mode = mode;
    b->manual_percent = manual_percent;
}

void brightness_update_lux(brightness_t *b, uint32_t lux_centi)
{
    const brightness_config_t *cfg = b->config;
    const uint32_t target_q8 = (uint32_t)brightness_curve_permille(cfg, lux_centi) << 8;

    if (!b->have_lux) {
        /* First reading: start at the target instead of crawling to it. */
        b->have_lux = true;
        b->auto_q8 = target_q8;
        return;
    }

    const int32_t error = (int32_t)target_q8 - (int32_t)b->auto_q8;
    const int32_t deadband = (int32_t)((uint32_t)cfg->deadband_permille << 8);
    if (error <= deadband && error >= -deadband) {
        return;
    }
    const int32_t divisor = (int32_t)1 << cfg->filter_shift;
    int32_t step = error / divisor;
    if (step == 0) {
        step = (error > 0) ? 1 : -1;
    }
    b->auto_q8 = (uint32_t)((int32_t)b->auto_q8 + step);
}

void brightness_lux_lost(brightness_t *b)
{
    b->have_lux = false;
    b->auto_q8 = (uint32_t)b->config->fallback_permille << 8;
}

uint16_t brightness_level_permille(const brightness_t *b)
{
    if (b->mode == BRIGHTNESS_MODE_MANUAL) {
        return (uint16_t)(b->manual_percent * 10u);
    }
    return (uint16_t)((b->auto_q8 + 128u) >> 8);
}

uint8_t brightness_level_percent(const brightness_t *b)
{
    return (uint8_t)((brightness_level_permille(b) + 5u) / 10u);
}
