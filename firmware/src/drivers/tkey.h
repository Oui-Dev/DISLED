/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * TouchKey sampling, ADC1 in touch mode on PB1. Every millisecond the SysTick
 * hook arms the PWM sample point, the measurement runs in the TIM3 interrupt,
 * and a small queue takes the samples to the main loop.
 */
#ifndef DISLED_TKEY_H
#define DISLED_TKEY_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t charge_time;     /* electrode charge time, ADC clock cycles */
    uint8_t discharge_time;  /* discharge time, ADC clock cycles */
    uint8_t adc_sample_time; /* ADC_SampleTime_* of the SDK: sets the reading range */
} tkey_config_t;

/* Sets up the ADC in touch mode. Requires led_pwm_init() first. */
void tkey_init(const tkey_config_t *config);

/* Starts the 1 kHz sampling (installs the SysTick hook). */
void tkey_start(void);

/* Stops sampling, for instance while the USB is suspended. */
void tkey_stop(void);

/* Pops the oldest sample. Returns false when the queue is empty. */
bool tkey_read(uint16_t *sample, uint32_t *time_ms);

/* Samples lost because the main loop did not read them in time. */
uint32_t tkey_overruns(void);

/* Measurements that never completed (hardware fault). */
uint32_t tkey_timeouts(void);

#endif /* DISLED_TKEY_H */
