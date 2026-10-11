/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "protocol.h"

#include <string.h>

_Static_assert(PROTO_INFO_SIZE <= PROTO_MAX_BODY_SIZE, "INFO fits one report");
_Static_assert(PROTO_TELEMETRY_SIZE <= PROTO_MAX_BODY_SIZE, "TELEMETRY fits one report");
_Static_assert(LAMP_FIELD_POWER == 0x01u && LAMP_FIELD_COLOR == 0x02u &&
                   LAMP_FIELD_BRIGHTNESS_MODE == 0x04u && LAMP_FIELD_MANUAL_LEVEL == 0x08u,
               "SET_STATE field bits are part of the protocol");

/* Little-endian writer over one payload. */
typedef struct {
    uint8_t *payload;
    size_t pos;
} writer_t;

static writer_t begin_message(uint8_t payload[PROTO_REPORT_SIZE], uint8_t type, uint8_t seq)
{
    memset(payload, 0, PROTO_REPORT_SIZE);
    payload[0] = type;
    payload[1] = seq;
    const writer_t w = { payload, PROTO_HEADER_SIZE };
    return w;
}

static void end_message(writer_t *w)
{
    w->payload[2] = (uint8_t)(w->pos - PROTO_HEADER_SIZE);
}

static void put_u8(writer_t *w, uint8_t value)
{
    w->payload[w->pos++] = value;
}

static void put_u16(writer_t *w, uint16_t value)
{
    put_u8(w, (uint8_t)(value & 0xFFu));
    put_u8(w, (uint8_t)(value >> 8));
}

static void put_u32(writer_t *w, uint32_t value)
{
    put_u16(w, (uint16_t)(value & 0xFFFFu));
    put_u16(w, (uint16_t)(value >> 16));
}

static uint16_t get_u16(const uint8_t *bytes)
{
    return (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
}

proto_status_t proto_decode_command(const uint8_t payload[PROTO_REPORT_SIZE],
                                    proto_command_t *command)
{
    command->type = payload[0];
    command->seq = payload[1];
    const uint8_t length = payload[2];
    const uint8_t *body = &payload[PROTO_HEADER_SIZE];

    if (length > PROTO_MAX_BODY_SIZE) {
        return PROTO_ERR_BAD_LENGTH;
    }

    switch (command->type) {
        case PROTO_CMD_GET_INFO:
        case PROTO_CMD_GET_STATE:
        case PROTO_CMD_RESET:
            return (length == 0u) ? PROTO_OK : PROTO_ERR_BAD_LENGTH;

        case PROTO_CMD_SET_STATE: {
            if (length != PROTO_SET_STATE_SIZE) {
                return PROTO_ERR_BAD_LENGTH;
            }
            /* Fields that are not selected are ignored, whatever their value. */
            if ((body[0] & LAMP_FIELD_POWER) != 0u && body[1] > 1u) {
                return PROTO_ERR_BAD_VALUE;
            }
            lamp_request_t *request = &command->body.set_state;
            request->fields = body[0];
            request->on = body[1] != 0u;
            request->color = (lamp_color_t)body[2];
            request->brightness_mode = (brightness_mode_t)body[3];
            request->manual_level = body[4];
            return lamp_request_is_valid(request) ? PROTO_OK : PROTO_ERR_BAD_VALUE;
        }

        case PROTO_CMD_SET_TELEMETRY: {
            if (length != PROTO_SET_TELEMETRY_SIZE) {
                return PROTO_ERR_BAD_LENGTH;
            }
            const uint16_t period = get_u16(body);
            if (period != 0u && period < PROTO_TELEMETRY_MIN_MS) {
                return PROTO_ERR_BAD_VALUE;
            }
            command->body.telemetry_period_ms = period;
            return PROTO_OK;
        }

        default:
            return PROTO_ERR_UNKNOWN_COMMAND;
    }
}

void proto_encode_info(uint8_t payload[PROTO_REPORT_SIZE], uint8_t seq, const proto_info_t *info)
{
    writer_t w = begin_message(payload, PROTO_MSG_INFO, seq);
    put_u8(&w, PROTO_VERSION_MAJOR);
    put_u8(&w, PROTO_VERSION_MINOR);
    put_u8(&w, info->fw_major);
    put_u8(&w, info->fw_minor);
    put_u8(&w, info->fw_patch);
    put_u8(&w, info->board_revision);
    for (size_t i = 0; i < sizeof info->uid; i++) {
        put_u8(&w, info->uid[i]);
    }
    put_u32(&w, info->capabilities);
    put_u8(&w, (uint8_t)info->reset_cause);
    put_u8(&w, info->flags);
    put_u32(&w, info->fault_mcause);
    put_u32(&w, info->fault_mepc);
    const char *version = (info->fw_version != NULL) ? info->fw_version : "";
    for (size_t i = 0; i < PROTO_FW_VERSION_SIZE; i++) {
        const uint8_t c = (uint8_t)version[i];
        put_u8(&w, c);
        if (c == 0u) {
            /* The rest of the field is already zero. */
            w.pos += PROTO_FW_VERSION_SIZE - i - 1u;
            break;
        }
    }
    end_message(&w);
}

void proto_encode_state(uint8_t payload[PROTO_REPORT_SIZE], uint8_t seq, const proto_state_t *state)
{
    writer_t w = begin_message(payload, PROTO_MSG_STATE, seq);
    put_u16(&w, state->state_seq);
    put_u8(&w, state->lamp.on ? 1u : 0u);
    put_u8(&w, (uint8_t)state->lamp.color);
    put_u8(&w, (uint8_t)state->lamp.brightness_mode);
    put_u8(&w, state->lamp.manual_level);
    put_u8(&w, state->applied_percent);
    put_u8(&w, (uint8_t)state->link);
    put_u8(&w, (uint8_t)state->source);
    end_message(&w);
}

void proto_encode_telemetry(uint8_t payload[PROTO_REPORT_SIZE], const proto_telemetry_t *telemetry)
{
    writer_t w = begin_message(payload, PROTO_MSG_TELEMETRY, 0u);
    put_u32(&w, telemetry->uptime_ms);
    put_u16(&w, telemetry->touch_raw);
    put_u16(&w, telemetry->touch_value);
    put_u16(&w, telemetry->touch_baseline);
    put_u16(&w, (uint16_t)telemetry->touch_delta_permille);
    put_u8(&w, telemetry->touch_state);
    put_u32(&w, telemetry->lux_centi);
    put_u16(&w, telemetry->als_raw);
    for (size_t i = 0; i < LED_CHANNEL_COUNT; i++) {
        put_u16(&w, telemetry->duty[i]);
    }
    put_u16(&w, telemetry->led_current_ma_x10);
    put_u16(&w, telemetry->cap_ma);
    put_u8(&w, (uint8_t)telemetry->link);
    put_u16(&w, telemetry->level_permille);
    end_message(&w);
}

void proto_encode_ack(uint8_t payload[PROTO_REPORT_SIZE], uint8_t seq, uint8_t command_type)
{
    writer_t w = begin_message(payload, PROTO_MSG_ACK, seq);
    put_u8(&w, command_type);
    end_message(&w);
}

void proto_encode_error(uint8_t payload[PROTO_REPORT_SIZE], uint8_t seq, uint8_t command_type,
                        proto_status_t error)
{
    writer_t w = begin_message(payload, PROTO_MSG_ERROR, seq);
    put_u8(&w, command_type);
    put_u8(&w, (uint8_t)error);
    end_message(&w);
}
