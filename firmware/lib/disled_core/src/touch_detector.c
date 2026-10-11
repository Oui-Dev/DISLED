/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "touch_detector.h"

#include <stddef.h>

#include "time_utils.h"

static void reset_contact(touch_detector_t *td)
{
    td->contact = false;
    td->pending = false;
    td->long_fired = false;
    td->inverted = false;
    td->delta_permille = 0;
}

void touch_detector_init(touch_detector_t *td, const touch_config_t *config)
{
    td->config = config;
    td->window_sum = 0;
    td->window_count = 0;
    td->last_raw = 0;
    td->value = 0;
    td->baseline_q8 = 0;
    td->press_start_ms = 0;
    td->pending_since_ms = 0;
    td->inverted_since_ms = 0;
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
    int32_t drop = baseline - (int32_t)td->value;
    if (td->config->rising_touch) {
        drop = -drop;
    }
    int32_t permille = (drop * 1000) / baseline;
    if (permille > INT16_MAX) {
        permille = INT16_MAX;
    } else if (permille < INT16_MIN) {
        permille = INT16_MIN;
    }
    return (int16_t)permille;
}

/* Follows slow drifts and recalibrates when the reading stays above the
 * baseline. Only called while the electrode is not touched. */
static void track_baseline(touch_detector_t *td, uint32_t now_ms)
{
    const touch_config_t *cfg = td->config;

    if (td->delta_permille <= -(int32_t)cfg->inverted_permille) {
        if (!td->inverted) {
            td->inverted = true;
            td->inverted_since_ms = now_ms;
        } else if (time_elapsed_ms(now_ms, td->inverted_since_ms) >= cfg->inverted_ms) {
            td->baseline_q8 = (uint32_t)td->value << 8;
            td->inverted = false;
        }
        return;
    }
    td->inverted = false;

    /* First-order low-pass: baseline += error / 2^shift. Division instead
     * of a shift keeps the rounding well defined for negative errors. */
    const int32_t target_q8 = (int32_t)((uint32_t)td->value << 8);
    const int32_t error = target_q8 - (int32_t)td->baseline_q8;
    const int32_t divisor = (int32_t)1 << cfg->baseline_shift;
    td->baseline_q8 = (uint32_t)((int32_t)td->baseline_q8 + (error / divisor));
}

/* Hysteresis and debounce. Returns true when the debounced contact flips. */
static bool update_contact(touch_detector_t *td, uint32_t now_ms)
{
    const touch_config_t *cfg = td->config;
    const bool raw_contact = td->contact ? (td->delta_permille > (int32_t)cfg->release_permille)
                                         : (td->delta_permille >= (int32_t)cfg->press_permille);

    if (raw_contact == td->contact) {
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
    td->contact = raw_contact;
    td->pending = false;
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
    const uint32_t contact_start_ms = td->pending_since_ms;
    const bool flipped = update_contact(td, now_ms);

    if (td->state == TOUCH_STATE_IDLE) {
        if (flipped && td->contact) {
            /* Time the press from the first raw contact, not from the end
             * of the debounce. */
            td->state = TOUCH_STATE_PRESSED;
            td->press_start_ms = contact_start_ms;
            td->long_fired = false;
        } else if (!td->contact && !td->pending) {
            track_baseline(td, now_ms);
        }
        return TOUCH_EVENT_NONE;
    }

    /* TOUCH_STATE_PRESSED: the baseline is frozen. */
    if (flipped && !td->contact) {
        td->state = TOUCH_STATE_IDLE;
        return td->long_fired ? TOUCH_EVENT_NONE : TOUCH_EVENT_SHORT_PRESS;
    }

    const uint32_t held_ms = time_elapsed_ms(now_ms, td->press_start_ms);
    if (held_ms >= cfg->object_ms) {
        /* Object on the lid: absorb it into the baseline. */
        td->baseline_q8 = (uint32_t)td->value << 8;
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
