#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

enum { VENTURED_PCM_SILENCE_RMS = 80 };

typedef struct {
    uint32_t samples;
    uint64_t sum_squares;
    uint32_t peak;
} ventured_pcm_metrics_t;

void ventured_pcm_metrics_reset(ventured_pcm_metrics_t *metrics);
void ventured_pcm_metrics_add(ventured_pcm_metrics_t *metrics, const int16_t *samples, size_t count);
uint32_t ventured_pcm_metrics_rms(const ventured_pcm_metrics_t *metrics);
bool ventured_pcm_metrics_silence(const ventured_pcm_metrics_t *metrics, uint32_t threshold);
int ventured_pcm_i2s32_shift(const int32_t *samples, size_t count);
void ventured_pcm_i2s32_to_s16(const int32_t *input, int16_t *output, size_t count, int shift);
