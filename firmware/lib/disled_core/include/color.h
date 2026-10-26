/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Colors in linear light, 0 to 65535, proportional to the PWM duty. Levels
 * are perceived brightness (CIE 1976 L*), so that equal steps look equal.
 */
#ifndef DISLED_COLOR_H
#define DISLED_COLOR_H

#include <stdint.h>

#include "disled_types.h"

#define COLOR_LINEAR_MAX 65535u

typedef struct {
    /* Each lamp color at full brightness. */
    rgb16_t palette[LAMP_COLOR_COUNT];
} color_config_t;

/* Perceived brightness (per mille of L*) to linear light. */
uint16_t color_perceived_to_linear(uint16_t level_permille);

/* Palette color scaled to a perceived brightness level. */
rgb16_t color_compose(const color_config_t *config, lamp_color_t color, uint16_t level_permille);

/* Linear interpolation; `t` = 0 gives `from`, 65535 gives `to`. */
rgb16_t color_mix(rgb16_t from, rgb16_t to, uint16_t t);

/* Product of two fractions of 65535, rounded. */
uint16_t color_scale(uint16_t value, uint16_t factor);

#endif /* DISLED_COLOR_H */
