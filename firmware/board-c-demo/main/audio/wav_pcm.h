#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { VENTURED_WAV_HEADER_BYTES = 44 };

void ventured_wav_write_header(uint8_t *out, uint32_t pcm_bytes, uint32_t sample_rate_hz);
bool ventured_wav_header_is_pcm16_mono(const uint8_t *header, uint32_t pcm_bytes, uint32_t sample_rate_hz);
