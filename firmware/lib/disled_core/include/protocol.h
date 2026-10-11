/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * USB HID protocol v1: wire constants. The specification lives in
 * protocol/README.md at the repository root.
 */
#ifndef DISLED_PROTOCOL_H
#define DISLED_PROTOCOL_H

#define PROTO_VERSION_MAJOR 1u
#define PROTO_VERSION_MINOR 0u

/* HID report IDs: commands go host -> device, messages device -> host. */
#define PROTO_REPORT_ID_COMMAND 1u
#define PROTO_REPORT_ID_MESSAGE 2u

/* Payload bytes of one report, after the report ID. */
#define PROTO_REPORT_SIZE 63u

#endif /* DISLED_PROTOCOL_H */
