/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * DISLED firmware entry point. The startup code already set the clocks
 * (SystemInit); everything else starts here.
 */
#include "app/app.h"
#include "bsp/bsp.h"
#include "bsp/bsp_fault.h"

int main(void)
{
    bsp_init();
    const fault_record_t fault = bsp_fault_take();
    app_init(&fault);
    app_run();
}
