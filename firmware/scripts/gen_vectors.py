# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 The DISLED contributors
# Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
"""Turn protocol/vectors/v1.json into a C header for the unit tests.

Runs before the native build. The header lands in the build directory, so
it never goes stale in the source tree.
"""

import json
import os

Import("env")  # noqa: F821 - provided by PlatformIO

STATUS = {
    "ok": "PROTO_OK",
    "unknown_command": "PROTO_ERR_UNKNOWN_COMMAND",
    "bad_length": "PROTO_ERR_BAD_LENGTH",
    "bad_value": "PROTO_ERR_BAD_VALUE",
}
KINDS = {"state": "MSG_STATE", "info": "MSG_INFO", "telemetry": "MSG_TELEMETRY",
         "diagnostics": "MSG_DIAGNOSTICS", "self_test_result": "MSG_SELF_TEST_RESULT",
         "ack": "MSG_ACK", "error": "MSG_ERROR"}


def c_bytes(hex_text):
    values = [int(pair, 16) for pair in hex_text.split()]
    return "{ " + ", ".join("0x%02x" % v for v in values) + " }"


def c_bool(value):
    return "true" if value else "false"


def command_entry(vector):
    set_state = vector.get("set_state")
    request = "{ 0 }"
    if set_state is not None:
        request = "{ .fields = %d, .on = %s, .color = (lamp_color_t)%d, " \
                  ".brightness_mode = (brightness_mode_t)%d, .manual_level = %d }" % (
                      set_state["fields"], c_bool(set_state.get("on", False)),
                      set_state.get("color", 0), set_state.get("brightness_mode", 0),
                      set_state.get("manual_level", 0))
    period = vector.get("telemetry_period_ms")
    bias = vector.get("auto_bias")
    return ("    { .name = \"%s\", .payload = %s, .status = %s, .type = 0x%02x, .seq = %d,\n"
            "      .has_set_state = %s, .set_state = %s,\n"
            "      .has_period = %s, .telemetry_period_ms = %d,\n"
            "      .has_auto_bias = %s, .auto_bias = %d },\n") % (
                vector["name"], c_bytes(vector["bytes"]), STATUS[vector["status"]], vector["type"],
                vector["seq"], c_bool(set_state is not None), request, c_bool(period is not None),
                period or 0, c_bool(bias is not None), bias or 0)


def message_entry(vector):
    kind = vector["kind"]
    fields = []
    if kind == "state":
        s = vector["state"]
        fields.append(".state = { .state_seq = %d, .lamp = { .on = %s, .color = (lamp_color_t)%d, "
                      ".brightness_mode = (brightness_mode_t)%d, .manual_level = %d }, "
                      ".applied_percent = %d, .link = (usb_link_t)%d, .source = (change_source_t)%d }"
                      % (s["state_seq"], c_bool(s["on"]), s["color"], s["brightness_mode"],
                         s["manual_level"], s["applied_percent"], s["link"], s["source"]))
    elif kind == "info":
        i = vector["info"]
        uid = ", ".join("0x%s" % i["uid"][k:k + 2] for k in range(0, len(i["uid"]), 2))
        fields.append(".info = { .fw_major = %d, .fw_minor = %d, .fw_patch = %d, "
                      ".board_revision = %d, .uid = { %s }, .capabilities = %du, "
                      ".reset_cause = (reset_cause_t)%d, .flags = %d, .fault_mcause = 0x%08xu, "
                      ".fault_mepc = 0x%08xu, .fw_version = \"%s\" }"
                      % (i["fw_major"], i["fw_minor"], i["fw_patch"], i["board_revision"], uid,
                         i["capabilities"], i["reset_cause"], i["flags"], i["fault_mcause"],
                         i["fault_mepc"], i["fw_version"]))
    elif kind == "telemetry":
        t = vector["telemetry"]
        fields.append(".telemetry = { .uptime_ms = %du, .touch_raw = %d, .touch_value = %d, "
                      ".touch_baseline = %d, .touch_delta_permille = %d, .touch_state = %d, "
                      ".lux_centi = %du, .als_raw = %d, .duty = { %d, %d, %d }, "
                      ".led_current_ma_x10 = %d, .cap_ma = %d, .link = (usb_link_t)%d, "
                      ".level_permille = %d }"
                      % (t["uptime_ms"], t["touch_raw"], t["touch_value"], t["touch_baseline"],
                         t["touch_delta_permille"], t["touch_state"], t["lux_centi"], t["als_raw"],
                         t["duty"][0], t["duty"][1], t["duty"][2], t["led_current_ma_x10"],
                         t["cap_ma"], t["link"], t["level_permille"]))
    elif kind == "diagnostics":
        d = vector["diagnostics"]
        fields.append(".diagnostics = { .uptime_ms = %du, .boots = %d, .watchdog_resets = %d, "
                      ".faults = %d, .fault_mcause = 0x%08xu, .fault_mepc = 0x%08xu, "
                      ".fault_mtval = 0x%08xu, .stack_free_min = %d, .i2c_errors = %d, "
                      ".light_sensor_probes = %d, .touch_samples_lost = %d, "
                      ".touch_recalibrations = %d, .usb_suspends = %d, .auto_bias = %d, "
                      ".flags = %d }"
                      % (d["uptime_ms"], d["boots"], d["watchdog_resets"], d["faults"],
                         d["fault_mcause"], d["fault_mepc"], d["fault_mtval"], d["stack_free_min"],
                         d["i2c_errors"], d["light_sensor_probes"], d["touch_samples_lost"],
                         d["touch_recalibrations"], d["usb_suspends"], d["auto_bias"],
                         d["flags"]))
    elif kind == "self_test_result":
        r = vector["self_test_result"]
        fields.append(".self_test_result = { .results = %d, .lux_centi = %du, "
                      ".touch_baseline = %d, .touch_peak_permille = %d }"
                      % (r["flags"], r["lux_centi"], r["touch_baseline"],
                         r["touch_peak_permille"]))
    elif kind == "ack":
        fields.append(".command = 0x%02x" % vector["command"])
    elif kind == "error":
        fields.append(".command = 0x%02x, .error = (proto_status_t)%d" % (vector["command"],
                                                                         vector["error"]))
    fields.append(".canonical = %s" % c_bool(vector.get("canonical", True)))
    return "    { .name = \"%s\", .kind = %s, .seq = %d, .payload = %s,\n      %s },\n" % (
        vector["name"], KINDS[kind], vector["seq"], c_bytes(vector["bytes"]), ", ".join(fields))


