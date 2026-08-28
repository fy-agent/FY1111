#include "pcm_metrics.h"

#include <limits.h>

static uint32_t isqrt_u64(uint64_t value) {
    uint64_t op = value;
    uint64_t result = 0;
    uint64_t bit = 1ULL << 62;
    while (bit > op) bit >>= 2;
    while (bit != 0) {
        if (op >= result + bit) {
            op -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }
    return (uint32_t)result;
}

static uint32_t abs_i32(int32_t value) {
    if (value == INT32_MIN) return (uint32_t)INT32_MAX + 1U;
    return value < 0 ? (uint32_t)(-value) : (uint32_t)value;
}

void ventured_pcm_metrics_reset(ventured_pcm_metrics_t *metrics) {
    if (metrics == NULL) return;
    *metrics = (ventured_pcm_metrics_t){0};
}

void ventured_pcm_metrics_add(ventured_pcm_metrics_t *metrics, const int16_t *samples, size_t count) {
    if (metrics == NULL || samples == NULL) return;
    for (size_t i = 0; i < count; ++i) {
        int32_t sample = samples[i];
        uint32_t magnitude = sample < 0 ? (uint32_t)(-sample) : (uint32_t)sample;
        if (magnitude > metrics->peak) metrics->peak = magnitude;
        metrics->sum_squares += (uint64_t)((int32_t)sample * (int32_t)sample);
        if (metrics->samples < UINT32_MAX) ++metrics->samples;
    }
}

uint32_t ventured_pcm_metrics_rms(const ventured_pcm_metrics_t *metrics) {
    if (metrics == NULL || metrics->samples == 0U) return 0;
    return isqrt_u64(metrics->sum_squares / metrics->samples);
}

bool ventured_pcm_metrics_silence(const ventured_pcm_metrics_t *metrics, uint32_t threshold) {
    return ventured_pcm_metrics_rms(metrics) < threshold;
}

int ventured_pcm_i2s32_shift(const int32_t *samples, size_t count) {
    uint32_t max_abs = 0;
    if (samples == NULL) return 16;
    for (size_t i = 0; i < count; ++i) {
        uint32_t magnitude = abs_i32(samples[i]);
        if (magnitude > max_abs) max_abs = magnitude;
    }
    if (max_abs == 0U) return 16;
    if ((max_abs >> 16) != 0U) return 16;
    if ((max_abs >> 8) != 0U) return 8;
    return 0;
}

void ventured_pcm_i2s32_to_s16(const int32_t *input, int16_t *output, size_t count, int shift) {
    if (input == NULL || output == NULL) return;
    if (shift < 0) shift = 0;
    if (shift > 31) shift = 31;
    for (size_t i = 0; i < count; ++i) {
        int32_t shifted = input[i] >> shift;
        if (shifted > INT16_MAX) shifted = INT16_MAX;
        if (shifted < INT16_MIN) shifted = INT16_MIN;
        output[i] = (int16_t)shifted;
    }
}
