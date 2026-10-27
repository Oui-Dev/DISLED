/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "touch_detector.h"

#include <stddef.h>

#include "time_utils.h"

static void rebaseline(touch_detector_t *td)
{
    td->baseline_q8 = (uint32_t)td->value << 8;
    if (td->recalibrations < UINT16_MAX) {
        td->recalibrations++;
    }
}

static void reset_contact(touch_detector_t *td)
{
    td->pending = false;
    td->long_fired = false;
    td->rearming = false;
    td->inverted = false;
    td->near = false;
    td->delta_permille = 0;
    for (size_t i = 0; i < TOUCH_FLOOR_WINDOWS; i++) {
        td->recent[i] = 0;
    }
    td->recent_next = 0;
    td->press_floor = 0;
}

void touch_detector_init(touch_detector_t *td, const touch_config_t *config)
{
    *td = (touch_detector_t){ .config = config };
    touch_detector_recalibrate(td);
}

void touch_detector_recalibrate(touch_detector_t *td)
{
    td->state = TOUCH_STATE_CALIBRATING;
    td->calibration_sum = 0;
    td->calibration_count = 0;
    reset_contact(td);
}

static uint16_t baseline_counts(const touch_detector_t *td)
{
    return (uint16_t)((td->baseline_q8 + 128u) >> 8);
}

static int16_t compute_delta_permille(const touch_detector_t *td)
{
    const int32_t baseline = (int32_t)baseline_counts(td);
    if (baseline == 0) {
        return 0;
    }
    const int32_t drop = baseline - (int32_t)td->value;
    /* At most 1000, since the drop never exceeds the baseline; a reading far
     * above a small baseline (a failing electrode) goes far below. */
    int32_t permille = (drop * 1000) / baseline;
    if (permille < INT16_MIN) {
        permille = INT16_MIN;
    }
    return (int16_t)permille;
}

/* Called only while nothing touches the electrode. */
static void track_baseline(touch_detector_t *td, uint32_t now_ms)
{
    const touch_config_t *cfg = td->config;

    /* A hand near the lid would pull the baseline down and blunt the next
     * touches: hold it, unless the hand stays as long as an object would. */
    if (td->delta_permille >= (int32_t)cfg->proximity_permille) {
        td->inverted = false;
        if (!td->near) {
            td->near = true;
            td->near_since_ms = now_ms;
        } else if (time_elapsed_ms(now_ms, td->near_since_ms) >= cfg->object_ms) {
            rebaseline(td);
            td->near = false;
        }
        return;
    }
    td->near = false;

    if (td->delta_permille <= -(int32_t)cfg->inverted_permille) {
        if (!td->inverted) {
            td->inverted = true;
            td->inverted_since_ms = now_ms;
        } else if (time_elapsed_ms(now_ms, td->inverted_since_ms) >= cfg->inverted_ms) {
            rebaseline(td);
            td->inverted = false;
        }
        return;
    }
    td->inverted = false;

    /* Divided, not shifted: shifting a negative value is implementation-defined. */
    const int32_t target_q8 = (int32_t)((uint32_t)td->value << 8);
    const int32_t error = target_q8 - (int32_t)td->baseline_q8;
    const int32_t divisor = (int32_t)1 << cfg->baseline_shift;
    td->baseline_q8 = (uint32_t)((int32_t)td->baseline_q8 + (error / divisor));
}

/* Lowest drop of the last TOUCH_FLOOR_WINDOWS windows. */
static int16_t recent_floor(const touch_detector_t *td)
{
    int16_t floor = td->recent[0];
    for (size_t i = 1; i < TOUCH_FLOOR_WINDOWS; i++) {
        if (td->recent[i] < floor) {
            floor = td->recent[i];
        }
    }
    return floor;
}

static int32_t max_i32(int32_t a, int32_t b)
{
    return a > b ? a : b;
}

