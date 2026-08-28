#include "input_event.h"

#include <inttypes.h>
#include <stdio.h>

const char *ventured_input_token(ventured_input_t input) {
    switch (input) {
        case VENTURED_INPUT_ENCODER_CW: return "ENCODER_CW";
        case VENTURED_INPUT_ENCODER_CCW: return "ENCODER_CCW";
        case VENTURED_INPUT_ENCODER_PRESS: return "ENCODER_PRESS";
        case VENTURED_INPUT_BUTTON_A: return "BUTTON_A";
        case VENTURED_INPUT_BUTTON_B: return "BUTTON_B";
        default: return NULL;
    }
}

bool ventured_format_input_event(char *buffer, size_t capacity, uint32_t sequence,
                                 ventured_input_t input) {
    const char *token = ventured_input_token(input);
    if (buffer == NULL || capacity == 0U || token == NULL) return false;
    int written = snprintf(buffer, capacity, "VKEY_INPUT/1 {\"seq\":%" PRIu32 ",\"input\":\"%s\"}\n", sequence, token);
    return written >= 0 && (size_t)written < capacity;
}
