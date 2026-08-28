#pragma once

#include <stdbool.h>
#include <stdint.h>

enum { VENTURED_FONT_ROWS = 32, VENTURED_FONT_COLS = 32 };

bool ventured_font_bits(uint32_t codepoint, const uint32_t **rows);
