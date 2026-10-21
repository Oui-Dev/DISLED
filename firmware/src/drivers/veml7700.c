/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "veml7700.h"

#include "bsp/board.h"
#include "i2c_bus.h"

#define REG_ALS_CONF 0x00u
#define REG_ALS      0x04u
#define REG_ID       0x07u

#define DEVICE_ID_LSB 0x81u
#define CONF_SHUTDOWN (1u << 0) /* ALS_SD */

/* ALS_CONF_0 of the current setting, shutdown bit cleared. */
static uint16_t s_conf;

/* ALS_GAIN, bits 12:11. */
static uint16_t gain_bits(uint8_t gain_eighths)
{
    switch (gain_eighths) {
        case 16:
            return 0x1u; /* x2 */
        case 8:
            return 0x0u; /* x1 */
        case 2:
            return 0x3u; /* x1/4 */
        case 1:
        default:
            return 0x2u; /* x1/8 */
    }
}

/* ALS_IT, bits 9:6. */
static uint16_t integration_bits(uint16_t integration_ms)
{
    switch (integration_ms) {
        case 25:
            return 0xCu;
        case 50:
            return 0x8u;
        case 200:
            return 0x1u;
        case 400:
            return 0x2u;
        case 800:
            return 0x3u;
        case 100:
        default:
            return 0x0u;
    }
}

/* Persistence 1, interrupt off. */
static uint16_t configuration(uint8_t gain_eighths, uint16_t integration_ms)
{
    return (uint16_t)((gain_bits(gain_eighths) << 11) | (integration_bits(integration_ms) << 6));
}

static bool write_register(uint8_t reg, uint16_t value)
{
    const uint8_t frame[3] = { reg, (uint8_t)(value & 0xFFu), (uint8_t)(value >> 8) };
    return i2c_bus_write(BOARD_VEML7700_ADDR, frame, sizeof frame) == I2C_BUS_OK;
}

static bool read_register(uint8_t reg, uint16_t *value)
{
    uint8_t bytes[2];
    if (i2c_bus_read_word(BOARD_VEML7700_ADDR, reg, bytes) != I2C_BUS_OK) {
        return false;
    }
    *value = (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
    return true;
}

bool veml7700_init(uint8_t gain_eighths, uint16_t integration_ms)
{
    uint16_t id;
    if (!read_register(REG_ID, &id) || (id & 0xFFu) != DEVICE_ID_LSB) {
        return false;
    }
    s_conf = configuration(gain_eighths, integration_ms);
    return write_register(REG_ALS_CONF, s_conf); /* shutdown bit cleared: powered on */
}

bool veml7700_configure(uint8_t gain_eighths, uint16_t integration_ms)
{
    s_conf = configuration(gain_eighths, integration_ms);
    return write_register(REG_ALS_CONF, (uint16_t)(s_conf | CONF_SHUTDOWN)) &&
           write_register(REG_ALS_CONF, s_conf);
}

bool veml7700_shutdown(void)
{
    return write_register(REG_ALS_CONF, (uint16_t)(s_conf | CONF_SHUTDOWN));
}

bool veml7700_read(uint16_t *count)
{
    return read_register(REG_ALS, count);
}
