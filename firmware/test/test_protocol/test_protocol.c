/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include <string.h>
#include <unity.h>

#include "protocol.h"

static uint8_t payload[PROTO_REPORT_SIZE];
static proto_command_t command;

static void command_payload(uint8_t type, uint8_t seq, const uint8_t *body, uint8_t length)
{
    memset(payload, 0, sizeof payload);
    payload[0] = type;
    payload[1] = seq;
    payload[2] = length;
    if (length > 0u) {
        memcpy(&payload[3], body, length);
    }
}

void setUp(void)
{
    memset(&command, 0, sizeof command);
}

void tearDown(void)
{
}

static void test_decode_get_state(void)
{
    command_payload(PROTO_CMD_GET_STATE, 7, NULL, 0);
    TEST_ASSERT_EQUAL(PROTO_OK, proto_decode_command(payload, &command));
    TEST_ASSERT_EQUAL_UINT8(PROTO_CMD_GET_STATE, command.type);
    TEST_ASSERT_EQUAL_UINT8(7, command.seq);
}

static void test_decode_rejects_a_body_on_get_info(void)
{
    const uint8_t body[] = { 0 };
    command_payload(PROTO_CMD_GET_INFO, 1, body, 1);
    TEST_ASSERT_EQUAL(PROTO_ERR_BAD_LENGTH, proto_decode_command(payload, &command));
    TEST_ASSERT_EQUAL_UINT8(1, command.seq);
}

static void test_decode_unknown_command(void)
{
    command_payload(0x42, 9, NULL, 0);
    TEST_ASSERT_EQUAL(PROTO_ERR_UNKNOWN_COMMAND, proto_decode_command(payload, &command));
    TEST_ASSERT_EQUAL_UINT8(0x42, command.type);
}

static void test_decode_rejects_an_oversized_length(void)
{
    command_payload(PROTO_CMD_GET_STATE, 1, NULL, 0);
    payload[2] = 61;
    TEST_ASSERT_EQUAL(PROTO_ERR_BAD_LENGTH, proto_decode_command(payload, &command));
}

static void test_decode_set_state(void)
{
    const uint8_t body[] = { LAMP_FIELD_ALL, 1, LAMP_COLOR_WARM_WHITE, BRIGHTNESS_MODE_MANUAL, 42 };
    command_payload(PROTO_CMD_SET_STATE, 3, body, sizeof body);
    TEST_ASSERT_EQUAL(PROTO_OK, proto_decode_command(payload, &command));
    const lamp_request_t *request = &command.body.set_state;
    TEST_ASSERT_EQUAL_UINT8(LAMP_FIELD_ALL, request->fields);
    TEST_ASSERT_TRUE(request->on);
    TEST_ASSERT_EQUAL(LAMP_COLOR_WARM_WHITE, request->color);
    TEST_ASSERT_EQUAL(BRIGHTNESS_MODE_MANUAL, request->brightness_mode);
    TEST_ASSERT_EQUAL_UINT8(42, request->manual_level);
}

static void test_decode_set_state_bad_values(void)
{
    const uint8_t bad_color[] = { LAMP_FIELD_COLOR, 0, 4, 0, 0 };
    command_payload(PROTO_CMD_SET_STATE, 1, bad_color, sizeof bad_color);
    TEST_ASSERT_EQUAL(PROTO_ERR_BAD_VALUE, proto_decode_command(payload, &command));

    const uint8_t bad_power[] = { LAMP_FIELD_POWER, 2, 0, 0, 0 };
    command_payload(PROTO_CMD_SET_STATE, 1, bad_power, sizeof bad_power);
    TEST_ASSERT_EQUAL(PROTO_ERR_BAD_VALUE, proto_decode_command(payload, &command));

    const uint8_t bad_level[] = { LAMP_FIELD_MANUAL_LEVEL, 0, 0, 0, 0 };
    command_payload(PROTO_CMD_SET_STATE, 1, bad_level, sizeof bad_level);
    TEST_ASSERT_EQUAL(PROTO_ERR_BAD_VALUE, proto_decode_command(payload, &command));
}

static void test_decode_set_state_ignores_unselected_fields(void)
{
    const uint8_t body[] = { LAMP_FIELD_COLOR, 0xFF, LAMP_COLOR_RED, 0xFF, 0xFF };
    command_payload(PROTO_CMD_SET_STATE, 1, body, sizeof body);
    TEST_ASSERT_EQUAL(PROTO_OK, proto_decode_command(payload, &command));
}

static void test_decode_set_telemetry_limits(void)
{
    const uint8_t too_fast[] = { 10, 0 };
    command_payload(PROTO_CMD_SET_TELEMETRY, 1, too_fast, sizeof too_fast);
    TEST_ASSERT_EQUAL(PROTO_ERR_BAD_VALUE, proto_decode_command(payload, &command));

    const uint8_t stop[] = { 0, 0 };
    command_payload(PROTO_CMD_SET_TELEMETRY, 1, stop, sizeof stop);
    TEST_ASSERT_EQUAL(PROTO_OK, proto_decode_command(payload, &command));
    TEST_ASSERT_EQUAL_UINT16(0, command.body.telemetry_period_ms);

    const uint8_t every_250ms[] = { 0xFA, 0x00 };
    command_payload(PROTO_CMD_SET_TELEMETRY, 1, every_250ms, sizeof every_250ms);
    TEST_ASSERT_EQUAL(PROTO_OK, proto_decode_command(payload, &command));
    TEST_ASSERT_EQUAL_UINT16(250, command.body.telemetry_period_ms);
}

