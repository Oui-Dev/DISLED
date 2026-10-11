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

/* Resolution at gain x2 and 800 ms, in micro-lux per count (VEML7700
 * datasheet; to be confirmed against the revision in use). Other settings
 * scale it by (2 / gain) * (800 ms / integration time). */
#define RESOLUTION_ULX_X2_800MS 4200u

static veml7700_config_t s_config;

static uint8_t gain_eighths(veml7700_gain_t gain)
{
    switch (gain) {
        case VEML7700_GAIN_X2:
            return 16;
        case VEML7700_GAIN_X1:
            return 8;
        case VEML7700_GAIN_X1_4:
            return 2;
        case VEML7700_GAIN_X1_8:
        default:
            return 1;
    }
}

uint16_t veml7700_integration_ms(void)
{
    switch (s_config.integration) {
        case VEML7700_IT_25MS:
            return 25;
        case VEML7700_IT_50MS:
            return 50;
        case VEML7700_IT_200MS:
            return 200;
        case VEML7700_IT_400MS:
            return 400;
        case VEML7700_IT_800MS:
            return 800;
        case VEML7700_IT_100MS:
        default:
            return 100;
    }
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

bool veml7700_init(const veml7700_config_t *config)
{
    s_config = *config;

    uint16_t id;
    if (!read_register(REG_ID, &id) || (id & 0xFFu) != DEVICE_ID_LSB) {
        return false;
    }
    /* Persistence 1, interrupt off, shutdown bit cleared (powered on). */
    const uint16_t conf = (uint16_t)(((uint16_t)config->gain << 11) |
                                     ((uint16_t)config->integration << 6));
    return write_register(REG_ALS_CONF, conf);
}

bool veml7700_read(uint16_t *als_raw)
{
    return read_register(REG_ALS, als_raw);
}

/* lux x 100 = raw * resolution(uLx) * (16 / eighths) * (800 / it) / 10^4
 *           = raw * SCALE / (eighths * it), SCALE = resolution * 1.28. */
#define LUX_CENTI_SCALE ((RESOLUTION_ULX_X2_800MS * 16u * 800u) / 10000u)
_Static_assert((RESOLUTION_ULX_X2_800MS * 16u * 800u) % 10000u == 0u, "exact scale");
_Static_assert(65535ull * LUX_CENTI_SCALE < (1ull << 32), "32-bit product");

uint32_t veml7700_lux_centi(uint16_t als_raw)
{
    const uint32_t denominator = (uint32_t)gain_eighths(s_config.gain) * veml7700_integration_ms();
    return ((uint32_t)als_raw * LUX_CENTI_SCALE + denominator / 2u) / denominator;
}
