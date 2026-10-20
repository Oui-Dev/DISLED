/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Capacitive touch detection on raw TouchKey readings.
 *
 * Signal chain:
 *  1. Raw samples are averaged over a window (20 ms at 1 kHz: one 50 Hz
 *     mains period, which cancels mains hum picked up by the finger).
 *  2. A baseline follows slow drifts (temperature, humidity, cable) while
 *     the electrode is not touched. It freezes during a touch and while a
 *     hand is near the lid (the proximity band below the press threshold),
 *     which would otherwise pull it down and blunt the next touches.
 *  3. The touch signal is the drop of the reading below the baseline, in
 *     per mille of the baseline, so thresholds do not depend on the board.
 *  4. Hysteresis and debouncing turn it into press/release, then into
 *     gestures: short press (released before the long-press time), long
 *     press (fires while the finger is still down). The thresholds are
 *     also relative to the floor, the lowest signal of the last
 *     TOUCH_FLOOR_WINDOWS windows: a press must rise above it, and ends
 *     halfway back. A hand resting near the lid lifts the signal to the
 *     level of a light touch (seen at bring-up), so fixed thresholds alone
 *     either merged the taps on top of it or held one long press. After a
 *     release, a new press needs a short re-arm delay.
 *  5. A contact longer than the object time is taken for an object lying on
 *     the lid: the baseline absorbs it and nothing more fires. A touch on
 *     top of the object still works, with less sensitivity.
 *  6. A reading that stays above the baseline (finger present at power-up,
 *     object removed) recalibrates the baseline.
 */
#ifndef DISLED_TOUCH_DETECTOR_H
#define DISLED_TOUCH_DETECTOR_H

#include <stdbool.h>
#include <stdint.h>

/* Windows that define the floor: 16 x 20 ms. A tap rises in 40-100 ms, a
 * hand settling near the lid more slowly. */
#define TOUCH_FLOOR_WINDOWS 16u

typedef enum {
    TOUCH_EVENT_NONE = 0,
    TOUCH_EVENT_SHORT_PRESS,
    TOUCH_EVENT_LONG_PRESS,
} touch_event_t;

typedef enum {
    TOUCH_STATE_CALIBRATING = 0, /* building the first baseline */
    TOUCH_STATE_IDLE,
    TOUCH_STATE_PRESSED,
} touch_state_t;

typedef struct {
    uint16_t window_samples;     /* raw samples averaged per window */
    uint8_t calibration_windows; /* windows averaged into the first baseline */
    uint8_t baseline_shift;      /* baseline time constant: 2^shift windows */
    uint16_t press_permille;     /* drop that starts a touch */
    uint16_t release_permille;   /* drop under which the touch ends */
    /* Rise above the floor that starts a touch; it ends at half of it. */
    uint16_t press_rise_permille;
    uint16_t proximity_permille; /* drop that freezes the baseline (hand near) */
    uint16_t debounce_ms;        /* a change must hold this long */
    uint16_t rearm_ms;           /* after a release, no new press before this */
    uint16_t long_press_ms;      /* contact time of a long press */
    uint16_t object_ms;          /* contact or proximity time that means "object" */
    uint16_t inverted_permille;  /* rise above baseline that means "recalibrate" */
    uint16_t inverted_ms;        /* how long the rise must hold */
    bool rising_touch;           /* set if a touch raises the reading */
} touch_config_t;

typedef struct {
    const touch_config_t *config;
    touch_state_t state;
    /* Window accumulator. */
    uint32_t window_sum;
    uint16_t window_count;
    uint16_t last_raw;
    uint16_t value; /* last window average */
    /* Baseline in 1/256 counts, for a smooth fixed-point filter. */
    uint32_t baseline_q8;
    uint32_t calibration_sum;
    uint8_t calibration_count;
    int16_t delta_permille; /* positive while touched */
    /* Debounced contact tracking. */
    bool contact; /* debounced contact */
    bool pending; /* raw contact differs from `contact` */
    uint32_t pending_since_ms;
    uint32_t press_start_ms;
    /* Recent drops, for the floor; the floor is latched while touched. */
    int16_t recent[TOUCH_FLOOR_WINDOWS];
    uint8_t recent_next;
    int16_t press_floor;
    bool long_fired;
    bool rearming; /* a release happened less than rearm_ms ago */
    uint32_t release_ms;
    bool inverted;
    uint32_t inverted_since_ms;
    bool near; /* in the proximity band, baseline frozen */
    uint32_t near_since_ms;
    uint16_t recalibrations; /* automatic re-baselines since init */
} touch_detector_t;

void touch_detector_init(touch_detector_t *td, const touch_config_t *config);

/* Feeds one raw sample. Returns the gesture completed by this sample, if
 * any. `now_ms` is the time of the sample. */
touch_event_t touch_detector_feed(touch_detector_t *td, uint16_t raw, uint32_t now_ms);

/* Restarts the calibration, for instance after the electrode was touched at
 * power-up or when resuming from USB suspend. */
void touch_detector_recalibrate(touch_detector_t *td);

touch_state_t touch_detector_state(const touch_detector_t *td);
uint16_t touch_detector_raw(const touch_detector_t *td);
uint16_t touch_detector_value(const touch_detector_t *td);
uint16_t touch_detector_baseline(const touch_detector_t *td);
int16_t touch_detector_delta_permille(const touch_detector_t *td);

/* Automatic re-baselines since init: finger at power-up, object on or near
 * the lid (diagnostics). */
uint16_t touch_detector_recalibrations(const touch_detector_t *td);

#endif /* DISLED_TOUCH_DETECTOR_H */
