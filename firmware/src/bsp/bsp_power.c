/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "bsp_power.h"

#include "bsp_time.h"
#include "ch32.h"

/* A few milliseconds at 8 MHz: the crystal starts in about 2 ms. */
#define START_TIMEOUT_LOOPS 0x8000u

static volatile bool s_slow;

void bsp_power_idle(void)
{
    __WFI();
}

void bsp_power_slow_clock(void)
{
    if (s_slow) {
        return;
    }
    __disable_irq();
    RCC->CTLR |= RCC_HSION;
    while ((RCC->CTLR & RCC_HSIRDY) == 0u) {
    }
    RCC->CFGR0 = (RCC->CFGR0 & ~RCC_SW) | RCC_SW_HSI;
    while ((RCC->CFGR0 & RCC_SWS) != RCC_SWS_HSI) {
    }
    /* The PLL settings stay in CFGR0 for the way back. */
    RCC->CTLR &= ~RCC_PLLON;
    RCC->CTLR &= ~RCC_HSEON;
    SystemCoreClockUpdate();
    bsp_time_core_clock_changed();
    s_slow = true;
    __enable_irq();
}

static bool wait_for(const volatile uint32_t *reg, uint32_t mask, uint32_t value)
{
    for (uint32_t i = 0; i < START_TIMEOUT_LOOPS; i++) {
        if ((*reg & mask) == value) {
            return true;
        }
    }
    return false;
}

bool bsp_power_full_clock_from_isr(void)
{
    if (!s_slow) {
        return true;
    }
    RCC->CTLR |= RCC_HSEON;
    if (!wait_for(&RCC->CTLR, RCC_HSERDY, RCC_HSERDY)) {
        RCC->CTLR &= ~RCC_HSEON;
        return false;
    }
    RCC->CTLR |= RCC_PLLON;
    if (!wait_for(&RCC->CTLR, RCC_PLLRDY, RCC_PLLRDY)) {
        RCC->CTLR &= ~(RCC_PLLON | RCC_HSEON);
        return false;
    }
    RCC->CFGR0 = (RCC->CFGR0 & ~RCC_SW) | RCC_SW_PLL;
    if (!wait_for(&RCC->CFGR0, RCC_SWS, RCC_SWS_PLL)) {
        return false;
    }
    SystemCoreClockUpdate();
    bsp_time_core_clock_changed();
    s_slow = false;
    return true;
}

bool bsp_power_full_clock(void)
{
    __disable_irq();
    const bool fast = bsp_power_full_clock_from_isr();
    __enable_irq();
    return fast;
}

bool bsp_power_clock_is_slow(void)
{
    return s_slow;
}
