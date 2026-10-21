/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Vishay VEML7700 ambient light sensor (I2C address 0x10).
 *
 * The sensor integrates continuously; reading it once per integration time
 * gives a fresh count each time. The application picks the gain and the
 * integration time (lib/disled_core/als_range.h) and converts the counts.
 */
#ifndef DISLED_VEML7700_H
#define DISLED_VEML7700_H

#include <stdbool.h>
#include <stdint.h>

/* Checks the device ID and powers the sensor on with a gain (in eighths:
 * 1, 2, 8 or 16) and an integration time (25 to 800 ms). False if it does
 * not answer or is not a VEML7700. */
bool veml7700_init(uint8_t gain_eighths, uint16_t integration_ms);

/* Changes the gain and the integration time as the application note does:
 * shut down, new setting, power on. The first count of the new setting is
 * ready one integration time later. False on a bus error. */
bool veml7700_configure(uint8_t gain_eighths, uint16_t integration_ms);

/* Reads the ALS count. False on a bus error. */
bool veml7700_read(uint16_t *count);

/* Puts the sensor to sleep (about 0.5 uA); veml7700_init() wakes it. */
bool veml7700_shutdown(void);

#endif /* DISLED_VEML7700_H */
