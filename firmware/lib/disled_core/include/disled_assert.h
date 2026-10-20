/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Invariant checks. A failed check calls disled_assert_failed(), which the
 * firmware provides (it records a fault, reported at the next boot, and
 * resets the chip); the native test build aborts.
 */
#ifndef DISLED_ASSERT_H
#define DISLED_ASSERT_H

#include <stdint.h>

#define DISLED_ASSERT(condition)                      \
    do {                                              \
        if (!(condition)) {                           \
            disled_assert_failed((uint32_t)__LINE__); \
        }                                             \
    } while (0)

__attribute__((noreturn)) void disled_assert_failed(uint32_t line);

#endif /* DISLED_ASSERT_H */
