/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "led_pwm.h"

#include <stddef.h>

#include "bsp/board.h"
#include "bsp/bsp.h"
#include "bsp/ch32.h"
#include "pwm_dither.h"

static uint16_t s_period;
static volatile led_pwm_sample_callback_t s_sample_callback;

/* Duties in sub-ticks, written by the main loop and read by the update
 * interrupt (16-bit stores are atomic). */
static volatile uint16_t s_duty[LED_CHANNEL_COUNT];
static uint8_t s_dither[LED_CHANNEL_COUNT]; /* interrupt only */

static void init_channel(void (*oc_init)(TIM_TypeDef *, TIM_OCInitTypeDef *),
                         void (*preload)(TIM_TypeDef *, uint16_t), uint16_t mode,
                         uint16_t output_state)
{
    TIM_OCInitTypeDef oc = {
        .TIM_OCMode = mode,
        .TIM_OutputState = output_state,
        .TIM_Pulse = 0,
        .TIM_OCPolarity = TIM_OCPolarity_High,
    };
    oc_init(BOARD_LED_TIMER, &oc);
    preload(BOARD_LED_TIMER, TIM_OCPreload_Enable);
}

/* The three LED channels, in PWM mode. */
static void init_led_channels(void)
{
    init_channel(TIM_OC1Init, TIM_OC1PreloadConfig, TIM_OCMode_PWM1, TIM_OutputState_Enable);
    init_channel(TIM_OC2Init, TIM_OC2PreloadConfig, TIM_OCMode_PWM1, TIM_OutputState_Enable);
    init_channel(TIM_OC3Init, TIM_OC3PreloadConfig, TIM_OCMode_PWM1, TIM_OutputState_Enable);
}

void led_pwm_init(uint32_t frequency_hz)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* Largest resolution within PWM_DITHER_PERIOD_MAX: prescaler only if
     * needed (72 MHz / 20 kHz = 3600 ticks needs none). */
    const uint32_t ticks = bsp_apb1_timer_clock_hz() / frequency_hz;
    const uint32_t prescaler = (ticks - 1u) / PWM_DITHER_PERIOD_MAX;
    s_period = (uint16_t)(ticks / (prescaler + 1u));

    TIM_TimeBaseInitTypeDef base = {
        .TIM_Prescaler = (uint16_t)prescaler,
        .TIM_CounterMode = TIM_CounterMode_Up,
        .TIM_Period = (uint16_t)(s_period - 1u),
        .TIM_ClockDivision = TIM_CKD_DIV1,
        .TIM_RepetitionCounter = 0,
    };
    TIM_TimeBaseInit(BOARD_LED_TIMER, &base);

    init_led_channels();
    /* Channel 4: internal compare event only, output disabled (PB1 is the
     * touch input). */
    init_channel(TIM_OC4Init, TIM_OC4PreloadConfig, TIM_OCMode_Timing, TIM_OutputState_Disable);
    TIM_SetCompare4(BOARD_LED_TIMER, (uint16_t)(s_period / 2u));

    TIM_ARRPreloadConfig(BOARD_LED_TIMER, ENABLE);
    TIM_Cmd(BOARD_LED_TIMER, ENABLE);

    /* Hand the pins to the timer only now, with every duty at 0 %. */
    GPIO_InitTypeDef pins = { .GPIO_Speed = GPIO_Speed_2MHz, .GPIO_Mode = GPIO_Mode_AF_PP };
    pins.GPIO_Pin = BOARD_LED_RED_PIN;
    GPIO_Init(BOARD_LED_RED_PORT, &pins);
    pins.GPIO_Pin = BOARD_LED_GREEN_PIN;
    GPIO_Init(BOARD_LED_GREEN_PORT, &pins);
    pins.GPIO_Pin = BOARD_LED_BLUE_PIN;
    GPIO_Init(BOARD_LED_BLUE_PORT, &pins);

    TIM_ClearITPendingBit(BOARD_LED_TIMER, TIM_IT_Update);
    TIM_ITConfig(BOARD_LED_TIMER, TIM_IT_Update, ENABLE);
    NVIC_EnableIRQ(TIM3_IRQn);
}

void led_pwm_stop(void)
{
    for (size_t i = 0; i < LED_CHANNEL_COUNT; i++) {
        s_duty[i] = 0;
    }
    /* Force the outputs low now: stopping the counter in the middle of a
     * pulse would otherwise leave a channel on. */
    TIM_ForcedOC1Config(BOARD_LED_TIMER, TIM_ForcedAction_InActive);
    TIM_ForcedOC2Config(BOARD_LED_TIMER, TIM_ForcedAction_InActive);
    TIM_ForcedOC3Config(BOARD_LED_TIMER, TIM_ForcedAction_InActive);
    TIM_ITConfig(BOARD_LED_TIMER, TIM_IT_Update | TIM_IT_CC4, DISABLE);
    TIM_Cmd(BOARD_LED_TIMER, DISABLE);
}

void led_pwm_start(void)
{
    init_led_channels();
    TIM_ClearITPendingBit(BOARD_LED_TIMER, TIM_IT_Update);
    TIM_ITConfig(BOARD_LED_TIMER, TIM_IT_Update, ENABLE);
    TIM_Cmd(BOARD_LED_TIMER, ENABLE);
}

uint16_t led_pwm_period(void)
{
    return s_period;
}

void led_pwm_set(const uint16_t duty[LED_CHANNEL_COUNT])
{
    const uint16_t full = (uint16_t)(s_period * PWM_DITHER_SUBTICKS);
    for (size_t i = 0; i < LED_CHANNEL_COUNT; i++) {
        s_duty[i] = (duty[i] > full) ? full : duty[i];
    }
}

/* Start of a period: the compare registers are preloaded, so the values
 * written now apply to the next period. */
static void dither_period(void)
{
    BOARD_LED_TIMER->CH1CVR = pwm_dither_next(&s_dither[LED_CHANNEL_RED], s_duty[LED_CHANNEL_RED]);
    BOARD_LED_TIMER->CH2CVR =
        pwm_dither_next(&s_dither[LED_CHANNEL_GREEN], s_duty[LED_CHANNEL_GREEN]);
    BOARD_LED_TIMER->CH3CVR =
        pwm_dither_next(&s_dither[LED_CHANNEL_BLUE], s_duty[LED_CHANNEL_BLUE]);
}

void led_pwm_set_sample_callback(led_pwm_sample_callback_t callback)
{
    s_sample_callback = callback;
}

void led_pwm_arm_sample(void)
{
    TIM_ClearITPendingBit(BOARD_LED_TIMER, TIM_IT_CC4);
    TIM_ITConfig(BOARD_LED_TIMER, TIM_IT_CC4, ENABLE);
}

void TIM3_IRQHandler(void) BSP_ISR;
void TIM3_IRQHandler(void)
{
    const uint16_t flags = BOARD_LED_TIMER->INTFR;
    if ((flags & TIM_IT_Update) != 0u) {
        BOARD_LED_TIMER->INTFR = (uint16_t)~TIM_IT_Update;
        dither_period();
    }
    if (TIM_GetITStatus(BOARD_LED_TIMER, TIM_IT_CC4) != RESET) {
        /* One shot: disarm before running the callback. */
        TIM_ITConfig(BOARD_LED_TIMER, TIM_IT_CC4, DISABLE);
        TIM_ClearITPendingBit(BOARD_LED_TIMER, TIM_IT_CC4);
        const led_pwm_sample_callback_t callback = s_sample_callback;
        if (callback != NULL) {
            callback();
        }
    }
}
