/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "scheduler.h"

#include "time_utils.h"

void scheduler_start(scheduler_task_t *tasks, size_t count, uint32_t now_ms)
{
    for (size_t i = 0; i < count; i++) {
        tasks[i].next_ms = now_ms;
    }
}

size_t scheduler_run(scheduler_task_t *tasks, size_t count, uint32_t now_ms)
{
    return scheduler_run_timed(tasks, count, now_ms, NULL);
}

size_t scheduler_run_timed(scheduler_task_t *tasks, size_t count, uint32_t now_ms,
                           scheduler_clock_us_t clock_us)
{
    size_t ran = 0;
    for (size_t i = 0; i < count; i++) {
        scheduler_task_t *task = &tasks[i];
        if (task->period_ms != 0u && !time_reached(now_ms, task->next_ms)) {
            continue;
        }
        if (clock_us != NULL) {
            const uint32_t start = clock_us();
            task->run(now_ms);
            const uint32_t elapsed = clock_us() - start;
            if (elapsed > task->max_us) {
                task->max_us = elapsed;
            }
        } else {
            task->run(now_ms);
        }
        ran++;
        if (task->period_ms != 0u) {
            task->next_ms += task->period_ms;
            /* Fell behind by more than a period: resynchronize. */
            if (time_reached(now_ms, task->next_ms)) {
                task->next_ms = now_ms + task->period_ms;
            }
        }
    }
    return ran;
}
