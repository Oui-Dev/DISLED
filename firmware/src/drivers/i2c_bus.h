/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Blocking I2C master on I2C1. Every wait has a timeout, so a stuck bus
 * costs a few milliseconds instead of hanging the firmware, and
 * i2c_bus_recover() frees a slave that holds SDA low.
 */
#ifndef DISLED_I2C_BUS_H
#define DISLED_I2C_BUS_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    I2C_BUS_OK = 0,
    I2C_BUS_TIMEOUT,
    I2C_BUS_NACK,
    I2C_BUS_ERROR, /* arbitration lost or bus error */
} i2c_bus_status_t;

void i2c_bus_init(uint32_t speed_hz);

/* Writes `length` bytes to the 7-bit address. */
i2c_bus_status_t i2c_bus_write(uint8_t address, const uint8_t *data, size_t length);

/* Writes one register index, then reads two bytes after a repeated start
 * (the transaction shape of SMBus "read word" devices such as the
 * VEML7700). */
i2c_bus_status_t i2c_bus_read_word(uint8_t address, uint8_t reg, uint8_t out[2]);

/* Last failure, for bring-up diagnostics. Steps: 1 bus busy, 2 START,
 * 3 address (write), 4 data byte, 5 repeated START, 6 address (read),
 * 7 and 8 received bytes. */
typedef struct {
    uint8_t step;
    i2c_bus_status_t status;
    uint16_t star1;
    uint16_t star2;
    uint16_t ctlr1;
} i2c_bus_failure_t;

i2c_bus_failure_t i2c_bus_last_failure(void);

/* Failed transfers since boot (diagnostics). */
uint16_t i2c_bus_error_count(void);

/* Clocks SCL until a stuck slave releases SDA, then re-initializes. */
void i2c_bus_recover(void);

#endif /* DISLED_I2C_BUS_H */
