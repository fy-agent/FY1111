#include "mic_rec.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_heap_caps.h"

#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_c_pins.h"
#include "display/st7789_status.h"
#include "ec11_core.h"
#include "net/asr_upload.h"
#include "net/wifi_sta.h"
#include "pcm_metrics.h"
#include "protocol/rec_event.h"
#include "rec_gate.h"

static const char *TAG = "board_c_mic";
#define SCAN_PERIOD_MS 10U
#define BUTTON_DEBOUNCE_MS 25U
#define SAMPLE_RATE_HZ 16000U
#define ACTIVE_EMIT_MS 250U
#define DMA_FRAMES 256U
#define KEEP_RESERVE_BYTES (256U * 1024U)

static i2s_chan_handle_t s_rx;
static ventured_button_t s_button;
static uint32_t s_sequence;
static bool s_i2s_ok;
static int32_t s_dma[DMA_FRAMES];
static int16_t s_pcm[DMA_FRAMES];
static int16_t *s_keep;
static size_t s_keep_cap;
static size_t s_keep_count;
static bool s_keep_held;
static volatile bool s_hand_held;

static void *alloc_audio(size_t bytes) {
    void *block = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (block == NULL) block = malloc(bytes);
    return block;
}

static void alloc_keep(void) {
    size_t spiram_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    if (spiram_block > KEEP_RESERVE_BYTES + SAMPLE_RATE_HZ * sizeof(int16_t)) {
        size_t bytes = spiram_block - KEEP_RESERVE_BYTES;
        bytes -= bytes % sizeof(int16_t);
        s_keep = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (s_keep != NULL) {
            s_keep_cap = bytes / sizeof(int16_t);
            ESP_LOGI(TAG, "pcm keep spiram samples=%u ms=%u", (unsigned)s_keep_cap,
                     (unsigned)((s_keep_cap * 1000U) / SAMPLE_RATE_HZ));
            return;
        }
    }
    static const uint32_t try_ms[] = { 8000U, 5000U, 3000U };
    for (size_t i = 0; i < sizeof(try_ms) / sizeof(try_ms[0]); ++i) {
        size_t samples = (SAMPLE_RATE_HZ * try_ms[i]) / 1000U;
        s_keep = alloc_audio(samples * sizeof(int16_t));
        if (s_keep != NULL) {
            s_keep_cap = samples;
            ESP_LOGI(TAG, "pcm keep fallback samples=%u ms=%u", (unsigned)samples, (unsigned)try_ms[i]);
            return;
        }
    }
    ESP_LOGW(TAG, "pcm keep buffer unavailable; asr upload disabled");
}

static bool keep_full(void) {
    return s_keep != NULL && s_keep_cap > 0 && s_keep_count >= s_keep_cap;
}

static void clear_keep(void) {
    if (s_keep != NULL && s_keep_count > 0) {
        memset(s_keep, 0, s_keep_count * sizeof(int16_t));
        ESP_LOGI(TAG, "pcm keep cleared samples=%u", (unsigned)s_keep_count);
    }
    s_keep_count = 0;
    s_keep_held = false;
}

static void emit_rec(const ventured_rec_status_t *status) {
    char record[192];
    if (!ventured_format_rec_event(record, sizeof(record), status)) return;
    fputs(record, stdout);
    fflush(stdout);
}

static ventured_rec_status_t rec_from_metrics(ventured_rec_state_t state, const ventured_pcm_metrics_t *metrics,
                                              uint32_t ms) {
    ventured_rec_status_t status = {
        .sequence = ++s_sequence,
        .state = state,
        .ms = ms,
        .samples = metrics->samples,
        .rms = ventured_pcm_metrics_rms(metrics),
        .peak = metrics->peak,
        .silence = ventured_pcm_metrics_silence(metrics, VENTURED_PCM_SILENCE_RMS),
    };
    return status;
}

static void fail_rec(const char *reason) {
    ventured_rec_status_t status = {
        .sequence = ++s_sequence,
        .state = VENTURED_REC_FAIL,
    };
    if (reason != NULL) strncpy(status.reason, reason, sizeof(status.reason) - 1U);
    emit_rec(&status);
    ventured_display_show_rec(&status);
}

static void fail_i2s(void) {
    fail_rec("I2S");
}

static bool begin_record(void) {
    if (!s_i2s_ok || s_rx == NULL) {
        fail_i2s();
        return false;
    }
    if (i2s_channel_enable(s_rx) != ESP_OK) {
        fail_i2s();
        return false;
    }
    return true;
}

static void end_record(void) {
    if (s_rx != NULL) {
        i2s_channel_disable(s_rx);
    }
}

static void ingest_i2s(ventured_pcm_metrics_t *metrics) {
    size_t bytes = 0;
    if (i2s_channel_read(s_rx, s_dma, sizeof(s_dma), &bytes, pdMS_TO_TICKS(30)) != ESP_OK) return;
    size_t count = bytes / sizeof(int32_t);
    if (count == 0) return;
    if (count > DMA_FRAMES) count = DMA_FRAMES;
    int shift = ventured_pcm_i2s32_shift(s_dma, count);
    ventured_pcm_i2s32_to_s16(s_dma, s_pcm, count, shift);
    ventured_pcm_metrics_add(metrics, s_pcm, count);
    if (s_keep != NULL && s_keep_count < s_keep_cap) {
        size_t room = s_keep_cap - s_keep_count;
        size_t copy = count < room ? count : room;
        memcpy(s_keep + s_keep_count, s_pcm, copy * sizeof(int16_t));
        s_keep_count += copy;
    }
}

