/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Table-driven cooperative scheduler. Tasks run to completion from the main
 * loop; interrupt handlers only capture data. A task with period 0 runs on
 * every pass. A late task runs once and keeps its period from that point,
 * so it never fires in a burst to catch up.
 */
#ifndef DISLED_SCHEDULER_H
#define DISLED_SCHEDULER_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char *name;
    uint32_t period_ms;
    void (*run)(uint32_t now_ms);
    uint32_t next_ms; /* managed by the scheduler */
    uint32_t max_us;  /* longest run, in microseconds */
} scheduler_task_t;

/* Microsecond clock for scheduler_run(); it may wrap. */
typedef uint32_t (*scheduler_clock_us_t)(void);

/* Makes every task due at `now_ms`. */
void scheduler_start(scheduler_task_t *tasks, size_t count, uint32_t now_ms);

/* Runs the tasks that are due, in table order, and records the longest run
 * of each task in `max_us`, to check the margin left to the watchdog. Every
 * run is timed with `clock_us`, which must not be NULL. */
void scheduler_run(scheduler_task_t *tasks, size_t count, uint32_t now_ms,
                   scheduler_clock_us_t clock_us);

#endif /* DISLED_SCHEDULER_H */
