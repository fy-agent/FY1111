#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool ventured_format_log_event(char *buffer, size_t capacity, uint32_t sequence, const char *message);
bool ventured_format_ping_event(char *buffer, size_t capacity, uint32_t sequence, const char *host,
                                bool ok, int ms, unsigned lost, unsigned sent);
