#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    VENTURED_NET_DISCONNECTED = 0,
    VENTURED_NET_CONNECTING,
    VENTURED_NET_CONNECTED,
    VENTURED_NET_FAILED,
} ventured_net_state_t;

typedef struct {
    uint32_t sequence;
    ventured_net_state_t state;
    char ssid[33];
    char ip[16];
    int rssi;
    char reason[12];
} ventured_net_status_t;

const char *ventured_net_state_token(ventured_net_state_t state);
bool ventured_parse_net_state(const char *token, ventured_net_state_t *out);
bool ventured_ssid_looks_5g(const char *ssid);
bool ventured_format_net_event(char *buffer, size_t capacity, const ventured_net_status_t *status);
