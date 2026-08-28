#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { VENTURED_ASR_TEXT_MAX = 360 };

typedef enum {
    VENTURED_ASR_START = 0,
    VENTURED_ASR_DONE,
    VENTURED_ASR_FAIL,
} ventured_asr_state_t;

typedef struct {
    uint32_t sequence;
    ventured_asr_state_t state;
    char text[VENTURED_ASR_TEXT_MAX + 1];
    char reason[12];
} ventured_asr_status_t;

const char *ventured_asr_state_token(ventured_asr_state_t state);
const char *ventured_asr_state_zh(ventured_asr_state_t state);
bool ventured_asr_extract_text(const char *json, char *out, size_t out_len);
bool ventured_format_asr_event(char *buffer, size_t capacity, const ventured_asr_status_t *status);
