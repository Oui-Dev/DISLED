/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 *
 * Lamp state machine: power, color and brightness settings, from the touch
 * gestures and the host requests (docs/behavior.md). USB suspend stays out of
 * it: the application drops the gestures and the LED output goes dark.
 */
#ifndef DISLED_LAMP_FSM_H
#define DISLED_LAMP_FSM_H

#include <stdbool.h>
#include <stdint.h>

#include "disled_types.h"

/* Manual level used until the host chooses one. */
#define LAMP_DEFAULT_MANUAL_LEVEL 50u

typedef struct {
    bool on;
    /* Shown when on; kept while off so that the lamp comes back with it. */
    lamp_color_t color;
    brightness_mode_t brightness_mode;
    /* Percent, BRIGHTNESS_MANUAL_MIN to BRIGHTNESS_MANUAL_MAX. */
    uint8_t manual_level;
} lamp_state_t;

/* Fields of a host request. The values are part of the protocol. */
#define LAMP_FIELD_POWER           0x01u
#define LAMP_FIELD_COLOR           0x02u
#define LAMP_FIELD_BRIGHTNESS_MODE 0x04u
#define LAMP_FIELD_MANUAL_LEVEL    0x08u
#define LAMP_FIELD_ALL             0x0Fu

typedef struct {
    uint8_t fields; /* LAMP_FIELD_* bits: which members below apply */
    bool on;
    lamp_color_t color;
    brightness_mode_t brightness_mode;
    uint8_t manual_level;
} lamp_request_t;

/* Power-up state: on, green, automatic brightness. */
void lamp_fsm_init(lamp_state_t *state);

/* The next color while on (docs/behavior.md), nothing while off. Returns true if
 * the state changed. */
bool lamp_fsm_short_press(lamp_state_t *state);

/* Toggles the power, keeping the color: the state always changes. */
void lamp_fsm_long_press(lamp_state_t *state);

/* True if every selected field holds a valid value and no unknown field
 * bit is set. */
bool lamp_request_is_valid(const lamp_request_t *request);

bool lamp_state_equal(const lamp_state_t *a, const lamp_state_t *b);

/* Applies a valid request atomically and returns true if the state changed.
 * An invalid request changes nothing and returns false. */
bool lamp_fsm_apply(lamp_state_t *state, const lamp_request_t *request);

#endif /* DISLED_LAMP_FSM_H */
