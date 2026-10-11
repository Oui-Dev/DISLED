/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Application wiring: drivers -> core logic -> outputs, run by the
 * cooperative scheduler.
 *
 *   TouchKey samples -> touch_detector -> lamp_fsm
 *   VEML7700 lux     -> brightness
 *   lamp + level     -> led_output (fade, power cap) -> LED PWM
 *   USB link + HID   -> app_host (commands, notifications, telemetry)
 */
#include "app.h"

#include <stddef.h>

#include "app_config.h"
#include "app_console.h"
#include "app_context.h"
#include "app_host.h"
#include "bsp/bsp.h"
#include "bsp/bsp_time.h"
#include "bsp/bsp_watchdog.h"
#include "bsp/ch32.h"
#include "diag/log.h"
#include "drivers/i2c_bus.h"
#include "drivers/led_pwm.h"
#include "drivers/tkey.h"
#include "drivers/veml7700.h"
#include "scheduler.h"
#include "time_utils.h"
#include "usb/usb_device.h"

app_context_t app;

bool app_apply_request(const lamp_request_t *request)
{
    const bool changed = lamp_fsm_apply(&app.lamp, request);
    if (changed) {
        app.source = CHANGE_SOURCE_HOST;
    }
    return changed;
}

static const char *link_name(usb_link_t link)
{
    switch (link) {
        case USB_LINK_CONFIGURED:
            return "configured";
        case USB_LINK_SUSPENDED:
            return "suspended";
        case USB_LINK_UNCONFIGURED:
        default:
            return "unconfigured";
    }
}

/* --- Tasks ----------------------------------------------------------- */

static void task_usb(uint32_t now_ms)
{
    (void)now_ms;
    usb_device_task();

    const usb_link_t link = usb_device_link();
    if (link == app.link) {
        return;
    }
    if (link == USB_LINK_SUSPENDED) {
        /* Host asleep: the LED goes dark (cap 0) and the touch is ignored. */
        tkey_stop();
    } else if (app.link == USB_LINK_SUSPENDED) {
        touch_detector_recalibrate(&app.touch);
        tkey_start();
    }
    app.link = link;
    LOG_INFO("usb: %s", link_name(link));
}

static void task_touch(uint32_t now_ms)
{
    (void)now_ms;
    uint16_t raw;
    uint32_t sample_ms;
    while (tkey_read(&raw, &sample_ms)) {
        const touch_event_t event = touch_detector_feed(&app.touch, raw, sample_ms);
        bool changed = false;
        if (event == TOUCH_EVENT_SHORT_PRESS) {
            changed = lamp_fsm_short_press(&app.lamp);
        } else if (event == TOUCH_EVENT_LONG_PRESS) {
            changed = lamp_fsm_long_press(&app.lamp);
        }
        if (changed) {
            app.source = CHANGE_SOURCE_TOUCH;
            LOG_INFO("touch: %s press, lamp %s, color %u",
                     event == TOUCH_EVENT_SHORT_PRESS ? "short" : "long",
                     app.lamp.on ? "on" : "off", (unsigned)app.lamp.color);
        }
    }
}

static void task_als(uint32_t now_ms)
{
    app_als_t *als = &app.als;

    if (!als->present) {
        if (time_reached(now_ms, als->retry_ms)) {
            als->present = veml7700_init(&app_veml7700_config);
            als->errors = 0;
            als->retry_ms = now_ms + APP_ALS_RETRY_MS;
            if (als->present) {
                LOG_INFO("als: sensor found");
            }
        }
        return;
    }

    uint16_t raw;
    if (veml7700_read(&raw)) {
        als->errors = 0;
        als->raw = raw;
        als->lux_centi = veml7700_lux_centi(raw);
        brightness_update_lux(&app.brightness, als->lux_centi);
        return;
    }
    if (++als->errors >= APP_ALS_MAX_ERRORS) {
        LOG_WARN("als: sensor lost, fixed brightness until it answers again");
        als->present = false;
        als->retry_ms = now_ms + APP_ALS_RETRY_MS;
        brightness_lux_lost(&app.brightness);
        i2c_bus_recover();
    }
}

static void task_output(uint32_t now_ms)
{
    brightness_set_mode(&app.brightness, app.lamp.brightness_mode, app.lamp.manual_level);
    const led_output_inputs_t inputs = {
        .on = app.lamp.on,
        .color = app.lamp.color,
        .level_permille = brightness_level_permille(&app.brightness),
        .link = app.link,
        .cap_override_ma = app.cap_override_ma,
    };
    led_output_update(&app.output, &inputs, now_ms, app.duty);
    led_pwm_set(app.duty);
}

static void task_host(uint32_t now_ms)
{
    app_host_task(now_ms);
}

#if DISLED_DEBUG
static void task_console(uint32_t now_ms)
{
    app_console_task(now_ms);
}
#endif

static scheduler_task_t tasks[] = {
    { "usb", 0, task_usb, 0 },
    { "touch", 0, task_touch, 0 },
    { "als", APP_ALS_PERIOD_MS, task_als, 0 },
    { "output", APP_OUTPUT_PERIOD_MS, task_output, 0 },
    { "host", 0, task_host, 0 },
#if DISLED_DEBUG
    { "console", 0, task_console, 0 },
#endif
};

#define TASK_COUNT (sizeof tasks / sizeof tasks[0])

/* --- Start-up -------------------------------------------------------- */

void app_init(const fault_record_t *fault)
{
    app.fault = *fault;
    app.reset_cause = bsp_reset_cause();
    app.crystal_ok = bsp_crystal_ok();

    /* LED first, dark: from here on only led_output drives it. */
    led_pwm_init(APP_PWM_FREQUENCY_HZ);
    app.output_config = (led_output_config_t){
        .color = &app_color_config,
        .power = &app_power_config,
        .fade_ms = APP_LED_FADE_MS,
        .pwm_period = led_pwm_period(),
    };
    led_output_init(&app.output, &app.output_config);

    lamp_fsm_init(&app.lamp);
    app.source = CHANGE_SOURCE_BOOT;
    brightness_init(&app.brightness, &app_brightness_config);

    i2c_bus_init(APP_I2C_SPEED_HZ);
    app.als.present = veml7700_init(&app_veml7700_config);
    app.als.retry_ms = bsp_time_ms() + APP_ALS_RETRY_MS;

    touch_detector_init(&app.touch, &app_touch_config);
    tkey_init(&app_tkey_config);
    tkey_start();

    usb_device_init(app.crystal_ok);
    app.link = usb_device_link();
    app_host_init();
#if DISLED_DEBUG
    app_console_init();
#endif

    bsp_watchdog_start(APP_WATCHDOG_MS);

    LOG_INFO("DISLED firmware %s, reset cause %u%s", DISLED_FW_VERSION_STRING,
             (unsigned)app.reset_cause, app.fault.valid ? ", after a FAULT" : "");
    if (!app.crystal_ok) {
        LOG_ERROR("crystal failed: USB disabled");
    }
    if (!app.als.present) {
        LOG_WARN("als: sensor not found");
    }
}

void app_run(void)
{
    scheduler_start(tasks, TASK_COUNT, bsp_time_ms());
    for (;;) {
        (void)scheduler_run(tasks, TASK_COUNT, bsp_time_ms());
        /* Every task ran in this pass: the loop is alive. */
        bsp_watchdog_feed();
        /* Sleep until the next interrupt (1 ms tick at the latest). */
        __WFI();
    }
}