static void test_encode_state_bytes(void)
{
    const proto_state_t state = {
        .state_seq = 0x1234,
        .lamp = { .on = true, .color = LAMP_COLOR_ORANGE, .brightness_mode = BRIGHTNESS_MODE_MANUAL,
                  .manual_level = 30 },
        .applied_percent = 30,
        .link = USB_LINK_CONFIGURED,
        .source = CHANGE_SOURCE_TOUCH,
    };
    proto_encode_state(payload, 5, &state);
    const uint8_t expected[] = { PROTO_MSG_STATE, 5, PROTO_STATE_SIZE, 0x34, 0x12, 1, 1, 1, 30, 30, 1, 1 };
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, payload, sizeof expected);
    for (size_t i = sizeof expected; i < PROTO_REPORT_SIZE; i++) {
        TEST_ASSERT_EQUAL_HEX8(0, payload[i]);
    }
}

static void test_encode_info_layout(void)
{
    const proto_info_t info = {
        .fw_major = 0,
        .fw_minor = 1,
        .fw_patch = 2,
        .board_revision = 4,
        .uid = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 },
        .capabilities = PROTO_CAP_TELEMETRY | PROTO_CAP_DEBUG_CONSOLE,
        .reset_cause = RESET_CAUSE_POWER_ON,
        .flags = PROTO_INFO_FLAG_DEBUG_BUILD,
        .fault_mcause = 0x01020304,
        .fault_mepc = 0xA0B0C0D0,
        .fw_version = "0.1.2-3-gabcdef0",
    };
    proto_encode_info(payload, 9, &info);
    TEST_ASSERT_EQUAL_HEX8(PROTO_MSG_INFO, payload[0]);
    TEST_ASSERT_EQUAL_UINT8(9, payload[1]);
    TEST_ASSERT_EQUAL_UINT8(PROTO_INFO_SIZE, payload[2]);
    TEST_ASSERT_EQUAL_UINT8(PROTO_VERSION_MAJOR, payload[3]);
    TEST_ASSERT_EQUAL_UINT8(4, payload[8]);
    TEST_ASSERT_EQUAL_UINT8(1, payload[9]);
    TEST_ASSERT_EQUAL_UINT8(12, payload[20]);
    TEST_ASSERT_EQUAL_HEX8(0x03, payload[21]);
    TEST_ASSERT_EQUAL_UINT8(RESET_CAUSE_POWER_ON, payload[25]);
    TEST_ASSERT_EQUAL_HEX8(0x04, payload[27]);
    TEST_ASSERT_EQUAL_HEX8(0xD0, payload[31]);
    TEST_ASSERT_EQUAL_STRING("0.1.2-3-gabcdef0", (const char *)&payload[35]);
    TEST_ASSERT_EQUAL_HEX8(0, payload[3 + PROTO_INFO_SIZE]);
}

static void test_encode_info_truncates_a_long_version(void)
{
    const proto_info_t info = { .fw_version = "0123456789abcdefghijklmnopqrstuvwxyz" };
    proto_encode_info(payload, 0, &info);
    TEST_ASSERT_EQUAL_MEMORY("0123456789abcdefghijklmn", &payload[35], PROTO_FW_VERSION_SIZE);
    TEST_ASSERT_EQUAL_UINT8(PROTO_INFO_SIZE, payload[2]);
}

static void test_encode_telemetry_size_and_signed_delta(void)
{
    const proto_telemetry_t telemetry = { .touch_delta_permille = -2, .level_permille = 1000 };
    proto_encode_telemetry(payload, &telemetry);
    TEST_ASSERT_EQUAL_HEX8(PROTO_MSG_TELEMETRY, payload[0]);
    TEST_ASSERT_EQUAL_UINT8(0, payload[1]);
    TEST_ASSERT_EQUAL_UINT8(PROTO_TELEMETRY_SIZE, payload[2]);
    TEST_ASSERT_EQUAL_HEX8(0xFE, payload[3 + 10]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, payload[3 + 11]);
    TEST_ASSERT_EQUAL_HEX8(0xE8, payload[3 + 30]);
    TEST_ASSERT_EQUAL_HEX8(0x03, payload[3 + 31]);
}

static void test_encode_ack_and_error(void)
{
    proto_encode_ack(payload, 4, PROTO_CMD_SET_TELEMETRY);
    const uint8_t ack[] = { PROTO_MSG_ACK, 4, 1, PROTO_CMD_SET_TELEMETRY };
    TEST_ASSERT_EQUAL_HEX8_ARRAY(ack, payload, sizeof ack);

    proto_encode_error(payload, 6, 0x42, PROTO_ERR_UNKNOWN_COMMAND);
    const uint8_t error[] = { PROTO_MSG_ERROR, 6, 2, 0x42, PROTO_ERR_UNKNOWN_COMMAND };
    TEST_ASSERT_EQUAL_HEX8_ARRAY(error, payload, sizeof error);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_decode_get_state);
    RUN_TEST(test_decode_rejects_a_body_on_get_info);
    RUN_TEST(test_decode_unknown_command);
    RUN_TEST(test_decode_rejects_an_oversized_length);
    RUN_TEST(test_decode_set_state);
    RUN_TEST(test_decode_set_state_bad_values);
    RUN_TEST(test_decode_set_state_ignores_unselected_fields);
    RUN_TEST(test_decode_set_telemetry_limits);
    RUN_TEST(test_encode_state_bytes);
    RUN_TEST(test_encode_info_layout);
    RUN_TEST(test_encode_info_truncates_a_long_version);
    RUN_TEST(test_encode_telemetry_size_and_signed_delta);
    RUN_TEST(test_encode_ack_and_error);
    return UNITY_END();
}
