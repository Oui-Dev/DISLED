/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * USBD (FSDEV) interrupts, forwarded to TinyUSB on root port 0.
 *
 * The three vector entries are aliases of one handler. Three identical
 * handlers would let the compiler fold them into calls to each other,
 * which only works by accident with interrupt functions.
 *
 * During USB suspend the core runs on the slow clock without the PLL, and
 * USB cannot work without it: the wake-up (EXTI line 18, or the USB
 * interrupt itself) restores the clock before TinyUSB sees anything.
 */
#include "bsp/bsp_power.h"
#include "bsp/ch32.h"
#include "tusb.h"

void usb_irq_handler(void) BSP_ISR;
void usb_irq_handler(void)
{
    if (bsp_power_clock_is_slow()) {
        (void)bsp_power_full_clock_from_isr();
    }
    EXTI->INTFR = EXTI_Line18; /* USB wake-up line, if it fired */
    tud_int_handler(0);
}

void USB_LP_CAN1_RX0_IRQHandler(void) __attribute__((alias("usb_irq_handler")));
void USB_HP_CAN1_TX_IRQHandler(void) __attribute__((alias("usb_irq_handler")));
void USBWakeUp_IRQHandler(void) __attribute__((alias("usb_irq_handler")));
