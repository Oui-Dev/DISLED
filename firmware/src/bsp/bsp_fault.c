/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "bsp_fault.h"

#include "ch32.h"

#define FAULT_MAGIC 0xFA17u

static void unlock_backup_domain(void)
{
    RCC->APB1PCENR |= RCC_APB1Periph_PWR | RCC_APB1Periph_BKP;
    PWR->CTLR |= PWR_CTLR_DBP;
}

static uint32_t read_u32(volatile uint16_t *low, volatile uint16_t *high)
{
    return (uint32_t)*low | ((uint32_t)*high << 16);
}

static void write_u32(volatile uint16_t *low, volatile uint16_t *high, uint32_t value)
{
    *low = (uint16_t)(value & 0xFFFFu);
    *high = (uint16_t)(value >> 16);
}

fault_record_t bsp_fault_take(void)
{
    fault_record_t record = { 0 };

    unlock_backup_domain();
    if (BKP->DATAR1 == FAULT_MAGIC) {
        record.valid = true;
        record.mcause = read_u32(&BKP->DATAR2, &BKP->DATAR3);
        record.mepc = read_u32(&BKP->DATAR4, &BKP->DATAR5);
        record.mtval = read_u32(&BKP->DATAR6, &BKP->DATAR7);
    }
    BKP->DATAR1 = 0;
    return record;
}

__attribute__((noreturn)) static void record_and_reset(void)
{
    unlock_backup_domain();
    write_u32(&BKP->DATAR2, &BKP->DATAR3, __get_MCAUSE());
    write_u32(&BKP->DATAR4, &BKP->DATAR5, __get_MEPC());
    write_u32(&BKP->DATAR6, &BKP->DATAR7, __get_MTVAL());
    BKP->DATAR1 = FAULT_MAGIC;
    NVIC_SystemReset();
    for (;;) {
    }
}

/* One handler for both vectors (see usb_irq.c on folded handlers). */
void bsp_fault_handler(void) BSP_ISR;
void bsp_fault_handler(void)
{
    record_and_reset();
}

void HardFault_Handler(void) __attribute__((alias("bsp_fault_handler")));
void NMI_Handler(void) __attribute__((alias("bsp_fault_handler")));
