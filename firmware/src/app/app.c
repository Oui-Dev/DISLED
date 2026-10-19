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

/* Probes the sensor. The first readings after power-on are not valid
 * until an integration completes, so they are skipped. */
static void als_probe(app_als_t *als, uint32_t now_ms, uint32_t retry_delay_ms)
{
    als->present = veml7700_init(&app_veml7700_config);
    als->errors = 0;
    als->retry_ms = now_ms + retry_delay_ms;
    als->settle_ms = now_ms + 2u * veml7700_integration_ms();
}

static void task_als(uint32_t now_ms)
{
    app_als_t *als = &app.als;

    if (!als->present) {
        if (time_reached(now_ms, als->retry_ms)) {
            i2c_bus_recover(); /* start each attempt from a clean bus */
            als_probe(als, now_ms, APP_ALS_RETRY_MS);
            if (als->present) {
                LOG_INFO("als: sensor found");
            }
        }
        return;
    }
    if (!time_reached(now_ms, als->settle_ms)) {
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
#ifdef DISLED_BOOT_DIAG
    /* Heartbeat: the level breathes while the loop runs. */
    led_output_inputs_t breathing = inputs;
    const uint32_t phase = now_ms % 2000u;
    breathing.level_permille =
        (uint16_t)(200u + (4u * ((phase < 1000u) ? phase : 2000u - phase)) / 5u);
    led_output_update(&app.output, &breathing, now_ms, app.duty);
#else
    led_output_update(&app.output, &inputs, now_ms, app.duty);
#endif
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

#ifdef DISLED_BOOT_DIAG
/* Bring-up diagnostic: the running task is noted in a backup register, so
 * that a watchdog reset can tell which one hung. */
#define DIAG_MARK(n) (BKP->DATAR8 = (uint16_t)(n))
#define DIAG_TASK(id, fn)                  \
    static void diag_##fn(uint32_t now_ms) \
    {                                      \
        DIAG_MARK(id);                     \
        fn(now_ms);                        \
        DIAG_MARK(0);                      \
    }
DIAG_TASK(1, task_usb)
DIAG_TASK(2, task_touch)
DIAG_TASK(3, task_als)
DIAG_TASK(4, task_output)
DIAG_TASK(5, task_host)
#if DISLED_DEBUG
DIAG_TASK(6, task_console)
#endif
#define TASK_FN(fn) diag_##fn

static void diag_blink(uint16_t count)
{
    uint16_t duty[LED_CHANNEL_COUNT] = { 0, 0, 0 };
    for (uint16_t i = 0; i < 2u * count; i++) {
        /* 1/8 of full scale, in sub-ticks. */
        duty[LED_CHANNEL_RED] = (i % 2u == 0u) ? (uint16_t)(led_pwm_period() * 2u) : 0u;
        led_pwm_set(duty);
        const uint32_t start = bsp_time_ms();
        while (time_elapsed_ms(bsp_time_ms(), start) < 400u) {
        }
    }
    const uint32_t start = bsp_time_ms();
    while (time_elapsed_ms(bsp_time_ms(), start) < 1500u) {
    }
}
#else
#define DIAG_MARK(n) ((void)0)
#define TASK_FN(fn)  fn
#endif

static scheduler_task_t tasks[] = {
    { "usb", 0, TASK_FN(task_usb), 0 },
    { "touch", 0, TASK_FN(task_touch), 0 },
    { "als", APP_ALS_PERIOD_MS, TASK_FN(task_als), 0 },
    { "output", APP_OUTPUT_PERIOD_MS, TASK_FN(task_output), 0 },
    { "host", 0, TASK_FN(task_host), 0 },
#if DISLED_DEBUG
    { "console", 0, TASK_FN(task_console), 0 },
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
    app.colors = app_color_config;
    app.output_config = (led_output_config_t){
        .color = &app.colors,
        .power = &app_power_config,
        .fade_ms = APP_LED_FADE_MS,
        .pwm_period = led_pwm_period(),
    };
    const uint32_t timer_mhz = bsp_apb1_timer_clock_hz() / 1000000u;
    for (size_t i = 0; i < LED_CHANNEL_COUNT; i++) {
        app.output_config.pulse_loss[i] =
            (uint16_t)((app_led_pulse_loss_ns[i] * timer_mhz * LED_DUTY_SUBTICKS + 500u) / 1000u);
    }
    led_output_init(&app.output, &app.output_config);
#ifdef DISLED_BOOT_DIAG
    const uint16_t hung_task = BKP->DATAR8;
    DIAG_MARK(0);
    if (app.reset_cause == RESET_CAUSE_WATCHDOG && hung_task != 0u) {
        diag_blink(hung_task);
    }
#endif

    lamp_fsm_init(&app.lamp);
#ifdef DISLED_BOOT_DIAG
    /* Bring-up diagnostic: the start-up color tells why the chip last reset. */
    if (app.fault.valid) {
        app.lamp.color = LAMP_COLOR_RED;
    } else if (app.reset_cause == RESET_CAUSE_WATCHDOG) {
        app.lamp.color = LAMP_COLOR_WARM_WHITE;
    } else if (!app.crystal_ok) {
        app.lamp.color = LAMP_COLOR_ORANGE;
    }
#endif
    app.source = CHANGE_SOURCE_BOOT;
    brightness_init(&app.brightness, &app_brightness_config);

    i2c_bus_init(APP_I2C_SPEED_HZ);
    /* The sensor may still be powering up: retry soon if it is silent. */
    als_probe(&app.als, bsp_time_ms(), APP_ALS_FIRST_RETRY_MS);

    touch_detector_init(&app.touch, &app_touch_config);
    tkey_init(&app_tkey_config);
    tkey_start();

    usb_device_init(app.crystal_ok);
    app.link = usb_device_link();
    app_host_init();
#if DISLED_DEBUG
    app_console_init();
#endif

#ifndef DISLED_NO_WATCHDOG
    bsp_watchdog_start(APP_WATCHDOG_MS);
#endif

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
        /* No WFI here: on this QingKe core the CPU never woke from it (not
         * even on the 1 ms tick), which starved USB and tripped the
         * watchdog. Low-power idle comes back in phase 2, tested on a board. */
    }
}
