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

static uint16_t s_period;
static uint16_t s_sample_phase;
static volatile led_pwm_sample_callback_t s_sample_callback;

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

void led_pwm_init(uint32_t frequency_hz)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* Largest resolution that fits 16 bits: prescaler only if needed. */
    const uint32_t ticks = bsp_apb1_timer_clock_hz() / frequency_hz;
    const uint32_t prescaler = (ticks - 1u) / 65536u;
    s_period = (uint16_t)(ticks / (prescaler + 1u));

    TIM_TimeBaseInitTypeDef base = {
        .TIM_Prescaler = (uint16_t)prescaler,
        .TIM_CounterMode = TIM_CounterMode_Up,
        .TIM_Period = (uint16_t)(s_period - 1u),
        .TIM_ClockDivision = TIM_CKD_DIV1,
        .TIM_RepetitionCounter = 0,
    };
    TIM_TimeBaseInit(BOARD_LED_TIMER, &base);

    init_channel(TIM_OC1Init, TIM_OC1PreloadConfig, TIM_OCMode_PWM1, TIM_OutputState_Enable);
    init_channel(TIM_OC2Init, TIM_OC2PreloadConfig, TIM_OCMode_PWM1, TIM_OutputState_Enable);
    init_channel(TIM_OC3Init, TIM_OC3PreloadConfig, TIM_OCMode_PWM1, TIM_OutputState_Enable);
    /* Channel 4: internal compare event only, output disabled (PB1 is the
     * touch input). */
    init_channel(TIM_OC4Init, TIM_OC4PreloadConfig, TIM_OCMode_Timing, TIM_OutputState_Disable);
    led_pwm_set_sample_phase((uint16_t)(s_period / 2u));

    TIM_ARRPreloadConfig(BOARD_LED_TIMER, ENABLE);
    TIM_Cmd(BOARD_LED_TIMER, ENABLE);

    /* Hand the pins to the timer only now, with every duty at 0 %. */
    GPIO_InitTypeDef pins = { .GPIO_Speed = GPIO_Speed_2MHz, .GPIO_Mode = GPIO_Mode_AF_PP };
    pins.GPIO_Pin = BOARD_LED_RED_PIN | BOARD_LED_GREEN_PIN;
    GPIO_Init(BOARD_LED_RED_PORT, &pins);
    pins.GPIO_Pin = BOARD_LED_BLUE_PIN;
    GPIO_Init(BOARD_LED_BLUE_PORT, &pins);

    NVIC_EnableIRQ(TIM3_IRQn);
}

uint16_t led_pwm_period(void)
{
    return s_period;
}

void led_pwm_set(const uint16_t duty[LED_CHANNEL_COUNT])
{
    uint16_t clamped[LED_CHANNEL_COUNT];
    for (size_t i = 0; i < LED_CHANNEL_COUNT; i++) {
        clamped[i] = (duty[i] > s_period) ? s_period : duty[i];
    }
    TIM_SetCompare1(BOARD_LED_TIMER, clamped[LED_CHANNEL_RED]);
    TIM_SetCompare2(BOARD_LED_TIMER, clamped[LED_CHANNEL_GREEN]);
    TIM_SetCompare3(BOARD_LED_TIMER, clamped[LED_CHANNEL_BLUE]);
}

void led_pwm_set_sample_callback(led_pwm_sample_callback_t callback)
{
    s_sample_callback = callback;
}

void led_pwm_set_sample_phase(uint16_t phase_ticks)
{
    s_sample_phase = (phase_ticks < s_period) ? phase_ticks : (uint16_t)(s_period - 1u);
    TIM_SetCompare4(BOARD_LED_TIMER, s_sample_phase);
}

uint16_t led_pwm_sample_phase(void)
{
    return s_sample_phase;
}

void led_pwm_arm_sample(void)
{
    TIM_ClearITPendingBit(BOARD_LED_TIMER, TIM_IT_CC4);
    TIM_ITConfig(BOARD_LED_TIMER, TIM_IT_CC4, ENABLE);
}

void TIM3_IRQHandler(void) BSP_ISR;
void TIM3_IRQHandler(void)
{
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
