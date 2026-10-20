/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "usb_device.h"

#include <string.h>

#include "bsp/bsp_time.h"
#include "bsp/ch32.h"
#include "tusb.h"

#define RX_QUEUE_SIZE 2u
#define TX_QUEUE_SIZE 4u

typedef struct {
    uint8_t payload[PROTO_REPORT_SIZE];
} report_t;

static bool s_started;
static usb_link_t s_link = USB_LINK_UNCONFIGURED;

static report_t s_rx[RX_QUEUE_SIZE];
static size_t s_rx_head;
static size_t s_rx_count;

static report_t s_tx[TX_QUEUE_SIZE];
static size_t s_tx_head;
static size_t s_tx_count;

uint32_t tusb_time_millis_api(void)
{
    return bsp_time_ms();
}

void usb_device_init(bool crystal_ok)
{
    if (!crystal_ok) {
        return;
    }
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
    /* Bus activity during suspend raises EXTI line 18 (TinyUSB enables its
     * USBWakeUp interrupt, not the line itself). */
    EXTI->INTFR = EXTI_Line18;
    EXTI->RTENR |= EXTI_Line18;
    EXTI->INTENR |= EXTI_Line18;
    const tusb_rhport_init_t dev_init = { .role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_FULL };
    s_started = tusb_init(0, &dev_init);
}

static void flush_tx(void)
{
    if (s_tx_count == 0u || !tud_hid_ready()) {
        return;
    }
    if (tud_hid_report(PROTO_REPORT_ID_MESSAGE, s_tx[s_tx_head].payload, PROTO_REPORT_SIZE)) {
        s_tx_head = (s_tx_head + 1u) % TX_QUEUE_SIZE;
        s_tx_count--;
    }
}

void usb_device_task(void)
{
    if (!s_started) {
        return;
    }
    tud_task();
    flush_tx();
}

usb_link_t usb_device_link(void)
{
    return s_link;
}

bool usb_hid_send(const uint8_t payload[PROTO_REPORT_SIZE])
{
    if (s_link != USB_LINK_CONFIGURED || s_tx_count == TX_QUEUE_SIZE) {
        return false;
    }
    const size_t tail = (s_tx_head + s_tx_count) % TX_QUEUE_SIZE;
    memcpy(s_tx[tail].payload, payload, PROTO_REPORT_SIZE);
    s_tx_count++;
    flush_tx();
    return true;
}

bool usb_hid_receive(uint8_t payload[PROTO_REPORT_SIZE])
{
    if (s_rx_count == 0u) {
        return false;
    }
    memcpy(payload, s_rx[s_rx_head].payload, PROTO_REPORT_SIZE);
    s_rx_head = (s_rx_head + 1u) % RX_QUEUE_SIZE;
    s_rx_count--;
    return true;
}

/* --- TinyUSB callbacks (called from tud_task, in the main loop) --- */

void tud_mount_cb(void)
{
    s_link = USB_LINK_CONFIGURED;
}

void tud_umount_cb(void)
{
    s_link = USB_LINK_UNCONFIGURED;
    s_tx_count = 0;
    s_rx_count = 0;
}

void tud_suspend_cb(bool remote_wakeup_en)
{
    (void)remote_wakeup_en;
    s_link = USB_LINK_SUSPENDED;
}

void tud_resume_cb(void)
{
    s_link = tud_mounted() ? USB_LINK_CONFIGURED : USB_LINK_UNCONFIGURED;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen)
{
    /* No input or feature report is served through the control pipe. */
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
    if (report_type != HID_REPORT_TYPE_OUTPUT) {
        return;
    }
    /* Interrupt OUT transfers keep the report ID as the first byte. */
    if (report_id == 0u) {
        if (bufsize == 0u) {
            return;
        }
        report_id = buffer[0];
        buffer++;
        bufsize--;
    }
    if (report_id != PROTO_REPORT_ID_COMMAND || s_rx_count == RX_QUEUE_SIZE) {
        return;
    }
    const size_t tail = (s_rx_head + s_rx_count) % RX_QUEUE_SIZE;
    const size_t length = (bufsize < PROTO_REPORT_SIZE) ? bufsize : PROTO_REPORT_SIZE;
    memset(s_rx[tail].payload, 0, PROTO_REPORT_SIZE);
    memcpy(s_rx[tail].payload, buffer, length);
    s_rx_count++;
}

#if DISLED_DEBUG

size_t usb_cdc_read(char *buffer, size_t max)
{
    if (!s_started || !tud_cdc_available()) {
        return 0;
    }
    return tud_cdc_read(buffer, (uint32_t)max);
}

void usb_cdc_write(const char *data, size_t length)
{
    if (!s_started || !tud_cdc_connected() || tud_cdc_write_available() < length) {
        return;
    }
    (void)tud_cdc_write(data, (uint32_t)length);
    (void)tud_cdc_write_flush();
}

#endif
