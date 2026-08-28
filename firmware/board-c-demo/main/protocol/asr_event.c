#include "asr_event.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

const char *ventured_asr_state_token(ventured_asr_state_t state) {
    switch (state) {
        case VENTURED_ASR_START: return "START";
        case VENTURED_ASR_DONE: return "DONE";
        case VENTURED_ASR_FAIL: return "FAIL";
        default: return NULL;
    }
}

const char *ventured_asr_state_zh(ventured_asr_state_t state) {
    switch (state) {
        case VENTURED_ASR_START: return "转写";
        case VENTURED_ASR_DONE: return "转写完成";
        case VENTURED_ASR_FAIL:
        default: return "转写失败";
    }
}

bool ventured_asr_extract_text(const char *json, char *out, size_t out_len) {
    if (json == NULL || out == NULL || out_len == 0) return false;
    const char *cursor = strstr(json, "\"text\"");
    if (cursor == NULL) return false;
    cursor = strchr(cursor + 6, ':');
    if (cursor == NULL) return false;
    ++cursor;
    while (*cursor == ' ' || *cursor == '\t') ++cursor;
    if (*cursor != '"') return false;
    ++cursor;
    size_t written = 0;
    while (*cursor != '\0' && *cursor != '"' && written + 1 < out_len) {
        char ch = *cursor++;
        if (ch == '\\' && *cursor != '\0') {
            ch = *cursor++;
            if (ch == 'n' || ch == 'r' || ch == 't') ch = ' ';
        }
        if ((unsigned char)ch < 32) continue;
        out[written++] = ch;
    }
    out[written] = '\0';
    return written > 0;
}

static size_t json_escape(char *dst, size_t capacity, const char *src) {
    size_t written = 0;
    if (dst == NULL || capacity == 0) return 0;
    while (src != NULL && *src != '\0' && written + 2 < capacity) {
        char ch = *src++;
        if (ch == '"' || ch == '\\') {
            if (written + 3 >= capacity) break;
            dst[written++] = '\\';
            dst[written++] = ch;
            continue;
        }
        if ((unsigned char)ch < 32) continue;
        dst[written++] = ch;
    }
    dst[written] = '\0';
    return written;
}

bool ventured_format_asr_event(char *buffer, size_t capacity, const ventured_asr_status_t *status) {
    const char *state = status == NULL ? NULL : ventured_asr_state_token(status->state);
    if (buffer == NULL || capacity == 0U || state == NULL) return false;
    if (status->state == VENTURED_ASR_DONE) {
        char escaped[VENTURED_ASR_TEXT_MAX * 2 + 8];
        json_escape(escaped, sizeof(escaped), status->text);
        int written = snprintf(buffer, capacity, "VKEY_ASR/1 {\"seq\":%" PRIu32 ",\"state\":\"DONE\",\"text\":\"%s\"}\n",
                               status->sequence, escaped);
        return written > 0 && (size_t)written < capacity;
    }
    if (status->state == VENTURED_ASR_FAIL && status->reason[0] != '\0') {
        int written = snprintf(buffer, capacity, "VKEY_ASR/1 {\"seq\":%" PRIu32 ",\"state\":\"FAIL\",\"reason\":\"%s\"}\n",
                               status->sequence, status->reason);
        return written > 0 && (size_t)written < capacity;
    }
    int written = snprintf(buffer, capacity, "VKEY_ASR/1 {\"seq\":%" PRIu32 ",\"state\":\"%s\"}\n",
                           status->sequence, state);
    return written > 0 && (size_t)written < capacity;
}
