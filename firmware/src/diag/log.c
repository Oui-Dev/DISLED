/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "log.h"

#if DISLED_DEBUG

#include <stdarg.h>

#include "bsp/bsp_time.h"
#include "fmt.h"
#include "usb/usb_device.h"

#define LINE_MAX 128u

static log_level_t s_level = LOG_LEVEL_INFO;

void log_set_level(log_level_t level)
{
    s_level = level;
}

void log_write(log_level_t level, const char *format, ...)
{
    static const char tags[] = { 'E', 'W', 'I' };
    if (level > s_level) {
        return;
    }

    char line[LINE_MAX];
    const uint32_t now = bsp_time_ms();
    size_t length =
        fmt_snprintf(line, sizeof line, "[%6lu.%03lu] %c ", (unsigned long)(now / 1000u),
                     (unsigned long)(now % 1000u), tags[level]);
    if (length < sizeof line) {
        va_list args;
        va_start(args, format);
        length += fmt_vsnprintf(&line[length], sizeof line - length, format, args);
        va_end(args);
    }
    /* Keep room for the line ending, even when truncated. */
    if (length > sizeof line - 3u) {
        length = sizeof line - 3u;
    }
    line[length++] = '\r';
    line[length++] = '\n';
    usb_cdc_write(line, length);
}

#endif
