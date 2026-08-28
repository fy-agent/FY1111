#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    VENTURED_REC_START = 0,
    VENTURED_REC_ACTIVE,
    VENTURED_REC_DONE,
    VENTURED_REC_FAIL,
} ventured_rec_state_t;

typedef struct {
    uint32_t sequence;
    ventured_rec_state_t state;
    uint32_t ms;
    uint32_t samples;
    uint32_t rms;
    uint32_t peak;
    bool silence;
    char reason[8];
} ventured_rec_status_t;

const char *ventured_rec_state_token(ventured_rec_state_t state);
const char *ventured_rec_state_zh(ventured_rec_state_t state);
bool ventured_format_rec_event(char *buffer, size_t capacity, const ventured_rec_status_t *status);
