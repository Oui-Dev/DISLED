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
#include "drivers/tkey.h"
#include "time_utils.h"

static uint32_t s_watch_period_ms;
static uint32_t s_watch_next_ms;

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

/* Prints sub-ticks as ticks with two decimals. */
static void print_subticks(uint16_t subticks)
{
    console_printf("%u.%02u", (unsigned)(subticks / LED_DUTY_SUBTICKS),
                   (unsigned)(((subticks % LED_DUTY_SUBTICKS) * 100u) / LED_DUTY_SUBTICKS));
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
    console_printf("usb %s, LED %lu.%lu mA of %u mA, duty ", app_link_name(app.link),
                   (unsigned long)(estimate / 10u), (unsigned long)(estimate % 10u),
                   (unsigned)led_output_cap_ma(&app.output));
    for (size_t i = 0; i < LED_CHANNEL_COUNT; i++) {
        print_subticks(app.duty[i]);
        console_printf(i + 1u < LED_CHANNEL_COUNT ? "/" : "");
    }
    console_printf(" of %u\r\n", (unsigned)led_pwm_period());
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
    } else if (argc == 3 && strcmp(argv[1], "bias") == 0 &&
               parse_u32(argv[2] + (argv[2][0] == '-' ? 1 : 0), &value) &&
               value <= (uint32_t)BRIGHTNESS_BIAS_MAX) {
        const int32_t bias = (argv[2][0] == '-') ? -(int32_t)value : (int32_t)value;
        brightness_set_auto_bias(&app.brightness, (int8_t)bias);
        cmd_state(0, NULL);
        return;
    } else {
        console_printf("usage: set on|off|auto|color <0-3>|level <1-100>|bias <-50..50>\r\n");
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
    console_printf("raw value baseline delta(permille) state | lux count level | mA\r\n");
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

static void cmd_log(int argc, char **argv)
{
    uint32_t level = 0;
    if (argc != 2 || !parse_u32(argv[1], &level) || level > LOG_LEVEL_INFO) {
        console_printf("usage: log <0 error .. 2 info>\r\n");
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

static void cmd_diag(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    proto_diagnostics_t d;
    app_diagnostics(&d);
    console_printf("since power-up: %u boots, %u watchdog resets, %u faults\r\n", (unsigned)d.boots,
                   (unsigned)d.watchdog_resets, (unsigned)d.faults);
    if (d.faults != 0u) {
        console_printf("last fault: mcause %08lx mepc %08lx mtval %08lx\r\n",
                       (unsigned long)d.fault_mcause, (unsigned long)d.fault_mepc,
                       (unsigned long)d.fault_mtval);
    }
    console_printf("since boot: stack free %u B, %u I2C errors, %u light sensor probes\r\n",
                   (unsigned)d.stack_free_min, (unsigned)d.i2c_errors,
                   (unsigned)d.light_sensor_probes);
    console_printf("touch: %u samples lost, %u recalibrations; %u USB suspends; bias %d%%\r\n",
                   (unsigned)d.touch_samples_lost, (unsigned)d.touch_recalibrations,
                   (unsigned)d.usb_suspends, (int)d.auto_bias);
}

static void cmd_selftest(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    console_printf(app_start_self_test(0) ? "self-test started: watch the colors, then touch\r\n"
                                          : "a self-test is already running\r\n");
}

static const console_command_t commands[] = {
    { "info", "", "firmware, chip and fault information", cmd_info },
    { "state", "", "lamp, USB and LED output", cmd_state },
    { "set", "<what> [value]", "change the lamp like the host does", cmd_set },
    { "watch", "<ms>|off", "print touch, light and current periodically", cmd_watch },
    { "cap", "<mA>|off", "override the LED current cap (thermal test only)", cmd_cap },
    { "diag", "", "diagnostics counters (as sent to the host)", cmd_diag },
    { "selftest", "", "run the production self-test", cmd_selftest },
    { "log", "<0-2>", "log level", cmd_log },
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
    const uint32_t lux = app.als.lux_milli;
    const uint32_t estimate = led_output_estimate_ma_x10(&app.output);
    console_printf(
        "%u %u %u %d %u | %lu.%03lu %u %u | %lu.%lu\r\n", (unsigned)touch_detector_raw(&app.touch),
        (unsigned)touch_detector_value(&app.touch), (unsigned)touch_detector_baseline(&app.touch),
        (int)touch_detector_delta_permille(&app.touch), (unsigned)touch_detector_state(&app.touch),
        (unsigned long)(lux / 1000u), (unsigned long)(lux % 1000u), (unsigned)app.als.count,
        (unsigned)brightness_level_permille(&app.brightness), (unsigned long)(estimate / 10u),
        (unsigned long)(estimate % 10u));
}

#endif
