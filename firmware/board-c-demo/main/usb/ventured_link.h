#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

esp_err_t ventured_link_start(void);
void ventured_link_write(const char *line);
bool ventured_link_readline(char *line, size_t capacity, TickType_t wait);
bool ventured_link_mounted(void);
