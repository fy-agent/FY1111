#include "asr_upload.h"

#include <stdio.h>
#include <string.h>

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "audio/wav_pcm.h"
#include "display/st7789_status.h"
#include "protocol/asr_event.h"
#include "protocol/config_record.h"
#include "usb/ventured_link.h"
#include "wifi_sta.h"

static const char *TAG = "board_c_asr";
#define SAMPLE_RATE_HZ 16000U
#define ASR_URL "https://api.siliconflow.cn/v1/audio/transcriptions"
#define BOUNDARY "----VenturedAsr8"
#define AUTH_PREFIX "Bearer "

typedef struct {
    const int16_t *pcm;
    size_t samples;
} asr_job_t;

static QueueHandle_t s_jobs;
static SemaphoreHandle_t s_http_lock;
static esp_http_client_handle_t s_client;
static uint32_t s_sequence;
static volatile bool s_busy;
static volatile bool s_cancel;

static void emit_asr(const ventured_asr_status_t *status) {
    char record[1024];
    if (!ventured_format_asr_event(record, sizeof(record), status)) return;
    ventured_link_write(record);
}

static void publish(ventured_asr_state_t state, const char *text, const char *reason) {
    ventured_asr_status_t status = {0};
    status.sequence = ++s_sequence;
    status.state = state;
    if (text != NULL) {
        strncpy(status.text, text, sizeof(status.text) - 1);
    }
    if (reason != NULL) {
        strncpy(status.reason, reason, sizeof(status.reason) - 1);
    }
    emit_asr(&status);
    ventured_display_show_asr(&status);
}

static const char *fail_reason_for_status(int status) {
    if (status == 401 || status == 403) return "AUTH";
    if (status == 400) return "FORMAT";
    return "HTTP";
}

static void attach_client(esp_http_client_handle_t client) {
    if (s_http_lock == NULL) return;
    xSemaphoreTake(s_http_lock, portMAX_DELAY);
    s_client = client;
    xSemaphoreGive(s_http_lock);
}

static void detach_client(void) {
    if (s_http_lock == NULL) return;
    xSemaphoreTake(s_http_lock, portMAX_DELAY);
    s_client = NULL;
    xSemaphoreGive(s_http_lock);
}

static bool write_all(esp_http_client_handle_t client, const void *data, size_t len) {
    const char *cursor = data;
    size_t left = len;
    while (left > 0) {
        if (s_cancel) return false;
        int wrote = esp_http_client_write(client, cursor, (int)left);
        if (wrote <= 0) return false;
        cursor += wrote;
        left -= (size_t)wrote;
    }
    return true;
}

static void upload_pcm(const int16_t *pcm, size_t samples, const char *api_key, const char *model) {
    uint32_t pcm_bytes = (uint32_t)(samples * sizeof(int16_t));
    char prefix[256];
    int prefix_len = snprintf(
        prefix, sizeof(prefix),
        "--" BOUNDARY "\r\n"
        "Content-Disposition: form-data; name=\"model\"\r\n\r\n"
        "%s\r\n"
        "--" BOUNDARY "\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"rec.wav\"\r\n"
        "Content-Type: audio/wav\r\n\r\n",
        model);
    static const char suffix[] = "\r\n--" BOUNDARY "--\r\n";
    if (prefix_len <= 0) {
        publish(VENTURED_ASR_FAIL, NULL, "MEM");
        return;
    }
    uint8_t header[VENTURED_WAV_HEADER_BYTES];
    ventured_wav_write_header(header, pcm_bytes, SAMPLE_RATE_HZ);
    int content_len = prefix_len + VENTURED_WAV_HEADER_BYTES + (int)pcm_bytes + (int)(sizeof(suffix) - 1U);

    char auth[sizeof(AUTH_PREFIX) + VENTURED_API_KEY_MAX];
    snprintf(auth, sizeof(auth), AUTH_PREFIX "%s", api_key);
    char response[768];
    memset(response, 0, sizeof(response));
    esp_http_client_config_t config = {
        .url = ASR_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 60000,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .buffer_size = 4096,
        .buffer_size_tx = 8192,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        memset(auth, 0, sizeof(auth));
        publish(VENTURED_ASR_FAIL, NULL, s_cancel ? "CANCEL" : "HTTP");
        return;
    }
    attach_client(client);
    if (s_cancel) {
        detach_client();
        esp_http_client_cleanup(client);
        memset(auth, 0, sizeof(auth));
        publish(VENTURED_ASR_FAIL, NULL, "CANCEL");
        return;
    }
    esp_http_client_set_header(client, "Authorization", auth);
    esp_http_client_set_header(client, "Content-Type", "multipart/form-data; boundary=" BOUNDARY);
    esp_err_t err = esp_http_client_open(client, content_len);
    bool sent = !s_cancel && err == ESP_OK &&
                write_all(client, prefix, (size_t)prefix_len) &&
                write_all(client, header, VENTURED_WAV_HEADER_BYTES) &&
                write_all(client, pcm, pcm_bytes) &&
                write_all(client, suffix, sizeof(suffix) - 1U);
    int status = 0;
    int read_len = 0;
    if (sent && !s_cancel) {
        if (esp_http_client_fetch_headers(client) < 0) sent = false;
        status = esp_http_client_get_status_code(client);
        read_len = esp_http_client_read_response(client, response, sizeof(response) - 1);
    }
    if (read_len < 0) read_len = 0;
    response[read_len] = '\0';
    detach_client();
    esp_http_client_cleanup(client);
    memset(auth, 0, sizeof(auth));
    if (s_cancel) {
        memset(response, 0, sizeof(response));
        publish(VENTURED_ASR_FAIL, NULL, "CANCEL");
        return;
    }
    char text[VENTURED_ASR_TEXT_MAX + 1];
    memset(text, 0, sizeof(text));
    if (!sent || status < 200 || status >= 300 || !ventured_asr_extract_text(response, text, sizeof(text))) {
        ESP_LOGW(TAG, "asr http status=%d sent=%d", status, (int)sent);
        publish(VENTURED_ASR_FAIL, NULL, fail_reason_for_status(sent ? status : -1));
        memset(response, 0, sizeof(response));
        return;
    }
    memset(response, 0, sizeof(response));
    publish(VENTURED_ASR_DONE, text, NULL);
    ESP_LOGI(TAG, "asr done chars=%u", (unsigned)strlen(text));
}

