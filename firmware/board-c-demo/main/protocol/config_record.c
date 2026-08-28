#include "config_record.h"

#include <ctype.h>
#include <string.h>

static const char *k_prefix = "VKEY_CONFIG/1 ";
bool ventured_model_allowed(const char *model) {
    if (model == NULL) return false;
    size_t n = strlen(model);
    if (n > VENTURED_MODEL_MAX) return false;
    for (size_t i = 0; i < n; ++i) {
        if ((unsigned char)model[i] < 32) return false;
    }
    return true;
}

static const char *skip_ws(const char *cursor) {
    while (*cursor == ' ' || *cursor == '\t') ++cursor;
    return cursor;
}

static bool parse_u32(const char *cursor, uint32_t *value, const char **end) {
    if (!isdigit((unsigned char)*cursor)) return false;
    unsigned long parsed = 0;
    while (isdigit((unsigned char)*cursor)) {
        parsed = parsed * 10UL + (unsigned long)(*cursor - '0');
        if (parsed > 0xFFFFFFFFUL) return false;
        ++cursor;
    }
    *value = (uint32_t)parsed;
    *end = cursor;
    return true;
}

static bool parse_string(const char *cursor, char *out, size_t out_len, const char **end) {
    if (*cursor != '"') return false;
    ++cursor;
    size_t written = 0;
    while (*cursor != '\0' && *cursor != '"') {
        char ch = *cursor++;
        if (ch == '\\') {
            if (*cursor == '\0') return false;
            ch = *cursor++;
            if (ch != '"' && ch != '\\' && ch != '/') return false;
        }
        if (ch < 32) return false;
        if (written + 1 >= out_len) return false;
        out[written++] = ch;
    }
    if (*cursor != '"') return false;
    out[written] = '\0';
    *end = cursor + 1;
    return true;
}

static bool copy_required(char *dest, size_t dest_len, const char *value, bool seen) {
    if (!seen || value[0] == '\0') return false;
    if (strlen(value) >= dest_len) return false;
    memcpy(dest, value, strlen(value) + 1);
    return true;
}

bool ventured_parse_config_line(const char *line, ventured_device_config_t *out) {
    if (line == NULL || out == NULL) return false;
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) --len;
    if (len > 1024) return false;
    char trimmed[1025];
    memcpy(trimmed, line, len);
    trimmed[len] = '\0';
    if (strncmp(trimmed, k_prefix, strlen(k_prefix)) != 0) return false;
    const char *cursor = skip_ws(trimmed + strlen(k_prefix));
    if (*cursor != '{') return false;
    ++cursor;

    bool seen_seq = false, seen_ssid = false, seen_password = false, seen_key = false,
         seen_model = false;
    ventured_device_config_t parsed;
    memset(&parsed, 0, sizeof(parsed));

    cursor = skip_ws(cursor);
    while (*cursor != '\0' && *cursor != '}') {
        char key[16];
        if (!parse_string(cursor, key, sizeof(key), &cursor)) return false;
        cursor = skip_ws(cursor);
        if (*cursor != ':') return false;
        cursor = skip_ws(cursor + 1);

        if (strcmp(key, "seq") == 0) {
            if (seen_seq || !parse_u32(cursor, &parsed.sequence, &cursor)) return false;
            seen_seq = true;
        } else if (strcmp(key, "ssid") == 0) {
            if (seen_ssid || !parse_string(cursor, parsed.ssid, sizeof(parsed.ssid), &cursor)) {
                return false;
            }
            seen_ssid = true;
        } else if (strcmp(key, "password") == 0) {
            if (seen_password ||
                !parse_string(cursor, parsed.password, sizeof(parsed.password), &cursor)) {
                return false;
            }
            seen_password = true;
        } else if (strcmp(key, "apiKey") == 0) {
            if (seen_key ||
                !parse_string(cursor, parsed.api_key, sizeof(parsed.api_key), &cursor)) {
                return false;
            }
            seen_key = true;
        } else if (strcmp(key, "model") == 0) {
            if (seen_model || !parse_string(cursor, parsed.model, sizeof(parsed.model), &cursor)) {
                return false;
            }
            seen_model = true;
        } else {
            return false;
        }

        cursor = skip_ws(cursor);
        if (*cursor == ',') {
            cursor = skip_ws(cursor + 1);
            continue;
        }
        if (*cursor != '}') return false;
    }
    if (*cursor != '}') return false;
    cursor = skip_ws(cursor + 1);
    if (*cursor != '\0') return false;

    if (!seen_seq || !copy_required(parsed.ssid, sizeof(parsed.ssid), parsed.ssid, seen_ssid) ||
        !seen_password || !seen_key || !seen_model ||
        !ventured_model_allowed(parsed.model)) {
        return false;
    }
    *out = parsed;
    return true;
}