def generate(source, target):
    with open(source, encoding="utf-8") as handle:
        doc = json.load(handle)
    lines = [
        "/* Generated from protocol/vectors/v1.json by scripts/gen_vectors.py. Do not edit. */\n",
        "#include <stdbool.h>\n#include <stdint.h>\n\n#include \"protocol.h\"\n\n",
        "#define VECTORS_PROTOCOL_MAJOR %du\n#define VECTORS_PROTOCOL_MINOR %du\n\n"
        % tuple(doc["protocol_version"]),
        "typedef struct {\n    const char *name;\n    uint8_t payload[PROTO_REPORT_SIZE];\n"
        "    proto_status_t status;\n    uint8_t type;\n    uint8_t seq;\n    bool has_set_state;\n"
        "    lamp_request_t set_state;\n    bool has_period;\n    uint16_t telemetry_period_ms;\n"
        "    bool has_auto_bias;\n    int8_t auto_bias;\n"
        "} command_vector_t;\n\n",
        "typedef enum {\n    MSG_STATE,\n    MSG_INFO,\n    MSG_TELEMETRY,\n    MSG_DIAGNOSTICS,\n"
        "    MSG_SELF_TEST_RESULT,\n    MSG_ACK,\n    MSG_ERROR\n} message_kind_t;\n\n",
        "typedef struct {\n    const char *name;\n    message_kind_t kind;\n    uint8_t seq;\n"
        "    uint8_t payload[PROTO_REPORT_SIZE];\n    proto_state_t state;\n    proto_info_t info;\n"
        "    proto_telemetry_t telemetry;\n    proto_diagnostics_t diagnostics;\n"
        "    proto_self_test_result_t self_test_result;\n    uint8_t command;\n"
        "    proto_status_t error;\n    bool canonical; /* false: decoder-only vector */\n"
        "} message_vector_t;\n\n",
        "static const command_vector_t command_vectors[] = {\n",
    ]
    lines += [command_entry(v) for v in doc["commands"]]
    lines += ["};\n\nstatic const message_vector_t message_vectors[] = {\n"]
    lines += [message_entry(v) for v in doc["messages"]]
    lines += ["};\n"]
    os.makedirs(os.path.dirname(target), exist_ok=True)
    with open(target, "w", encoding="utf-8", newline="\n") as handle:
        handle.writelines(lines)


project_dir = env.subst("$PROJECT_DIR")  # noqa: F821
generated_dir = os.path.join(env.subst("$BUILD_DIR"), "generated")  # noqa: F821
generate(os.path.join(project_dir, "..", "protocol", "vectors", "v1.json"),
         os.path.join(generated_dir, "protocol_vectors.h"))
env.Append(CPPPATH=[generated_dir])  # noqa: F821
