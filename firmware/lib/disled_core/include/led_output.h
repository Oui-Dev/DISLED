/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * The single path from lamp settings to PWM duties:
 *
 *   palette -> balance -> brightness -> fade -> power limit -> duty
 *
 * Nothing else may drive the LED, so the power limit always applies. Fades
 * run in linear light; a target change during a fade restarts it from the
 * color currently shown, so transitions never jump.
 */
#ifndef DISLED_LED_OUTPUT_H
#define DISLED_LED_OUTPUT_H

#include <stdbool.h>
#include <stdint.h>

#include "color.h"
#include "disled_types.h"
#include "power_limiter.h"

typedef struct {
    const color_config_t *color;
    const power_config_t *power;
    uint16_t fade_ms;
    uint16_t pwm_period; /* timer ticks for 100 % duty */
    /* Ticks of each pulse that give no light (driver edges), per channel.
     * They are added to any lit channel, so that very short pulses keep
     * their share of the color: at low levels the blue of warm white is a
     * few ticks long and would otherwise vanish (seen at bring-up). */
    uint16_t pulse_loss[LED_CHANNEL_COUNT];
} led_output_config_t;

typedef struct {
    bool on;
    lamp_color_t color;
    uint16_t level_permille;
    usb_link_t link;
    /* Debug override of the current cap (0 = use the cap of the link). */
    uint16_t cap_override_ma;
} led_output_inputs_t;

typedef struct {
    const led_output_config_t *config;
    rgb16_t from;
    rgb16_t target;
    rgb16_t shown; /* before the power limit */
    uint32_t fade_start_ms;
    bool fading;
    uint16_t cap_ma;
    uint32_t estimate_ma_x10;
    uint16_t duty[LED_CHANNEL_COUNT];
} led_output_t;

/* Starts dark. */
void led_output_init(led_output_t *out, const led_output_config_t *config);

/* Recomputes the output for `now_ms` and writes the PWM duties (ticks, one
 * per channel, in led_channel_t order). Call it at a steady rate, e.g.
 * 100 Hz, for smooth fades. */
void led_output_update(led_output_t *out, const led_output_inputs_t *in, uint32_t now_ms,
                       uint16_t duty[LED_CHANNEL_COUNT]);

/* Estimated LED current after the limit, in tenths of mA. */
uint32_t led_output_estimate_ma_x10(const led_output_t *out);

/* Cap applied by the last update, in mA. */
uint16_t led_output_cap_ma(const led_output_t *out);

#endif /* DISLED_LED_OUTPUT_H */
