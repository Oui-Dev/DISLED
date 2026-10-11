/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Brightness level of the lamp, in per mille of perceived brightness.
 *
 * Automatic mode maps the ambient light to a level through a curve that is
 * interpolated on a logarithmic lux scale (the eye adapts logarithmically),
 * then smooths it with a slow low-pass filter and a dead band so that the
 * lamp never visibly hunts. Manual mode uses the level chosen by the host.
 */
#ifndef DISLED_BRIGHTNESS_H
#define DISLED_BRIGHTNESS_H

#include <stdbool.h>
#include <stdint.h>

#include "disled_types.h"

#define BRIGHTNESS_PERMILLE_MAX 1000u

typedef struct {
    uint32_t lux_centi;      /* ambient light, lux x 100 */
    uint16_t level_permille; /* perceived brightness, 0 to 1000 */
} brightness_point_t;

typedef struct {
    const brightness_point_t *curve; /* sorted by lux, at least 2 points */
    uint8_t curve_points;
    uint8_t filter_shift;        /* time constant: 2^shift lux updates */
    uint16_t deadband_permille;  /* target changes smaller than this are ignored */
    uint16_t fallback_permille;  /* automatic level without a lux reading */
} brightness_config_t;

typedef struct {
    const brightness_config_t *config;
    brightness_mode_t mode;
    uint8_t manual_percent;
    bool have_lux;
    uint32_t auto_q8; /* filtered automatic level, per mille << 8 */
} brightness_t;

void brightness_init(brightness_t *b, const brightness_config_t *config);

/* Applies the lamp settings. `manual_percent` is 1 to 100. */
void brightness_set_mode(brightness_t *b, brightness_mode_t mode, uint8_t manual_percent);

/* New ambient light reading, typically every 100 ms. */
void brightness_update_lux(brightness_t *b, uint32_t lux_centi);

/* The sensor stopped answering: automatic mode falls back to a fixed level. */
void brightness_lux_lost(brightness_t *b);

/* Level to apply now, 0 to 1000. */
uint16_t brightness_level_permille(const brightness_t *b);

/* Same level in percent (rounded), as reported to the host. */
uint8_t brightness_level_percent(const brightness_t *b);

/* Curve lookup without filtering. Exposed for tests and telemetry. */
uint16_t brightness_curve_permille(const brightness_config_t *config, uint32_t lux_centi);

#endif /* DISLED_BRIGHTNESS_H */
