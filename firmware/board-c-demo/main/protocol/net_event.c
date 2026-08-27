#include "net_event.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

const char *ventured_net_state_token(ventured_net_state_t state) {
    switch (state) {
        case VENTURED_NET_DISCONNECTED: return "DISCONNECTED";
        case VENTURED_NET_CONNECTING: return "CONNECTING";
        case VENTURED_NET_CONNECTED: return "CONNECTED";
        case VENTURED_NET_FAILED: return "FAILED";
        default: return NULL;
    }
}

bool ventured_parse_net_state(const char *token, ventured_net_state_t *out) {
    if (token == NULL || out == NULL) return false;
    if (strcmp(token, "DISCONNECTED") == 0) {
        *out = VENTURED_NET_DISCONNECTED;
        return true;
    }
    if (strcmp(token, "CONNECTING") == 0) {
        *out = VENTURED_NET_CONNECTING;
        return true;
    }
    if (strcmp(token, "CONNECTED") == 0) {
        *out = VENTURED_NET_CONNECTED;
        return true;
    }
    if (strcmp(token, "FAILED") == 0) {
        *out = VENTURED_NET_FAILED;
        return true;
    }
    return false;
}

static bool append_escaped(char *buffer, size_t capacity, size_t *used, const char *text) {
    for (const char *cursor = text; *cursor != '\0'; ++cursor) {
        char ch = *cursor;
        if (ch == '"' || ch == '\\') {
            if (*used + 2 >= capacity) return false;
            buffer[(*used)++] = '\\';
        } else if ((unsigned char)ch < 32) {
            return false;
        }
        if (*used + 1 >= capacity) return false;
        buffer[(*used)++] = ch;
    }
    return true;
}

bool ventured_ssid_looks_5g(const char *ssid) {
    if (ssid == NULL) return false;
    for (const unsigned char *cursor = (const unsigned char *)ssid; *cursor != '\0'; ++cursor) {
        if (*cursor != '5') continue;
        const unsigned char *next = cursor + 1;
        while (*next == ' ') ++next;
        if (*next != 'g' && *next != 'G') continue;
        ++next;
        if ((next[0] == 'h' || next[0] == 'H') && (next[1] == 'z' || next[1] == 'Z')) {
            next += 2;
        }
        unsigned char tail = *next;
        if (tail == '\0' ||
            !((tail >= '0' && tail <= '9') || (tail >= 'A' && tail <= 'Z') || (tail >= 'a' && tail <= 'z'))) {
            return true;
        }
    }
    return false;
}

static bool reason_allowed(const char *reason) {
    return strcmp(reason, "AUTH") == 0 ||
           strcmp(reason, "TIMEOUT") == 0 ||
           strcmp(reason, "NO_AP") == 0 ||
           strcmp(reason, "BAND") == 0 ||
           strcmp(reason, "UNKNOWN") == 0;
}

bool ventured_format_net_event(char *buffer, size_t capacity, const ventured_net_status_t *status) {
    const char *state = status == NULL ? NULL : ventured_net_state_token(status->state);
    if (buffer == NULL || capacity == 0U || state == NULL) return false;
    int written = snprintf(buffer, capacity,
                           "VKEY_NET/1 {\"seq\":%" PRIu32 ",\"state\":\"%s\",\"ssid\":\"",
                           status->sequence, state);
    if (written < 0 || (size_t)written >= capacity) return false;
    size_t used = (size_t)written;
    if (!append_escaped(buffer, capacity, &used, status->ssid)) return false;
    const char *mid = "\",\"ip\":\"";
    if (used + strlen(mid) >= capacity) return false;
    memcpy(buffer + used, mid, strlen(mid));
    used += strlen(mid);
    if (!append_escaped(buffer, capacity, &used, status->ip)) return false;
    if (status->reason[0] != '\0' && reason_allowed(status->reason)) {
        written = snprintf(buffer + used, capacity - used, "\",\"rssi\":%d,\"reason\":\"%s\"}\n",
                           status->rssi, status->reason);
    } else {
        written = snprintf(buffer + used, capacity - used, "\",\"rssi\":%d}\n", status->rssi);
    }
    return written >= 0 && (size_t)written < capacity - used;
}
