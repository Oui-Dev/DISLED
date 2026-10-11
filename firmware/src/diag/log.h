/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Logging to the CDC console. Debug builds only: in release builds the
 * macros compile to nothing (their arguments are still type-checked).
 *
 *   LOG_INFO("touch: short press, color %u", color);
 * prints
 *   [  12.345] I touch: short press, color 1
 */
#ifndef DISLED_LOG_H
#define DISLED_LOG_H

typedef enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
} log_level_t;

#if DISLED_DEBUG

__attribute__((format(printf, 2, 3))) void log_write(log_level_t level, const char *format, ...);
void log_set_level(log_level_t level);
log_level_t log_level(void);

#define LOG_ERROR(...) log_write(LOG_LEVEL_ERROR, __VA_ARGS__)
#define LOG_WARN(...)  log_write(LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_INFO(...)  log_write(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_DEBUG(...) log_write(LOG_LEVEL_DEBUG, __VA_ARGS__)

#else

__attribute__((format(printf, 1, 2))) static inline void log_discard(const char *format, ...)
{
    (void)format;
}

#define LOG_ERROR(...) log_discard(__VA_ARGS__)
#define LOG_WARN(...)  log_discard(__VA_ARGS__)
#define LOG_INFO(...)  log_discard(__VA_ARGS__)
#define LOG_DEBUG(...) log_discard(__VA_ARGS__)

#endif

#endif /* DISLED_LOG_H */
