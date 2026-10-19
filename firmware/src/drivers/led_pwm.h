/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * RGB LED PWM on TIM3 (channels 1..3, active high, edge aligned).
 *
 * Duties are given in sub-ticks (1/16 of a tick, see pwm_dither.h): the
 * update interrupt of every period dithers them into whole ticks. The
 * period is kept within PWM_DITHER_PERIOD_MAX so that 100 % fits 16 bits.
 *
 * The driver also offers a "sample point": TIM3 channel 4 compare, used as
 * an internal event only (its output, which would land on the touch pin
 * PB1, is never enabled). The touch driver arms it to measure at a fixed
 * phase of the PWM period, away from the LED switching edges (review N1).
 */
#ifndef DISLED_LED_PWM_H
#define DISLED_LED_PWM_H

#include <stdint.h>

#include "disled_types.h"

/* Starts the PWM at `frequency_hz` with all channels at 0 %. */
void led_pwm_init(uint32_t frequency_hz);

/* Timer ticks for 100 % duty. */
uint16_t led_pwm_period(void);

/* New duties in sub-ticks (led_channel_t order), applied from the next
 * period. */
void led_pwm_set(const uint16_t duty[LED_CHANNEL_COUNT]);

/* Sample point: `callback` runs in interrupt context once per arming, when
 * the counter reaches `phase_ticks` (0 .. period - 1). */
typedef void (*led_pwm_sample_callback_t)(void);
void led_pwm_set_sample_callback(led_pwm_sample_callback_t callback);
void led_pwm_set_sample_phase(uint16_t phase_ticks);
uint16_t led_pwm_sample_phase(void);
void led_pwm_arm_sample(void);

#endif /* DISLED_LED_PWM_H */
