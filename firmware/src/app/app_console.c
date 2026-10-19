/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "app_console.h"

#if DISLED_DEBUG

#include <stdbool.h>
#include <string.h>

#include "app_context.h"
#include "bsp/board.h"
#include "bsp/bsp.h"
#include "bsp/bsp_uid.h"
#include "diag/console.h"
#include "diag/log.h"
#include "drivers/led_pwm.h"
#include "drivers/i2c_bus.h"
#include "drivers/tkey.h"
#include "time_utils.h"

static uint32_t s_watch_period_ms;
static uint32_t s_watch_next_ms;

static const char *const link_names[] = { "unconfigured", "configured", "suspended" };
static const char *const color_names[] = { "green", "orange", "red", "warm-white" };

/* Parses a decimal number; false if `text` is not one. */
static bool parse_u32(const char *text, uint32_t *value)
{
    uint32_t result = 0;
    if (*text == '\0') {
        return false;
    }
    for (; *text != '\0'; text++) {
        if (*text < '0' || *text > '9' || result > 100000000u) {
            return false;
        }
        result = result * 10u + (uint32_t)(*text - '0');
    }
    *value = result;
    return true;
}

static void cmd_info(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    char uid[BSP_UID_HEX_CHARS + 1u];
    bsp_uid_hex(uid);
    uid[BSP_UID_HEX_CHARS] = '\0';
    console_printf("firmware %s, board rev %u, uid %s\r\n", DISLED_FW_VERSION_STRING,
                   (unsigned)BOARD_REVISION, uid);
    console_printf("reset cause %u, crystal %s\r\n", (unsigned)app.reset_cause,
                   app.crystal_ok ? "ok" : "FAILED");
    if (app.fault.valid) {
        console_printf("last fault: mcause 0x%08lx mepc 0x%08lx mtval 0x%08lx\r\n",
                       (unsigned long)app.fault.mcause, (unsigned long)app.fault.mepc,
                       (unsigned long)app.fault.mtval);
    }
    console_printf("touch: %lu samples lost, %lu timeouts; light sensor %s\r\n",
                   (unsigned long)tkey_overruns(), (unsigned long)tkey_timeouts(),
                   app.als.present ? "present" : "ABSENT");
}

static void cmd_state(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    console_printf("lamp %s, %s, %s, manual %u%%, applied %u per mille\r\n",
                   app.lamp.on ? "on" : "off", color_names[app.lamp.color],
                   app.lamp.brightness_mode == BRIGHTNESS_MODE_AUTO ? "auto" : "manual",
                   (unsigned)app.lamp.manual_level,
                   (unsigned)brightness_level_permille(&app.brightness));
    const uint32_t estimate = led_output_estimate_ma_x10(&app.output);
    console_printf("usb %s, LED %lu.%lu mA of %u mA, duty %u/%u/%u of %u\r\n", link_names[app.link],
                   (unsigned long)(estimate / 10u), (unsigned long)(estimate % 10u),
                   (unsigned)led_output_cap_ma(&app.output), (unsigned)app.duty[LED_CHANNEL_RED],
                   (unsigned)app.duty[LED_CHANNEL_GREEN], (unsigned)app.duty[LED_CHANNEL_BLUE],
                   (unsigned)led_pwm_period());
}

static void cmd_set(int argc, char **argv)
{
    lamp_request_t request = { 0 };
    uint32_t value = 0;
    if (argc == 2 && strcmp(argv[1], "on") == 0) {
        request.fields = LAMP_FIELD_POWER;
        request.on = true;
    } else if (argc == 2 && strcmp(argv[1], "off") == 0) {
        request.fields = LAMP_FIELD_POWER;
        request.on = false;
    } else if (argc == 2 && strcmp(argv[1], "auto") == 0) {
        request.fields = LAMP_FIELD_BRIGHTNESS_MODE;
        request.brightness_mode = BRIGHTNESS_MODE_AUTO;
    } else if (argc == 3 && strcmp(argv[1], "color") == 0 && parse_u32(argv[2], &value)) {
        request.fields = LAMP_FIELD_COLOR;
        request.color = (lamp_color_t)value;
    } else if (argc == 3 && strcmp(argv[1], "level") == 0 && parse_u32(argv[2], &value)) {
        request.fields = LAMP_FIELD_BRIGHTNESS_MODE | LAMP_FIELD_MANUAL_LEVEL;
        request.brightness_mode = BRIGHTNESS_MODE_MANUAL;
        request.manual_level = (uint8_t)((value > 255u) ? 255u : value);
    } else {
        console_printf("usage: set on|off|auto|color <0-3>|level <1-100>\r\n");
        return;
    }
    if (!lamp_request_is_valid(&request)) {
        console_printf("invalid value\r\n");
        return;
    }
    (void)app_apply_request(&request);
    cmd_state(0, NULL);
}

static void cmd_watch(int argc, char **argv)
{
    uint32_t period = 0;
    if (argc == 2 && strcmp(argv[1], "off") == 0) {
        s_watch_period_ms = 0;
        return;
    }
    if (argc != 2 || !parse_u32(argv[1], &period) || period < 20u) {
        console_printf("usage: watch <period ms, >= 20>|off\r\n");
        return;
    }
    s_watch_period_ms = period;
    s_watch_next_ms = 0;
    console_printf("raw value baseline delta(permille) state | lux | mA\r\n");
}

