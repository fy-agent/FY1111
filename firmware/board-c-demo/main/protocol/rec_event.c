#include "rec_event.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

const char *ventured_rec_state_token(ventured_rec_state_t state) {
    switch (state) {
        case VENTURED_REC_START: return "START";
        case VENTURED_REC_ACTIVE: return "ACTIVE";
        case VENTURED_REC_DONE: return "DONE";
        case VENTURED_REC_FAIL: return "FAIL";
        default: return NULL;
    }
}

const char *ventured_rec_state_zh(ventured_rec_state_t state) {
    switch (state) {
        case VENTURED_REC_START:
        case VENTURED_REC_ACTIVE: return "录音中";
        case VENTURED_REC_DONE: return "完成";
        case VENTURED_REC_FAIL: return "失败";
        default: return "失败";
    }
}

static bool reason_allowed(const char *reason) {
    return strcmp(reason, "I2S") == 0 ||
           strcmp(reason, "BUSY") == 0 ||
           strcmp(reason, "WIFI") == 0 ||
           strcmp(reason, "UNKNOWN") == 0;
}

bool ventured_format_rec_event(char *buffer, size_t capacity, const ventured_rec_status_t *status) {
    const char *state = status == NULL ? NULL : ventured_rec_state_token(status->state);
    if (buffer == NULL || capacity == 0U || state == NULL) return false;
    int written = snprintf(buffer, capacity,
                           "VKEY_REC/1 {\"seq\":%" PRIu32 ",\"state\":\"%s\",\"ms\":%" PRIu32
                           ",\"samples\":%" PRIu32 ",\"rms\":%" PRIu32 ",\"peak\":%" PRIu32,
                           status->sequence, state, status->ms, status->samples, status->rms,
                           status->peak);
    if (written < 0 || (size_t)written >= capacity) return false;
    size_t used = (size_t)written;
    if (status->state == VENTURED_REC_DONE) {
        written = snprintf(buffer + used, capacity - used, ",\"silence\":%s}\n",
                           status->silence ? "true" : "false");
    } else if (status->state == VENTURED_REC_FAIL && status->reason[0] != '\0' &&
               reason_allowed(status->reason)) {
        written = snprintf(buffer + used, capacity - used, ",\"reason\":\"%s\"}\n", status->reason);
    } else {
        written = snprintf(buffer + used, capacity - used, "}\n");
    }
    return written >= 0 && (size_t)written < capacity - used;
}
