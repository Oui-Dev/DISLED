/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Vishay VEML7700 ambient light sensor (I2C address 0x10).
 *
 * The sensor integrates continuously; reading it once per integration time
 * gives a fresh value each time. Lux is only used relatively (to adapt the
 * LED), so the high-lux nonlinearity correction is not applied.
 */
#ifndef DISLED_VEML7700_H
#define DISLED_VEML7700_H

#include <stdbool.h>
#include <stdint.h>

/* Register encodings (ALS_CONF_0, bits 12:11). */
typedef enum {
    VEML7700_GAIN_X1 = 0,
    VEML7700_GAIN_X2 = 1,
    VEML7700_GAIN_X1_8 = 2,
    VEML7700_GAIN_X1_4 = 3,
} veml7700_gain_t;

/* Register encodings (ALS_CONF_0, bits 9:6). */
typedef enum {
    VEML7700_IT_100MS = 0x0,
    VEML7700_IT_200MS = 0x1,
    VEML7700_IT_400MS = 0x2,
    VEML7700_IT_800MS = 0x3,
    VEML7700_IT_50MS = 0x8,
    VEML7700_IT_25MS = 0xC,
} veml7700_integration_t;

typedef struct {
    veml7700_gain_t gain;
    veml7700_integration_t integration;
} veml7700_config_t;

/* Checks the device ID and powers the sensor on. False if it does not
 * answer or is not a VEML7700. */
bool veml7700_init(const veml7700_config_t *config);

/* Reads the ALS count. False on a bus error. */
bool veml7700_read(uint16_t *als_raw);

/* Converts a count to lux x 100 for the configured gain and integration. */
uint32_t veml7700_lux_centi(uint16_t als_raw);

/* Integration time of the configuration, in ms. */
uint16_t veml7700_integration_ms(void);

#endif /* DISLED_VEML7700_H */
