#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    VENTURED_SSID_MAX = 32,
    VENTURED_PASSWORD_MAX = 64,
    VENTURED_API_KEY_MAX = 256,
    VENTURED_MODEL_MAX = 64,
};

typedef struct {
    uint32_t sequence;
    char ssid[VENTURED_SSID_MAX + 1];
    char password[VENTURED_PASSWORD_MAX + 1];
    char api_key[VENTURED_API_KEY_MAX + 1];
    char model[VENTURED_MODEL_MAX + 1];
} ventured_device_config_t;

bool ventured_model_allowed(const char *model);
bool ventured_parse_config_line(const char *line, ventured_device_config_t *out);
