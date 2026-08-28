#include "wav_pcm.h"

#include <string.h>

static void put_le16(uint8_t *out, uint16_t value) {
    out[0] = (uint8_t)(value & 0xFF);
    out[1] = (uint8_t)((value >> 8) & 0xFF);
}

static void put_le32(uint8_t *out, uint32_t value) {
    out[0] = (uint8_t)(value & 0xFF);
    out[1] = (uint8_t)((value >> 8) & 0xFF);
    out[2] = (uint8_t)((value >> 16) & 0xFF);
    out[3] = (uint8_t)((value >> 24) & 0xFF);
}

void ventured_wav_write_header(uint8_t *out, uint32_t pcm_bytes, uint32_t sample_rate_hz) {
    if (out == NULL) return;
    memcpy(out, "RIFF", 4);
    put_le32(out + 4, 36U + pcm_bytes);
    memcpy(out + 8, "WAVE", 4);
    memcpy(out + 12, "fmt ", 4);
    put_le32(out + 16, 16);
    put_le16(out + 20, 1);
    put_le16(out + 22, 1);
    put_le32(out + 24, sample_rate_hz);
    put_le32(out + 28, sample_rate_hz * 2U);
    put_le16(out + 32, 2);
    put_le16(out + 34, 16);
    memcpy(out + 36, "data", 4);
    put_le32(out + 40, pcm_bytes);
}

bool ventured_wav_header_is_pcm16_mono(const uint8_t *header, uint32_t pcm_bytes, uint32_t sample_rate_hz) {
    uint8_t expected[VENTURED_WAV_HEADER_BYTES];
    if (header == NULL) return false;
    ventured_wav_write_header(expected, pcm_bytes, sample_rate_hz);
    return memcmp(header, expected, VENTURED_WAV_HEADER_BYTES) == 0;
}
