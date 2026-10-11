/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "led_output.h"

#include "time_utils.h"

static const rgb16_t black = { 0, 0, 0 };

static bool same_color(rgb16_t a, rgb16_t b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

void led_output_init(led_output_t *out, const led_output_config_t *config)
{
    out->config = config;
    out->from = black;
    out->target = black;
    out->shown = black;
    out->fade_start_ms = 0;
    out->fading = false;
    out->cap_ma = 0;
    out->estimate_ma_x10 = 0;
    for (unsigned i = 0; i < LED_CHANNEL_COUNT; i++) {
        out->duty[i] = 0;
    }
}

static rgb16_t target_for(const led_output_t *out, const led_output_inputs_t *in)
{
    if (!in->on || in->link == USB_LINK_SUSPENDED) {
        return black;
    }
    return color_compose(out->config->color, in->color, in->level_permille);
}

static uint16_t to_duty(uint16_t linear, uint16_t period)
{
    return (uint16_t)(((uint32_t)linear * period + COLOR_LINEAR_MAX / 2u) / COLOR_LINEAR_MAX);
}

void led_output_update(led_output_t *out, const led_output_inputs_t *in, uint32_t now_ms,
                       uint16_t duty[LED_CHANNEL_COUNT])
{
    const led_output_config_t *cfg = out->config;
    const rgb16_t target = target_for(out, in);

    if (!same_color(target, out->target)) {
        out->from = out->shown;
        out->target = target;
        out->fade_start_ms = now_ms;
        out->fading = true;
    }

    if (out->fading) {
        const uint32_t elapsed = time_elapsed_ms(now_ms, out->fade_start_ms);
        if (cfg->fade_ms == 0u || elapsed >= cfg->fade_ms) {
            out->shown = out->target;
            out->fading = false;
        } else {
            const uint16_t t = (uint16_t)((elapsed * COLOR_LINEAR_MAX) / cfg->fade_ms);
            out->shown = color_mix(out->from, out->target, t);
        }
    }

    out->cap_ma = (in->cap_override_ma != 0u) ? in->cap_override_ma : power_cap_ma(cfg->power, in->link);
    if (in->link == USB_LINK_SUSPENDED) {
        out->cap_ma = 0;
    }
    const rgb16_t limited = power_limit(cfg->power, out->shown, out->cap_ma);
    out->estimate_ma_x10 = power_estimate_ma_x10(cfg->power, limited);

    out->duty[LED_CHANNEL_RED] = to_duty(limited.r, cfg->pwm_period);
    out->duty[LED_CHANNEL_GREEN] = to_duty(limited.g, cfg->pwm_period);
    out->duty[LED_CHANNEL_BLUE] = to_duty(limited.b, cfg->pwm_period);
    for (unsigned i = 0; i < LED_CHANNEL_COUNT; i++) {
        duty[i] = out->duty[i];
    }
}

uint32_t led_output_estimate_ma_x10(const led_output_t *out)
{
    return out->estimate_ma_x10;
}

uint16_t led_output_cap_ma(const led_output_t *out)
{
    return out->cap_ma;
}
