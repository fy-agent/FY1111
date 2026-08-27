#pragma once

#include <stdbool.h>
#include <stdint.h>

enum { VENTURED_FONT16_ROWS = 16, VENTURED_FONT16_COLS = 16 };

bool ventured_font16_bits(uint32_t codepoint, const uint16_t **rows);
