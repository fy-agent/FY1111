#include "link_debug.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static bool append_escaped(char *buffer, size_t capacity, size_t *used, const char *text) {
    if (text == NULL) return false;
    for (const char *cursor = text; *cursor != '\0'; ++cursor) {
        char ch = *cursor;
        if (ch == '"' || ch == '\\') {
            if (*used + 2 >= capacity) return false;
            buffer[(*used)++] = '\\';
        } else if ((unsigned char)ch < 32) {
            ch = ' ';
        }
        if (*used + 1 >= capacity) return false;
        buffer[(*used)++] = ch;
    }
    return true;
}

bool ventured_format_log_event(char *buffer, size_t capacity, uint32_t sequence, const char *message) {
    if (buffer == NULL || capacity == 0U || message == NULL || message[0] == '\0') return false;
    int written = snprintf(buffer, capacity, "VKEY_LOG/1 {\"seq\":%" PRIu32 ",\"msg\":\"", sequence);
    if (written < 0 || (size_t)written >= capacity) return false;
    size_t used = (size_t)written;
    if (!append_escaped(buffer, capacity, &used, message)) return false;
    if (used + 3 >= capacity) return false;
    buffer[used++] = '"';
    buffer[used++] = '}';
    buffer[used++] = '\n';
    buffer[used] = '\0';
    return true;
}

bool ventured_format_ping_event(char *buffer, size_t capacity, uint32_t sequence, const char *host,
                                bool ok, int ms, unsigned lost, unsigned sent) {
    if (buffer == NULL || capacity == 0U || host == NULL || host[0] == '\0') return false;
    int written = snprintf(buffer, capacity, "VKEY_PING/1 {\"seq\":%" PRIu32 ",\"host\":\"", sequence);
    if (written < 0 || (size_t)written >= capacity) return false;
    size_t used = (size_t)written;
    if (!append_escaped(buffer, capacity, &used, host)) return false;
    written = snprintf(buffer + used, capacity - used,
                       "\",\"ok\":%s,\"ms\":%d,\"lost\":%u,\"sent\":%u}\n",
                       ok ? "true" : "false", ms, lost, sent);
    return written >= 0 && (size_t)written < capacity - used;
}
