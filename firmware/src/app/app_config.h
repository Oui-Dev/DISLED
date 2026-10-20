/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Every tunable of the firmware, in one place. Values marked "tune" are
 * first guesses, to be adjusted from the measurements on real boards
 * (docs/bringup.md: V1 light sensor, V2 thermal, V4 touch).
 */
#ifndef DISLED_APP_CONFIG_H
#define DISLED_APP_CONFIG_H

#include "brightness.h"
#include "color.h"
#include "drivers/tkey.h"
#include "drivers/veml7700.h"
#include "power_limiter.h"
#include "self_test.h"
#include "touch_detector.h"

/* LED PWM: above 20 kHz no audible capacitor whine. TIM3 runs at 72 MHz
 * (ADR 0009), so a period has 3600 ticks; the dithering splits each into
 * sixteenths for the very dim evening levels. */
#define APP_PWM_FREQUENCY_HZ 20000u
#define APP_LED_FADE_MS      300u
#define APP_OUTPUT_PERIOD_MS 10u

/* Light sensor: one reading per integration time. */
#define APP_ALS_PERIOD_MS      100u
#define APP_ALS_MAX_ERRORS     5u    /* consecutive failures before giving up */
#define APP_ALS_RETRY_MS       5000u /* then retry the sensor this often */
#define APP_ALS_FIRST_RETRY_MS 200u  /* sooner at start-up, in case it was still booting */
#define APP_I2C_SPEED_HZ       100000u

/* Host notifications caused by automatic brightness: at most this often. */
#define APP_AUTO_LEVEL_NOTIFY_MS 500u

/* Delay between the ACK of a RESET command and the reset itself. */
#define APP_RESET_DELAY_MS 50u

#define APP_WATCHDOG_MS 1000u

extern const tkey_config_t app_tkey_config;
extern const touch_config_t app_touch_config;
extern const veml7700_config_t app_veml7700_config;
extern const brightness_config_t app_brightness_config;
extern const color_config_t app_color_config;
extern const power_config_t app_power_config;
/* Light lost per PWM pulse, per channel (see led_output_config_t). */
extern const uint16_t app_led_pulse_loss_ns[LED_CHANNEL_COUNT];
extern const self_test_config_t app_self_test_config;

/* Touch press threshold, shared by the detector and the self-test. */
#define APP_TOUCH_PRESS_PERMILLE 28u

#endif /* DISLED_APP_CONFIG_H */
