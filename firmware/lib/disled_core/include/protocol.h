/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * USB HID protocol v1: report codec, without any I/O. The specification is
 * protocol/README.md at the repository root; protocol/vectors holds byte
 * vectors shared with the desktop app tests.
 *
 * Every report carries a 63-byte payload after its report ID:
 *   [0] type   [1] seq   [2] body length   [3..] body, zero padded
 * Multi-byte fields are little-endian.
 */
#ifndef DISLED_PROTOCOL_H
#define DISLED_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "disled_types.h"
#include "lamp_fsm.h"

#define PROTO_VERSION_MAJOR 1u
#define PROTO_VERSION_MINOR 0u

/* HID report IDs: commands go host -> device, messages device -> host. */
#define PROTO_REPORT_ID_COMMAND 1u
#define PROTO_REPORT_ID_MESSAGE 2u

/* Payload bytes of one report, after the report ID. */
#define PROTO_REPORT_SIZE   63u
#define PROTO_HEADER_SIZE   3u
#define PROTO_MAX_BODY_SIZE (PROTO_REPORT_SIZE - PROTO_HEADER_SIZE)

/* Host -> device. */
#define PROTO_CMD_GET_INFO      0x01u
#define PROTO_CMD_GET_STATE     0x02u
#define PROTO_CMD_SET_STATE     0x03u
#define PROTO_CMD_SET_TELEMETRY 0x04u
#define PROTO_CMD_RESET         0x05u

/* Device -> host. */
#define PROTO_MSG_INFO      0x81u
#define PROTO_MSG_STATE     0x82u
#define PROTO_MSG_TELEMETRY 0x83u
#define PROTO_MSG_ACK       0x8Eu
#define PROTO_MSG_ERROR     0x8Fu

/* Body sizes. */
#define PROTO_SET_STATE_SIZE     5u
#define PROTO_SET_TELEMETRY_SIZE 2u
#define PROTO_INFO_SIZE          56u
#define PROTO_STATE_SIZE         9u
#define PROTO_TELEMETRY_SIZE     32u

/* INFO capabilities. */
#define PROTO_CAP_TELEMETRY     (1u << 0)
#define PROTO_CAP_DEBUG_CONSOLE (1u << 1)

/* INFO flags. */
#define PROTO_INFO_FLAG_DEBUG_BUILD (1u << 0)
#define PROTO_INFO_FLAG_FAULT       (1u << 1)

#define PROTO_FW_VERSION_SIZE 24u

/* Telemetry period limits, in ms (0 stops the stream). */
#define PROTO_TELEMETRY_MIN_MS 20u

typedef enum {
    PROTO_OK = 0,
    PROTO_ERR_UNKNOWN_COMMAND = 1,
    PROTO_ERR_BAD_LENGTH = 2,
    PROTO_ERR_BAD_VALUE = 3,
} proto_status_t;

typedef struct {
    uint8_t type;
    uint8_t seq;
    union {
        lamp_request_t set_state;
        uint16_t telemetry_period_ms;
    } body;
} proto_command_t;

typedef struct {
    uint8_t fw_major;
    uint8_t fw_minor;
    uint8_t fw_patch;
    uint8_t board_revision;
    uint8_t uid[12];
    uint32_t capabilities;
    reset_cause_t reset_cause;
    uint8_t flags;
    uint32_t fault_mcause;
    uint32_t fault_mepc;
    const char *fw_version; /* truncated to PROTO_FW_VERSION_SIZE */
} proto_info_t;

typedef struct {
    uint16_t state_seq;
    lamp_state_t lamp;
    uint8_t applied_percent;
    usb_link_t link;
    change_source_t source;
} proto_state_t;

typedef struct {
    uint32_t uptime_ms;
    uint16_t touch_raw;
    uint16_t touch_value;
    uint16_t touch_baseline;
    int16_t touch_delta_permille;
    uint8_t touch_state;
    uint32_t lux_centi;
    uint16_t als_raw;
    uint16_t duty[LED_CHANNEL_COUNT];
    uint16_t led_current_ma_x10;
    uint16_t cap_ma;
    usb_link_t link;
    uint16_t level_permille;
} proto_telemetry_t;

/* Decodes a command payload. On error, `command->type` and `command->seq`
 * are still filled so that the error can be reported to the host. */
proto_status_t proto_decode_command(const uint8_t payload[PROTO_REPORT_SIZE],
                                    proto_command_t *command);

void proto_encode_info(uint8_t payload[PROTO_REPORT_SIZE], uint8_t seq, const proto_info_t *info);
void proto_encode_state(uint8_t payload[PROTO_REPORT_SIZE], uint8_t seq,
                        const proto_state_t *state);
void proto_encode_telemetry(uint8_t payload[PROTO_REPORT_SIZE], const proto_telemetry_t *telemetry);
void proto_encode_ack(uint8_t payload[PROTO_REPORT_SIZE], uint8_t seq, uint8_t command_type);
void proto_encode_error(uint8_t payload[PROTO_REPORT_SIZE], uint8_t seq, uint8_t command_type,
                        proto_status_t error);

#endif /* DISLED_PROTOCOL_H */