static void rec_task(void *arg) {
    (void)arg;
    TickType_t next_scan = xTaskGetTickCount();
    bool recording = false;
    TickType_t rec_start = 0;
    TickType_t next_active = 0;
    ventured_rec_gate_t gate;
    ventured_rec_gate_init(&gate);
    ventured_pcm_metrics_t metrics;
    ventured_pcm_metrics_reset(&metrics);

    for (;;) {
        TickType_t now = xTaskGetTickCount();
        if ((int32_t)(now - next_scan) >= 0) {
            bool gpio_down = gpio_get_level(BOARD_C_MIC_BUTTON_GPIO) == 0;
            ventured_button_update(&s_button, gpio_down);
            bool want = s_button.stable_pressed || s_hand_held;
            uint32_t now_ms = (uint32_t)now * portTICK_PERIOD_MS;
            bool net_ok = ventured_wifi_link_ready();
            if (s_keep_held && !ventured_asr_busy()) {
                clear_keep();
            }
            if (recording && !net_ok) {
                end_record();
                recording = false;
                ventured_rec_gate_abort(&gate, now_ms);
                fail_rec("WIFI");
                ESP_LOGW(TAG, "record aborted; wifi down");
                clear_keep();
            }
            ventured_rec_gate_action_t action =
                ventured_rec_gate_update(&gate, want, ventured_asr_busy(), recording && keep_full(), now_ms);
            if (action == VENTURED_REC_GATE_START) {
                if (!net_ok) {
                    ventured_rec_gate_abort(&gate, now_ms);
                    fail_rec("WIFI");
                    ESP_LOGW(TAG, "record blocked; wifi down");
                } else if (begin_record()) {
                    recording = true;
                    rec_start = now;
                    next_active = now + pdMS_TO_TICKS(ACTIVE_EMIT_MS);
                    ventured_pcm_metrics_reset(&metrics);
                    if (!s_keep_held) clear_keep();
                    s_keep_count = 0;
                    ventured_rec_status_t start = rec_from_metrics(VENTURED_REC_START, &metrics, 0);
                    emit_rec(&start);
                    ventured_display_show_rec(&start);
                    ESP_LOGI(TAG, "record start");
                } else {
                    ventured_rec_gate_abort(&gate, now_ms);
                }
            } else if (action == VENTURED_REC_GATE_STOP) {
                end_record();
                recording = false;
                uint32_t ms = (uint32_t)(now - rec_start);
                ventured_rec_status_t done = rec_from_metrics(VENTURED_REC_DONE, &metrics, ms);
                emit_rec(&done);
                ventured_display_show_rec(&done);
                ESP_LOGI(TAG, "record done ms=%" PRIu32 " rms=%" PRIu32 " peak=%" PRIu32 " silence=%d",
                         done.ms, done.rms, done.peak, (int)done.silence);
                if (!done.silence && s_keep != NULL && s_keep_count > 0 && ventured_wifi_cloud_ready()) {
                    s_keep_held = true;
                    ventured_asr_submit(s_keep, s_keep_count);
                    if (!ventured_asr_busy()) {
                        clear_keep();
                    }
                } else {
                    clear_keep();
                }
            } else if (action == VENTURED_REC_GATE_CANCEL_ASR) {
                ventured_asr_cancel();
                ESP_LOGI(TAG, "asr cancel requested");
            }
            next_scan += pdMS_TO_TICKS(SCAN_PERIOD_MS);
            if ((int32_t)(now - next_scan) >= 0) {
                next_scan = now + pdMS_TO_TICKS(SCAN_PERIOD_MS);
            }
        }

        if (recording) {
            ingest_i2s(&metrics);
            now = xTaskGetTickCount();
            if ((int32_t)(now - next_active) >= 0) {
                uint32_t ms = (uint32_t)(now - rec_start);
                ventured_rec_status_t active = rec_from_metrics(VENTURED_REC_ACTIVE, &metrics, ms);
                emit_rec(&active);
                ventured_display_show_rec(&active);
                next_active = now + pdMS_TO_TICKS(ACTIVE_EMIT_MS);
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(SCAN_PERIOD_MS));
        }
    }
}

static esp_err_t configure_button(void) {
    gpio_config_t button_io = {
        .pin_bit_mask = (1ULL << BOARD_C_MIC_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    return gpio_config(&button_io);
}

static esp_err_t configure_i2s(void) {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 4;
    chan_cfg.dma_frame_num = DMA_FRAMES;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, NULL, &s_rx), TAG, "i2s channel");
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE_HZ),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = BOARD_C_MIC_SCK_GPIO,
            .ws = BOARD_C_MIC_WS_GPIO,
            .dout = I2S_GPIO_UNUSED,
            .din = BOARD_C_MIC_SD_GPIO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
#ifdef CONFIG_VENTURED_MIC_SLOT_RIGHT
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT;
#else
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
#endif
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_rx, &std_cfg), TAG, "i2s std");
    return ESP_OK;
}

void ventured_mic_rec_set_hand_held(bool held) {
    s_hand_held = held;
}

esp_err_t ventured_mic_rec_start(void) {
    ESP_RETURN_ON_ERROR(configure_button(), TAG, "mic button GPIO");
    ventured_button_init(&s_button, gpio_get_level(BOARD_C_MIC_BUTTON_GPIO) == 0,
                         ventured_button_required_samples(SCAN_PERIOD_MS, BUTTON_DEBOUNCE_MS));
    alloc_keep();
    s_i2s_ok = configure_i2s() == ESP_OK;
    if (!s_i2s_ok) {
        ESP_LOGW(TAG, "i2s unavailable; GPIO9 still reports FAIL");
    }
    if (xTaskCreate(rec_task, "mic_rec", 4096, NULL, 5, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
