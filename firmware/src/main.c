/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Toolchain spike: USB device with the vendor HID interface, echoing each
 * command report back as a message report. Replaced by the application
 * entry point once the toolchain is validated.
 */
#include <string.h>

#include "bsp/bsp_time.h"
#include "bsp/ch32.h"
#include "protocol.h"
#include "tusb.h"

uint32_t tusb_time_millis_api(void)
{
    return bsp_time_ms();
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                           const uint8_t *buffer, uint16_t bufsize)
{
    (void)instance;
    (void)report_type;
    if (report_id == PROTO_REPORT_ID_COMMAND && bufsize <= PROTO_REPORT_SIZE) {
        uint8_t message[PROTO_REPORT_SIZE] = { 0 };
        memcpy(message, buffer, bufsize);
        (void)tud_hid_report(PROTO_REPORT_ID_MESSAGE, message, sizeof message);
    }
}

int main(void)
{
    RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_Div3); /* 144 MHz / 3 = 48 MHz */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
    bsp_time_init();

    const tusb_rhport_init_t dev_init = { .role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_AUTO };
    tusb_init(0, &dev_init);

    for (;;) {
        tud_task();
    }
}
