/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "lamp_fsm.h"

#include <stddef.h>

void lamp_fsm_init(lamp_state_t *state)
{
    state->on = true;
    state->color = LAMP_COLOR_GREEN;
    state->brightness_mode = BRIGHTNESS_MODE_AUTO;
    state->manual_level = LAMP_DEFAULT_MANUAL_LEVEL;
}

static lamp_color_t next_touch_color(lamp_color_t color)
{
    switch (color) {
        case LAMP_COLOR_GREEN:
            return LAMP_COLOR_ORANGE;
        case LAMP_COLOR_ORANGE:
            return LAMP_COLOR_RED;
        case LAMP_COLOR_RED:
        case LAMP_COLOR_WARM_WHITE:
        default:
            return LAMP_COLOR_GREEN;
    }
}

bool lamp_fsm_short_press(lamp_state_t *state)
{
    if (!state->on) {
        return false;
    }
    state->color = next_touch_color(state->color);
    return true;
}

bool lamp_fsm_long_press(lamp_state_t *state)
{
    state->on = !state->on;
    return true;
}

bool lamp_request_is_valid(const lamp_request_t *request)
{
    if ((request->fields & (uint8_t)~LAMP_FIELD_ALL) != 0u) {
        return false;
    }
    if ((request->fields & LAMP_FIELD_COLOR) != 0u &&
        (uint32_t)request->color >= LAMP_COLOR_COUNT) {
        return false;
    }
    if ((request->fields & LAMP_FIELD_BRIGHTNESS_MODE) != 0u &&
        request->brightness_mode != BRIGHTNESS_MODE_AUTO &&
        request->brightness_mode != BRIGHTNESS_MODE_MANUAL) {
        return false;
    }
    if ((request->fields & LAMP_FIELD_MANUAL_LEVEL) != 0u &&
        (request->manual_level < BRIGHTNESS_MANUAL_MIN ||
         request->manual_level > BRIGHTNESS_MANUAL_MAX)) {
        return false;
    }
    return true;
}

bool lamp_fsm_apply(lamp_state_t *state, const lamp_request_t *request)
{
    if (!lamp_request_is_valid(request)) {
        return false;
    }

    lamp_state_t next = *state;
    if ((request->fields & LAMP_FIELD_POWER) != 0u) {
        next.on = request->on;
    }
    if ((request->fields & LAMP_FIELD_COLOR) != 0u) {
        next.color = request->color;
    }
    if ((request->fields & LAMP_FIELD_BRIGHTNESS_MODE) != 0u) {
        next.brightness_mode = request->brightness_mode;
    }
    if ((request->fields & LAMP_FIELD_MANUAL_LEVEL) != 0u) {
        next.manual_level = request->manual_level;
    }

    const bool changed = next.on != state->on || next.color != state->color ||
                         next.brightness_mode != state->brightness_mode ||
                         next.manual_level != state->manual_level;
    *state = next;
    return changed;
}
