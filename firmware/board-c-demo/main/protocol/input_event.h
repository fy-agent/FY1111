#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    VENTURED_INPUT_ENCODER_CW = 0,
    VENTURED_INPUT_ENCODER_CCW,
    VENTURED_INPUT_ENCODER_PRESS,
} ventured_input_t;

const char *ventured_input_token(ventured_input_t input);
bool ventured_format_input_event(char *buffer, size_t capacity, uint32_t sequence,
                                 ventured_input_t input);