static void asr_task(void *arg) {
    (void)arg;
    asr_job_t job;
    for (;;) {
        if (xQueueReceive(s_jobs, &job, portMAX_DELAY) != pdTRUE) continue;
        if (s_cancel) {
            publish(VENTURED_ASR_FAIL, NULL, "CANCEL");
        } else {
            publish(VENTURED_ASR_START, NULL, NULL);
            char api_key[VENTURED_API_KEY_MAX + 1];
            char model[VENTURED_MODEL_MAX + 1];
            memset(api_key, 0, sizeof(api_key));
            memset(model, 0, sizeof(model));
            if (s_cancel) {
                publish(VENTURED_ASR_FAIL, NULL, "CANCEL");
            } else if (!ventured_wifi_copy_cloud(api_key, sizeof(api_key), model, sizeof(model))) {
                publish(VENTURED_ASR_FAIL, NULL, api_key[0] == '\0' || model[0] == '\0' ? "KEY" : "WIFI");
            } else {
                upload_pcm(job.pcm, job.samples, api_key, model);
            }
            memset(api_key, 0, sizeof(api_key));
        }
        if (job.pcm != NULL && job.samples > 0) {
            memset((void *)job.pcm, 0, job.samples * sizeof(int16_t));
        }
        s_busy = false;
        s_cancel = false;
    }
}

bool ventured_asr_busy(void) {
    return s_busy;
}

esp_err_t ventured_asr_start(void) {
    if (s_jobs != NULL) return ESP_OK;
    s_http_lock = xSemaphoreCreateMutex();
    if (s_http_lock == NULL) return ESP_ERR_NO_MEM;
    s_jobs = xQueueCreate(1, sizeof(asr_job_t));
    if (s_jobs == NULL) return ESP_ERR_NO_MEM;
    if (xTaskCreate(asr_task, "asr_up", 12288, NULL, 5, NULL) != pdPASS) {
        vQueueDelete(s_jobs);
        s_jobs = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void ventured_asr_cancel(void) {
    if (!s_busy) return;
    s_cancel = true;
    if (s_http_lock == NULL) return;
    if (xSemaphoreTake(s_http_lock, pdMS_TO_TICKS(20)) != pdTRUE) return;
    if (s_client != NULL) {
        (void)esp_http_client_cancel_request(s_client);
    }
    xSemaphoreGive(s_http_lock);
}

void ventured_asr_submit(const int16_t *pcm, size_t samples) {
    if (s_jobs == NULL || pcm == NULL || samples == 0) return;
    if (s_busy) return;
    asr_job_t job = {.pcm = pcm, .samples = samples};
    s_cancel = false;
    s_busy = true;
    if (xQueueSend(s_jobs, &job, 0) != pdTRUE) {
        s_busy = false;
    }
}
