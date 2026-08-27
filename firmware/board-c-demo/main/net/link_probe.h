#pragma once

#include "esp_err.h"

esp_err_t ventured_link_probe_start(void);
void ventured_link_logf(const char *fmt, ...);
void ventured_link_probe_now(void);
void ventured_link_probe_kick(void);
