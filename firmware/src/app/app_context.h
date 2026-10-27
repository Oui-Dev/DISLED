/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * State shared by the application modules (app.c, app_host.c,
 * app_console.c). Nothing outside src/app includes this file.
 */
#ifndef DISLED_APP_CONTEXT_H
#define DISLED_APP_CONTEXT_H

#include <stdbool.h>
#include <stdint.h>

#include "brightness.h"
#include "bsp/bsp_fault.h"
#include "disled_types.h"
#include "lamp_fsm.h"
#include "led_output.h"
#include "protocol.h"
#include "self_test.h"
#include "touch_detector.h"

typedef struct {
    bool present;
    uint8_t errors;    /* consecutive failures */
    uint32_t retry_ms; /* next detection attempt while absent */
    uint32_t read_ms;  /* next reading: once per integration */
    uint16_t count;    /* last count */
    uint32_t lux_milli;
    uint16_t probes; /* detection attempts since boot (diagnostics) */
} app_als_t;

typedef struct {
    lamp_state_t lamp;
    change_source_t source; /* cause of the latest lamp change */
    brightness_t brightness;
    touch_detector_t touch;
    led_output_t output;
    uint16_t duty[LED_CHANNEL_COUNT];
    usb_link_t link;
    app_als_t als;
    uint16_t cap_override_ma; /* debug: lifts the cap for the heat test, 0 = off */
    reset_cause_t reset_cause;
    fault_record_t fault;
    bool crystal_ok;
    uint16_t usb_suspends; /* since boot (diagnostics) */
    self_test_t self_test;
    uint8_t self_test_seq; /* seq of the SELF_TEST command, for the result */
    bool self_test_report; /* a result is waiting to be sent */
} app_context_t;

/* The single instance, owned by app.c. */
extern app_context_t app;

/* Applies a lamp request coming from the host or the console. Returns
 * true if the state changed. */
bool app_apply_request(const lamp_request_t *request);

/* Starts the self-test; its result is reported to the host with `seq`.
 * Returns false if one is already running. */
bool app_start_self_test(uint8_t seq);

/* Name of a USB link state, for the logs and the console. */
const char *app_link_name(usb_link_t link);

/* Snapshot of the DIAGNOSTICS message. */
void app_diagnostics(proto_diagnostics_t *diagnostics);

#endif /* DISLED_APP_CONTEXT_H */
