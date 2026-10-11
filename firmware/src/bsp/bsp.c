/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "bsp.h"

#include "board.h"
#include "bsp_time.h"
#include "ch32.h"

static reset_cause_t s_reset_cause = RESET_CAUSE_UNKNOWN;
static bool s_crystal_ok;

static void capture_reset_cause(void)
{
    /* Highest priority first: a watchdog reset also sets the pin flag. */
    if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET) {
        s_reset_cause = RESET_CAUSE_WATCHDOG;
    } else if (RCC_GetFlagStatus(RCC_FLAG_WWDGRST) != RESET) {
        s_reset_cause = RESET_CAUSE_WINDOW_WATCHDOG;
    } else if (RCC_GetFlagStatus(RCC_FLAG_SFTRST) != RESET) {
        s_reset_cause = RESET_CAUSE_SOFTWARE;
    } else if (RCC_GetFlagStatus(RCC_FLAG_LPWRRST) != RESET) {
        s_reset_cause = RESET_CAUSE_LOW_POWER;
    } else if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET) {
        s_reset_cause = RESET_CAUSE_POWER_ON;
    } else if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET) {
        s_reset_cause = RESET_CAUSE_PIN;
    } else {
        s_reset_cause = RESET_CAUSE_UNKNOWN;
    }
    RCC_ClearFlag();
}

static void init_clocks(void)
{
    /* The startup code ran SystemInit(): 8 MHz crystal x 18 = 144 MHz,
     * HCLK = PCLK2 = 144 MHz, PCLK1 = 72 MHz. If the crystal failed, the
     * chip stayed on the internal oscillator; recompute the real clock. */
    s_crystal_ok = (RCC->CFGR0 & RCC_SWS) == RCC_SWS_PLL;
    SystemCoreClockUpdate();

    /* ADC clock = PCLK2 / 8 must stay within 14 MHz: 72 MHz / 8 = 9 MHz. */
    RCC_PCLK2Config(RCC_HCLK_Div2);
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);

    if (s_crystal_ok) {
        RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_Div3); /* 144 / 3 = 48 MHz */
    }

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
}

static void init_unused_pins(void)
{
    GPIO_InitTypeDef init = {
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_Mode = GPIO_Mode_AIN,
    };
    init.GPIO_Pin = BOARD_UNUSED_PINS_A;
    GPIO_Init(GPIOA, &init);
    init.GPIO_Pin = BOARD_UNUSED_PINS_B;
    GPIO_Init(GPIOB, &init);
}

void bsp_init(void)
{
    capture_reset_cause();
    init_clocks();
    init_unused_pins();
    bsp_time_init();
}

reset_cause_t bsp_reset_cause(void)
{
    return s_reset_cause;
}

bool bsp_crystal_ok(void)
{
    return s_crystal_ok;
}

uint32_t bsp_apb1_timer_clock_hz(void)
{
    RCC_ClocksTypeDef clocks;
    RCC_GetClocksFreq(&clocks);
    /* Timers run at twice PCLK1 when APB1 is divided. */
    const bool divided = (RCC->CFGR0 & RCC_PPRE1) != RCC_PPRE1_DIV1;
    return divided ? 2u * clocks.PCLK1_Frequency : clocks.PCLK1_Frequency;
}

void bsp_system_reset(void)
{
    NVIC_SystemReset();
    for (;;) {
    }
}
