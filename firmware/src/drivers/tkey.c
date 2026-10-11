/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "tkey.h"

#include <stddef.h>

#include "bsp/board.h"
#include "bsp/bsp_time.h"
#include "bsp/ch32.h"
#include "led_pwm.h"

/* ADC_CTLR1 touch-key bits (WCH TouchKey example, CH32V20x EVT). */
#define ADC_CTLR1_TKENABLE (1u << 24)
#define ADC_CTLR1_BUFEN    (1u << 26)

/* Power of two, so the indices wrap with a mask. */
#define QUEUE_SIZE 32u

/* A measurement takes about 5 us; give up after roughly 100 us. */
#define MEASURE_MAX_POLLS 2000u

typedef struct {
    uint16_t value;
    uint32_t time_ms;
} sample_t;

static const tkey_config_t *s_config;
static sample_t s_queue[QUEUE_SIZE];
static volatile uint32_t s_head; /* written by the interrupt */
static volatile uint32_t s_tail; /* written by the main loop */
static volatile uint32_t s_overruns;
static volatile uint32_t s_timeouts;
static volatile bool s_dither;
static uint32_t s_lfsr = 0xACE1u;

void tkey_init(const tkey_config_t *config)
{
    s_config = config;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    GPIO_InitTypeDef pin = {
        .GPIO_Pin = BOARD_TOUCH_PIN,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_Mode = GPIO_Mode_AIN,
    };
    GPIO_Init(BOARD_TOUCH_PORT, &pin);

    ADC_InitTypeDef adc = {
        .ADC_Mode = ADC_Mode_Independent,
        .ADC_ScanConvMode = DISABLE,
        .ADC_ContinuousConvMode = DISABLE,
        .ADC_ExternalTrigConv = ADC_ExternalTrigConv_None,
        .ADC_DataAlign = ADC_DataAlign_Right,
        .ADC_NbrOfChannel = 1,
    };
    ADC_Init(ADC1, &adc);
    ADC_RegularChannelConfig(ADC1, BOARD_TOUCH_ADC_CHANNEL, 1, ADC_SampleTime_7Cycles5);
    ADC_Cmd(ADC1, ENABLE);
    TKey1->CTLR1 |= ADC_CTLR1_TKENABLE | ADC_CTLR1_BUFEN;
}

/* Runs in the TIM3 interrupt, at the sample point of the PWM period. */
static void measure(void)
{
    /* Writing the charge then the discharge time starts the measurement;
     * it lasts a few microseconds. */
    TKey1->IDATAR1 = s_config->charge_time;
    TKey1->RDATAR = s_config->discharge_time;
    uint32_t polls = 0;
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET) {
        if (++polls >= MEASURE_MAX_POLLS) {
            s_timeouts++;
            return;
        }
    }
    const uint16_t value = (uint16_t)TKey1->RDATAR;

    const uint32_t head = s_head;
    if (head - s_tail >= QUEUE_SIZE) {
        s_overruns++;
        return;
    }
    s_queue[head & (QUEUE_SIZE - 1u)] = (sample_t){ value, bsp_time_ms() };
    s_head = head + 1u;
}

/* Runs in the SysTick interrupt, every millisecond. */
static void on_tick(void)
{
    if (s_dither) {
        /* 16-bit Galois LFSR: a pseudo-random phase for each sample. */
        s_lfsr = (s_lfsr >> 1) ^ ((0u - (s_lfsr & 1u)) & 0xB400u);
        led_pwm_set_sample_phase((uint16_t)(s_lfsr % led_pwm_period()));
    }
    led_pwm_arm_sample();
}

void tkey_start(void)
{
    led_pwm_set_sample_callback(measure);
    bsp_time_set_tick_hook(on_tick);
}

void tkey_stop(void)
{
    bsp_time_set_tick_hook(NULL);
    led_pwm_set_sample_callback(NULL);
}

bool tkey_read(uint16_t *sample, uint32_t *time_ms)
{
    const uint32_t tail = s_tail;
    if (tail == s_head) {
        return false;
    }
    const sample_t entry = s_queue[tail & (QUEUE_SIZE - 1u)];
    s_tail = tail + 1u;
    *sample = entry.value;
    *time_ms = entry.time_ms;
    return true;
}

uint32_t tkey_overruns(void)
{
    return s_overruns;
}

uint32_t tkey_timeouts(void)
{
    return s_timeouts;
}

void tkey_set_phase(uint16_t phase_ticks)
{
    s_dither = false;
    led_pwm_set_sample_phase(phase_ticks);
}

void tkey_set_dither(bool enabled)
{
    s_dither = enabled;
}