/* Hysteresis and debounce. Returns true when the debounced contact flips. */
static bool update_contact(touch_detector_t *td, uint32_t now_ms)
{
    const touch_config_t *cfg = td->config;
    const bool contact = td->state == TOUCH_STATE_PRESSED;
    const int32_t rise = (int32_t)cfg->press_rise_permille;
    const int32_t press = max_i32((int32_t)cfg->press_permille, (int32_t)recent_floor(td) + rise);
    const int32_t release =
        max_i32((int32_t)cfg->release_permille, (int32_t)td->press_floor + rise / 2);
    bool raw_contact = contact ? (td->delta_permille > release) : (td->delta_permille >= press);

    if (td->rearming) {
        if (time_elapsed_ms(now_ms, td->release_ms) < cfg->rearm_ms) {
            raw_contact = false; /* too soon after the last release */
        } else {
            td->rearming = false;
        }
    }

    if (raw_contact == contact) {
        td->pending = false;
        return false;
    }
    if (!td->pending) {
        td->pending = true;
        td->pending_since_ms = now_ms;
        return false;
    }
    if (time_elapsed_ms(now_ms, td->pending_since_ms) < cfg->debounce_ms) {
        return false;
    }
    td->pending = false;
    if (!raw_contact) {
        td->rearming = true;
        td->release_ms = now_ms;
    }
    return true;
}

static touch_event_t process_window(touch_detector_t *td, uint32_t now_ms)
{
    const touch_config_t *cfg = td->config;

    if (td->state == TOUCH_STATE_CALIBRATING) {
        td->calibration_sum += td->value;
        td->calibration_count++;
        if (td->calibration_count >= cfg->calibration_windows) {
            td->baseline_q8 = (td->calibration_sum / td->calibration_count) << 8;
            td->state = TOUCH_STATE_IDLE;
            reset_contact(td);
        }
        return TOUCH_EVENT_NONE;
    }

    td->delta_permille = compute_delta_permille(td);
    td->recent[td->recent_next] = td->delta_permille;
    td->recent_next = (uint8_t)((td->recent_next + 1u) % TOUCH_FLOOR_WINDOWS);
    const uint32_t contact_start_ms = td->pending_since_ms;
    const bool flipped = update_contact(td, now_ms);

    if (td->state == TOUCH_STATE_IDLE) {
        if (flipped) {
            /* Timed from the first raw contact, not from the end of the debounce. */
            td->state = TOUCH_STATE_PRESSED;
            td->press_start_ms = contact_start_ms;
            td->long_fired = false;
            td->near = false;
            td->press_floor = recent_floor(td);
        } else if (!td->pending) {
            track_baseline(td, now_ms);
        }
        return TOUCH_EVENT_NONE;
    }

    /* TOUCH_STATE_PRESSED: the baseline is frozen. */
    if (flipped) {
        td->state = TOUCH_STATE_IDLE;
        return td->long_fired ? TOUCH_EVENT_NONE : TOUCH_EVENT_SHORT_PRESS;
    }

    const uint32_t held_ms = time_elapsed_ms(now_ms, td->press_start_ms);
    if (held_ms >= cfg->object_ms) {
        /* Object on the lid: absorb it into the baseline. */
        rebaseline(td);
        td->state = TOUCH_STATE_IDLE;
        reset_contact(td);
        return TOUCH_EVENT_NONE;
    }
    if (!td->long_fired && held_ms >= cfg->long_press_ms) {
        td->long_fired = true;
        return TOUCH_EVENT_LONG_PRESS;
    }
    return TOUCH_EVENT_NONE;
}

touch_event_t touch_detector_feed(touch_detector_t *td, uint16_t raw, uint32_t now_ms)
{
    td->last_raw = raw;
    td->window_sum += raw;
    td->window_count++;
    if (td->window_count < td->config->window_samples) {
        return TOUCH_EVENT_NONE;
    }

    td->value = (uint16_t)((td->window_sum + (td->window_count / 2u)) / td->window_count);
    td->window_sum = 0;
    td->window_count = 0;
    return process_window(td, now_ms);
}

touch_state_t touch_detector_state(const touch_detector_t *td)
{
    return td->state;
}

uint16_t touch_detector_raw(const touch_detector_t *td)
{
    return td->last_raw;
}

uint16_t touch_detector_value(const touch_detector_t *td)
{
    return td->value;
}

uint16_t touch_detector_baseline(const touch_detector_t *td)
{
    return baseline_counts(td);
}

int16_t touch_detector_delta_permille(const touch_detector_t *td)
{
    return td->delta_permille;
}

uint16_t touch_detector_recalibrations(const touch_detector_t *td)
{
    return td->recalibrations;
}

void touch_detector_consume(touch_detector_t *td)
{
    if (td->state == TOUCH_STATE_PRESSED) {
        td->long_fired = true; /* no long press, no event at the release */
    }
}
