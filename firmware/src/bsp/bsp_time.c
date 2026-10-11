/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "bsp_time.h"

#include "ch32.h"

/* SysTick control bits (QingKe V4 core). */
#define STK_CTLR_STE   (1u << 0) /* counter enable */
#define STK_CTLR_STIE  (1u << 1) /* interrupt enable */
#define STK_CTLR_STCLK (1u << 2) /* clock = HCLK (not HCLK/8) */
#define STK_CTLR_STRE  (1u << 3) /* auto-reload on compare match */

static volatile uint32_t s_ms;

void SysTick_Handler(void) BSP_ISR;
void SysTick_Handler(void)
{
    SysTick->SR = 0;
    s_ms++;
}

void bsp_time_init(void)
{
    SysTick->CTLR = 0;
    SysTick->SR = 0;
    SysTick->CNT = 0;
    SysTick->CMP = (SystemCoreClock / 1000u) - 1u;
    SysTick->CTLR = STK_CTLR_STE | STK_CTLR_STIE | STK_CTLR_STCLK | STK_CTLR_STRE;
    NVIC_EnableIRQ(SysTick_IRQn);
}

uint32_t bsp_time_ms(void)
{
    return s_ms;
}

void bsp_time_delay_us(uint32_t us)
{
    /* The counter reloads every millisecond, so measure elapsed ticks
     * modulo one period. */
    const uint32_t period = (uint32_t)SysTick->CMP + 1u;
    const uint32_t ticks = us * (SystemCoreClock / 1000000u);
    const uint32_t start = (uint32_t)SysTick->CNT;
    uint32_t elapsed = 0;

    while (elapsed < ticks) {
        const uint32_t now = (uint32_t)SysTick->CNT;
        elapsed = (now >= start) ? (now - start) : (period - start + now);
    }
}