static void cmd_touch(int argc, char **argv)
{
    uint32_t phase = 0;
    if (argc == 2 && strcmp(argv[1], "dither") == 0) {
        tkey_set_dither(true);
    } else if (argc == 3 && strcmp(argv[1], "phase") == 0 && parse_u32(argv[2], &phase) &&
               phase < led_pwm_period()) {
        tkey_set_phase((uint16_t)phase);
    } else if (argc == 2 && strcmp(argv[1], "recal") == 0) {
        touch_detector_recalibrate(&app.touch);
    } else {
        console_printf("usage: touch phase <0-%u>|dither|recal\r\n",
                       (unsigned)(led_pwm_period() - 1u));
        return;
    }
    console_printf("sample phase %u ticks\r\n", (unsigned)led_pwm_sample_phase());
}

static void cmd_cap(int argc, char **argv)
{
    uint32_t cap = 0;
    if (argc == 2 && strcmp(argv[1], "off") == 0) {
        app.cap_override_ma = 0;
    } else if (argc == 2 && parse_u32(argv[1], &cap) && cap > 0u && cap <= 300u) {
        app.cap_override_ma = (uint16_t)cap;
        console_printf("WARNING: LED cap forced to %lu mA (thermal test only)\r\n",
                       (unsigned long)cap);
    } else {
        console_printf("usage: cap <1-300 mA>|off\r\n");
        return;
    }
    cmd_state(0, NULL);
}

static const char *const i2c_status_names[] = { "ok", "timeout", "nack", "bus error" };

/* Parses a hexadecimal byte ("10" or "0x10"). */
static bool parse_hex8(const char *text, uint8_t *value)
{
    if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        text += 2;
    }
    uint32_t result = 0;
    if (*text == '\0') {
        return false;
    }
    for (; *text != '\0'; text++) {
        const char c = *text;
        uint32_t digit;
        if (c >= '0' && c <= '9') {
            digit = (uint32_t)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            digit = (uint32_t)(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            digit = (uint32_t)(c - 'A' + 10);
        } else {
            return false;
        }
        result = result * 16u + digit;
        if (result > 0xFFu) {
            return false;
        }
    }
    *value = (uint8_t)result;
    return true;
}

static void cmd_i2c(int argc, char **argv)
{
    uint8_t address = 0;
    uint8_t reg = 0;
    if (argc == 2 && strcmp(argv[1], "scan") == 0) {
        for (uint8_t a = 0x08; a < 0x78; a++) {
            const i2c_bus_status_t status = i2c_bus_write(a, NULL, 0);
            if (status == I2C_BUS_OK) {
                console_printf("0x%02x answers\r\n", (unsigned)a);
            } else if (status != I2C_BUS_NACK) {
                console_printf("0x%02x: %s\r\n", (unsigned)a, i2c_status_names[status]);
            }
        }
        console_printf("scan done\r\n");
    } else if (argc == 3 && parse_hex8(argv[1], &address) && parse_hex8(argv[2], &reg)) {
        uint8_t bytes[2] = { 0, 0 };
        const i2c_bus_status_t status = i2c_bus_read_word(address, reg, bytes);
        console_printf("0x%02x reg 0x%02x: %s, 0x%02x%02x\r\n", (unsigned)address, (unsigned)reg,
                       i2c_status_names[status], (unsigned)bytes[1], (unsigned)bytes[0]);
        if (status != I2C_BUS_OK) {
            const i2c_bus_failure_t f = i2c_bus_last_failure();
            console_printf("failed at step %u: STAR1 0x%04x STAR2 0x%04x CTLR1 0x%04x\r\n",
                           (unsigned)f.step, (unsigned)f.star1, (unsigned)f.star2, (unsigned)f.ctlr1);
        }
    } else if (argc == 2 && strcmp(argv[1], "recover") == 0) {
        i2c_bus_recover();
        console_printf("bus recovered\r\n");
    } else {
        console_printf("usage: i2c scan|recover|<addr hex> <reg hex>\r\n");
    }
}

static void cmd_log(int argc, char **argv)
{
    uint32_t level = 0;
    if (argc != 2 || !parse_u32(argv[1], &level) || level > LOG_LEVEL_DEBUG) {
        console_printf("usage: log <0 error .. 3 debug>\r\n");
        return;
    }
    log_set_level((log_level_t)level);
}

static void cmd_reset(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    bsp_system_reset();
}

static const console_command_t commands[] = {
    { "info", "", "firmware, chip and fault information", cmd_info },
    { "state", "", "lamp, USB and LED output", cmd_state },
    { "set", "<what> [value]", "change the lamp like the host does", cmd_set },
    { "watch", "<ms>|off", "print touch, light and current periodically", cmd_watch },
    { "touch", "phase|dither|recal", "touch sampling experiments (V4)", cmd_touch },
    { "cap", "<mA>|off", "override the LED current cap (V2 only)", cmd_cap },
    { "i2c", "scan|<addr> <reg>", "probe the I2C bus (light sensor)", cmd_i2c },
    { "log", "<0-3>", "log level", cmd_log },
    { "reset", "", "restart the firmware", cmd_reset },
};

void app_console_init(void)
{
    console_init(commands, sizeof commands / sizeof commands[0]);
}

void app_console_task(uint32_t now_ms)
{
    console_task();
    if (s_watch_period_ms == 0u || !time_reached(now_ms, s_watch_next_ms)) {
        return;
    }
    s_watch_next_ms = now_ms + s_watch_period_ms;
    const uint32_t lux = app.als.lux_centi;
    const uint32_t estimate = led_output_estimate_ma_x10(&app.output);
    console_printf(
        "%u %u %u %d %u | %lu.%02lu | %lu.%lu\r\n", (unsigned)touch_detector_raw(&app.touch),
        (unsigned)touch_detector_value(&app.touch), (unsigned)touch_detector_baseline(&app.touch),
        (int)touch_detector_delta_permille(&app.touch), (unsigned)touch_detector_state(&app.touch),
        (unsigned long)(lux / 100u), (unsigned long)(lux % 100u), (unsigned long)(estimate / 10u),
        (unsigned long)(estimate % 10u));
}

#endif
