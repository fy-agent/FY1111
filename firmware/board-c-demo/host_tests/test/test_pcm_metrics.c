#include "unity.h"

#include "pcm_metrics.h"

void test_rms_peak_and_silence_of_known_pcm(void) {
    ventured_pcm_metrics_t metrics;
    const int16_t quiet[] = {0, 0, 0, 0};
    ventured_pcm_metrics_reset(&metrics);
    ventured_pcm_metrics_add(&metrics, quiet, 4);
    TEST_ASSERT_EQUAL_UINT32(4, metrics.samples);
    TEST_ASSERT_EQUAL_UINT32(0, ventured_pcm_metrics_rms(&metrics));
    TEST_ASSERT_EQUAL_UINT32(0, metrics.peak);
    TEST_ASSERT_TRUE(ventured_pcm_metrics_silence(&metrics, VENTURED_PCM_SILENCE_RMS));

    const int16_t voice[] = {1000, -1000, 1000, -1000};
    ventured_pcm_metrics_reset(&metrics);
    ventured_pcm_metrics_add(&metrics, voice, 4);
    TEST_ASSERT_EQUAL_UINT32(1000, ventured_pcm_metrics_rms(&metrics));
    TEST_ASSERT_EQUAL_UINT32(1000, metrics.peak);
    TEST_ASSERT_FALSE(ventured_pcm_metrics_silence(&metrics, VENTURED_PCM_SILENCE_RMS));
}

void test_i2s32_shift_picks_occupied_bits(void) {
    const int32_t high[] = {0x00100000, (int32_t)0xFF000000};
    TEST_ASSERT_EQUAL_INT(16, ventured_pcm_i2s32_shift(high, 2));
    const int32_t mid[] = {0x00001200, (int32_t)0xFFFFF000};
    TEST_ASSERT_EQUAL_INT(8, ventured_pcm_i2s32_shift(mid, 2));
    const int32_t low[] = {0x00000040, -0x00000020};
    TEST_ASSERT_EQUAL_INT(0, ventured_pcm_i2s32_shift(low, 2));
    const int32_t none[] = {0, 0};
    TEST_ASSERT_EQUAL_INT(16, ventured_pcm_i2s32_shift(none, 2));
}

void test_i2s32_to_s16_uses_selected_shift(void) {
    const int32_t input[] = {0x00100000, (int32_t)0xFF000000};
    int16_t output[2] = {0, 0};
    ventured_pcm_i2s32_to_s16(input, output, 2, 16);
    TEST_ASSERT_EQUAL_INT16(0x0010, output[0]);
    TEST_ASSERT_EQUAL_INT16((int16_t)0xFF00, output[1]);
}
