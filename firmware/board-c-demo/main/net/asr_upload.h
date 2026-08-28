#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

esp_err_t ventured_asr_start(void);
bool ventured_asr_busy(void);
void ventured_asr_submit(const int16_t *pcm, size_t samples);
void ventured_asr_cancel(void);
