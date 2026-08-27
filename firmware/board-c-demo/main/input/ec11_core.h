#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "input_event.h"

#define VENTURED_EC11_COMPLETE_DETENT_STEPS 4

typedef struct {
    uint8_t previous_ab;
    uint8_t anchor_ab;
    int8_t quarter_steps;
    bool invert_direction;
    uint32_t invalid_transition_count;
} ventured_ec11_t;

typedef enum {
    VENTURED_BUTTON_NO_CHANGE = 0,
    VENTURED_BUTTON_STABLE_PRESSED,
    VENTURED_BUTTON_STABLE_RELEASED,
} ventured_button_transition_t;

typedef struct {
    bool stable_pressed;
    bool candidate_pressed;
    uint16_t consecutive_samples;
    uint16_t required_samples;
} ventured_button_t;

void ventured_ec11_init(ventured_ec11_t *encoder, uint8_t initial_ab,
                        bool invert_direction);
bool ventured_ec11_update(ventured_ec11_t *encoder, uint8_t normalized_ab,
                           ventured_input_t *event);
uint16_t ventured_button_required_samples(uint32_t scan_period_ms,
                                          uint32_t debounce_ms);
void ventured_button_init(ventured_button_t *button, bool initially_pressed,
                          uint16_t required_samples);
ventured_button_transition_t ventured_button_update(ventured_button_t *button,
                                                     bool raw_pressed);

