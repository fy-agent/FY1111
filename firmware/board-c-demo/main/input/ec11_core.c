#include "ec11_core.h"

#include <limits.h>
#include <stddef.h>

#define VENTURED_EC11_INVALID_TRANSITION 2

/* Retained from the verified pure EC11 core: invalid two-bit jumps reset a
 * partial detent instead of inventing a semantic encoder event. */
static const int8_t s_transition_delta[16] = {
    0, 1, -1, VENTURED_EC11_INVALID_TRANSITION,
    -1, 0, VENTURED_EC11_INVALID_TRANSITION, 1,
    1, VENTURED_EC11_INVALID_TRANSITION, 0, -1,
    VENTURED_EC11_INVALID_TRANSITION, -1, 1, 0,
};

void ventured_ec11_init(ventured_ec11_t *encoder, uint8_t initial_ab,
                        bool invert_direction) {
    if (encoder == NULL) return;
    *encoder = (ventured_ec11_t){.previous_ab = initial_ab & 0x03U,
                                 .anchor_ab = initial_ab & 0x03U,
                                 .invert_direction = invert_direction};
}

bool ventured_ec11_update(ventured_ec11_t *encoder, uint8_t normalized_ab,
                           ventured_input_t *event) {
    if (encoder == NULL || event == NULL) return false;
    if (normalized_ab > 0x03U) {
        if (encoder->invalid_transition_count < UINT32_MAX) ++encoder->invalid_transition_count;
        encoder->quarter_steps = 0;
        return false;
    }
    uint8_t previous = encoder->previous_ab;
    if (normalized_ab == previous) return false;
    encoder->previous_ab = normalized_ab;
    int8_t delta = s_transition_delta[(previous << 2U) | normalized_ab];
    if (delta == VENTURED_EC11_INVALID_TRANSITION) {
        if (encoder->invalid_transition_count < UINT32_MAX) ++encoder->invalid_transition_count;
        encoder->quarter_steps = 0;
        return false;
    }
    int16_t next = (int16_t)encoder->quarter_steps + delta;
    if (next > VENTURED_EC11_COMPLETE_DETENT_STEPS) next = VENTURED_EC11_COMPLETE_DETENT_STEPS;
    if (next < -VENTURED_EC11_COMPLETE_DETENT_STEPS) next = -VENTURED_EC11_COMPLETE_DETENT_STEPS;
    encoder->quarter_steps = (int8_t)next;
    if (encoder->quarter_steps != VENTURED_EC11_COMPLETE_DETENT_STEPS &&
        encoder->quarter_steps != -VENTURED_EC11_COMPLETE_DETENT_STEPS) {
        return false;
    }
    int8_t completed = encoder->quarter_steps;
    encoder->quarter_steps = 0;
    encoder->anchor_ab = normalized_ab;
    bool clockwise = completed == VENTURED_EC11_COMPLETE_DETENT_STEPS;
    *event = (encoder->invert_direction ? !clockwise : clockwise)
                 ? VENTURED_INPUT_ENCODER_CW : VENTURED_INPUT_ENCODER_CCW;
    return true;
}

uint16_t ventured_button_required_samples(uint32_t scan_period_ms,
                                          uint32_t debounce_ms) {
    uint32_t period = scan_period_ms == 0U ? 1U : scan_period_ms;
    uint64_t samples = ((uint64_t)debounce_ms + period - 1U) / period + 1U;
    return samples > UINT16_MAX ? UINT16_MAX : (uint16_t)samples;
}

void ventured_button_init(ventured_button_t *button, bool initially_pressed,
                          uint16_t required_samples) {
    if (button == NULL) return;
    *button = (ventured_button_t){.stable_pressed = initially_pressed,
                                  .candidate_pressed = initially_pressed,
                                  .required_samples = required_samples == 0U ? 1U : required_samples};
}

ventured_button_transition_t ventured_button_update(ventured_button_t *button,
                                                     bool raw_pressed) {
    if (button == NULL) return VENTURED_BUTTON_NO_CHANGE;
    if (raw_pressed == button->stable_pressed) {
        button->candidate_pressed = button->stable_pressed;
        button->consecutive_samples = 0U;
        return VENTURED_BUTTON_NO_CHANGE;
    }
    if (raw_pressed != button->candidate_pressed) {
        button->candidate_pressed = raw_pressed;
        button->consecutive_samples = 1U;
    } else if (button->consecutive_samples < UINT16_MAX) {
        ++button->consecutive_samples;
    }
    if (button->consecutive_samples < button->required_samples) return VENTURED_BUTTON_NO_CHANGE;
    button->stable_pressed = raw_pressed;
    button->candidate_pressed = raw_pressed;
    button->consecutive_samples = 0U;
    return raw_pressed ? VENTURED_BUTTON_STABLE_PRESSED : VENTURED_BUTTON_STABLE_RELEASED;
}

