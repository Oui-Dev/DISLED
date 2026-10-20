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
#include "bsp/bsp_power.h"
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

static void task_output(uint32_t now_ms);
static void als_start(app_als_t *als, uint32_t now_ms, uint32_t retry_delay_ms);

/* Host asleep: the LED goes dark, the touch and the light sensor stop, and
 * the core drops to the slow clock until the bus wakes up. */
static void enter_suspend(uint32_t now_ms)
{
    if (self_test_running(&app.self_test)) {
        self_test_abort(&app.self_test); /* nobody to report to */
    }
    tkey_stop();
    task_output(now_ms); /* app.link is suspended: duties drop to 0 */
    led_pwm_stop();
    if (app.als.present) {
        (void)veml7700_shutdown();
    }
    if (app.usb_suspends < UINT16_MAX) {
        app.usb_suspends++;
    }
#ifndef DISLED_NO_SLOW_SUSPEND
    usb_device_wake_irq(true);
    bsp_power_slow_clock();
#endif
}

static void leave_suspend(uint32_t now_ms)
{
    /* The USB wake-up interrupt normally restored the clock already. */
    if (!bsp_power_full_clock()) {
        LOG_ERROR("crystal failed on resume");
    }
    usb_device_wake_irq(false);
    led_pwm_start();
    if (app.als.present) {
        als_start(&app.als, now_ms, APP_ALS_FIRST_RETRY_MS); /* wakes the sensor */
    }
    touch_detector_recalibrate(&app.touch);
    tkey_start();
}

static void task_usb(uint32_t now_ms)
{
    usb_device_task();

    const usb_link_t link = usb_device_link();
    if (link == app.link) {
        return;
    }
    const usb_link_t previous = app.link;
    app.link = link;
    if (link == USB_LINK_SUSPENDED) {
        enter_suspend(now_ms);
    } else if (previous == USB_LINK_SUSPENDED) {
        leave_suspend(now_ms);
    }
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
        if (self_test_running(&app.self_test)) {
            /* The self-test watches the touch signal itself. */
        } else if (event == TOUCH_EVENT_SHORT_PRESS) {
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

/* Starts (or wakes) the sensor. The first readings after power-on are not
 * valid until an integration completes, so they are skipped. */
static void als_start(app_als_t *als, uint32_t now_ms, uint32_t retry_delay_ms)
{
    als->present = veml7700_init(&app_veml7700_config);
    als->errors = 0;
    als->retry_ms = now_ms + retry_delay_ms;
    als->settle_ms = now_ms + 2u * veml7700_integration_ms();
}

/* A detection attempt (start-up, sensor lost): counted for the diagnostics. */
static void als_probe(app_als_t *als, uint32_t now_ms, uint32_t retry_delay_ms)
{
    if (als->probes < UINT16_MAX) {
        als->probes++;
    }
    als_start(als, now_ms, retry_delay_ms);
}

static void task_als(uint32_t now_ms)
{
    app_als_t *als = &app.als;

    if (app.link == USB_LINK_SUSPENDED) {
        return; /* sensor asleep, I2C idle on the slow clock */
    }
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

/* Feeds the running self-test; returns the color it wants shown. */
static rgb16_t run_self_test(uint32_t now_ms)
{
    const self_test_inputs_t measurements = {
        .light_present = app.als.present,
        .lux_centi = app.als.lux_centi,
        .touch_ready = touch_detector_state(&app.touch) != TOUCH_STATE_CALIBRATING,
        .touch_baseline = touch_detector_baseline(&app.touch),
        .touch_delta_permille = touch_detector_delta_permille(&app.touch),
    };
    if (self_test_update(&app.self_test, now_ms, &measurements)) {
        if ((app.self_test.results & SELF_TEST_TOUCH_PRESSED) != 0u) {
            touch_detector_consume(&app.touch); /* the finger is still down */
        }
        app.self_test_report = true;
        LOG_INFO("self-test: results 0x%02x", (unsigned)app.self_test.results);
    }
    return self_test_color(&app.self_test, now_ms);
}

bool app_start_self_test(uint8_t seq)
{
    if (self_test_running(&app.self_test)) {
        return false;
    }
    app.self_test_seq = seq;
    app.self_test_report = false;
    self_test_start(&app.self_test, bsp_time_ms());
    LOG_INFO("self-test: started");
    return true;
}

static void task_output(uint32_t now_ms)
{
    brightness_set_mode(&app.brightness, app.lamp.brightness_mode, app.lamp.manual_level);
    const bool testing = self_test_running(&app.self_test);
    const led_output_inputs_t inputs = {
        .on = app.lamp.on,
        .color = app.lamp.color,
        .level_permille = brightness_level_permille(&app.brightness),
        .link = app.link,
        .cap_override_ma = app.cap_override_ma,
        .show_override = testing,
        .override_rgb = testing ? run_self_test(now_ms) : (rgb16_t){ 0, 0, 0 },
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

static uint16_t saturate_u16(uint32_t value)
{
    return (value > UINT16_MAX) ? (uint16_t)UINT16_MAX : (uint16_t)value;
}

void app_diagnostics(proto_diagnostics_t *diagnostics)
{
    const fault_stats_t stats = bsp_fault_stats();
    *diagnostics = (proto_diagnostics_t){
        .uptime_ms = bsp_time_ms(),
        .boots = stats.boots,
        .watchdog_resets = stats.watchdog_resets,
        .faults = stats.faults,
        .fault_mcause = stats.last.mcause,
        .fault_mepc = stats.last.mepc,
        .fault_mtval = stats.last.mtval,
        .stack_free_min = bsp_stack_free_min(),
        .i2c_errors = i2c_bus_error_count(),
        .light_sensor_probes = app.als.probes,
        .touch_samples_lost = saturate_u16(tkey_overruns()),
        .touch_recalibrations = touch_detector_recalibrations(&app.touch),
        .usb_suspends = app.usb_suspends,
        .auto_bias = brightness_auto_bias(&app.brightness),
        .flags = (uint8_t)((app.als.present ? PROTO_DIAG_FLAG_LIGHT_SENSOR : 0u) |
                           (app.crystal_ok ? PROTO_DIAG_FLAG_CRYSTAL : 0u)),
    };
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
    { "usb", 0, TASK_FN(task_usb), 0, 0 },
    { "touch", 0, TASK_FN(task_touch), 0, 0 },
    { "als", APP_ALS_PERIOD_MS, TASK_FN(task_als), 0, 0 },
    { "output", APP_OUTPUT_PERIOD_MS, TASK_FN(task_output), 0, 0 },
    { "host", 0, TASK_FN(task_host), 0, 0 },
#if DISLED_DEBUG
    { "console", 0, TASK_FN(task_console), 0, 0 },
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
    self_test_init(&app.self_test, &app_self_test_config);

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
        (void)scheduler_run_timed(tasks, TASK_COUNT, bsp_time_ms(), bsp_time_us);
        /* Every task ran in this pass: the loop is alive. */
        bsp_watchdog_feed();
        /* No WFI: in this loop the core never woke up again (ADR 0011). */
    }
}

const scheduler_task_t *app_tasks(size_t *count)
{
    *count = TASK_COUNT;
    return tasks;
}
