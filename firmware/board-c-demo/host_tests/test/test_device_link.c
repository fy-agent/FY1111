#include "unity.h"

#include <string.h>

#include "asr_event.h"
#include "config_record.h"
#include "font.h"
#include "link_debug.h"
#include "net_event.h"
#include "rec_event.h"
#include "sensor_event.h"
#include "status_text.h"
#include "wav_pcm.h"

void test_config_parser_accepts_open_network_and_allowed_model(void) {
    ventured_device_config_t config;
    TEST_ASSERT_TRUE(ventured_parse_config_line(
        "VKEY_CONFIG/1 {\"seq\":3,\"ssid\":\"cafe\",\"password\":\"\",\"apiKey\":\"sk-demo\",\"model\":\"XingChenAGI/XingChenASR-V3.2-Ultra\"}\n",
        &config));
    TEST_ASSERT_EQUAL_UINT32(3, config.sequence);
    TEST_ASSERT_EQUAL_STRING("cafe", config.ssid);
    TEST_ASSERT_EQUAL_STRING("", config.password);
    TEST_ASSERT_EQUAL_STRING("sk-demo", config.api_key);
    TEST_ASSERT_TRUE(ventured_model_allowed(config.model));
}

void test_config_parser_accepts_wifi_only_empty_cloud_fields(void) {
    ventured_device_config_t config;
    TEST_ASSERT_TRUE(ventured_parse_config_line(
        "VKEY_CONFIG/1 {\"seq\":4,\"ssid\":\"cafe\",\"password\":\"secret\",\"apiKey\":\"\",\"model\":\"\"}\n",
        &config));
    TEST_ASSERT_EQUAL_UINT32(4, config.sequence);
    TEST_ASSERT_EQUAL_STRING("cafe", config.ssid);
    TEST_ASSERT_EQUAL_STRING("secret", config.password);
    TEST_ASSERT_EQUAL_STRING("", config.api_key);
    TEST_ASSERT_EQUAL_STRING("", config.model);
    TEST_ASSERT_TRUE(ventured_model_allowed(config.model));
}

void test_config_parser_accepts_xingchen_and_custom_models(void) {
    ventured_device_config_t config;
    TEST_ASSERT_TRUE(ventured_parse_config_line(
        "VKEY_CONFIG/1 {\"seq\":5,\"ssid\":\"cafe\",\"password\":\"p\",\"apiKey\":\"k\",\"model\":\"XingChenAGI/XingChenASR-V3.2-Ultra\"}",
        &config));
    TEST_ASSERT_TRUE(ventured_parse_config_line(
        "VKEY_CONFIG/1 {\"seq\":6,\"ssid\":\"cafe\",\"password\":\"p\",\"apiKey\":\"k\",\"model\":\"custom/AddedModel\"}",
        &config));
}

void test_config_parser_rejects_unknown_fields_and_models(void) {
    ventured_device_config_t config;
    TEST_ASSERT_FALSE(ventured_parse_config_line(
        "VKEY_CONFIG/1 {\"seq\":1,\"ssid\":\"a\",\"password\":\"b\",\"apiKey\":\"c\",\"model\":\"XingChenAGI/XingChenASR-V3.2-Ultra\",\"extra\":true}",
        &config));
    TEST_ASSERT_FALSE(ventured_parse_config_line("VKEY_INPUT/1 {\"seq\":1,\"input\":\"ENCODER_CW\"}", &config));
}

void test_net_event_formatter_is_exact_and_omits_secrets(void) {
    char record[160];
    ventured_net_status_t status = {
        .sequence = 9,
        .state = VENTURED_NET_CONNECTED,
        .ssid = "cafe",
        .ip = "10.0.0.8",
        .rssi = -41,
    };
    TEST_ASSERT_TRUE(ventured_format_net_event(record, sizeof(record), &status));
    TEST_ASSERT_EQUAL_STRING(
        "VKEY_NET/1 {\"seq\":9,\"state\":\"CONNECTED\",\"ssid\":\"cafe\",\"ip\":\"10.0.0.8\",\"rssi\":-41}\n",
        record);
    TEST_ASSERT_EQUAL_STRING("已连接", ventured_net_state_zh(VENTURED_NET_CONNECTED));
    TEST_ASSERT_EQUAL_STRING("未连接", ventured_net_state_zh(VENTURED_NET_DISCONNECTED));
    TEST_ASSERT_EQUAL_STRING("失败", ventured_net_state_zh(VENTURED_NET_FAILED));
    TEST_ASSERT_EQUAL_STRING("连接失败", ventured_net_hero_zh(VENTURED_NET_FAILED));
    TEST_ASSERT_EQUAL_STRING("未连接", ventured_net_hero_zh(VENTURED_NET_DISCONNECTED));
    TEST_ASSERT_TRUE(ventured_lcd_net_should_redraw(VENTURED_NET_CONNECTED, "", VENTURED_NET_CONNECTING, ""));
    TEST_ASSERT_TRUE(ventured_lcd_net_should_redraw(VENTURED_NET_CONNECTED, "", VENTURED_NET_FAILED, "TIMEOUT"));
    TEST_ASSERT_TRUE(ventured_lcd_net_should_redraw(VENTURED_NET_DISCONNECTED, "", VENTURED_NET_FAILED, "NO_AP"));
    TEST_ASSERT_TRUE(ventured_lcd_net_should_redraw(VENTURED_NET_FAILED, "NO_AP", VENTURED_NET_FAILED, "AUTH"));
    TEST_ASSERT_FALSE(ventured_lcd_net_should_redraw(VENTURED_NET_FAILED, "AUTH", VENTURED_NET_FAILED, "AUTH"));
    TEST_ASSERT_FALSE(ventured_lcd_net_should_redraw(VENTURED_NET_CONNECTING, "", VENTURED_NET_CONNECTING, ""));
    TEST_ASSERT_NULL(strstr(record, "apiKey"));
    TEST_ASSERT_NULL(strstr(record, "password"));
    TEST_ASSERT_NULL(strstr(record, "reason"));
}

void test_ssid_5g_heuristic_and_failed_reason_field(void) {
    TEST_ASSERT_TRUE(ventured_ssid_looks_5g("Home-5G"));
    TEST_ASSERT_TRUE(ventured_ssid_looks_5g("office_5ghz"));
    TEST_ASSERT_TRUE(ventured_ssid_looks_5g("5G-office"));
    TEST_ASSERT_FALSE(ventured_ssid_looks_5g("5guys"));
    TEST_ASSERT_FALSE(ventured_ssid_looks_5g("cafe"));
    char record[180];
    ventured_net_status_t status = {
        .sequence = 4,
        .state = VENTURED_NET_FAILED,
        .ssid = "Home-5G",
        .ip = "",
        .rssi = 0,
        .reason = "BAND",
    };
    TEST_ASSERT_TRUE(ventured_format_net_event(record, sizeof(record), &status));
    TEST_ASSERT_EQUAL_STRING(
        "VKEY_NET/1 {\"seq\":4,\"state\":\"FAILED\",\"ssid\":\"Home-5G\",\"ip\":\"\",\"rssi\":0,\"reason\":\"BAND\"}\n",
        record);
}

void test_link_debug_records_are_exact_and_secret_free(void) {
    char record[160];
    TEST_ASSERT_TRUE(ventured_format_log_event(record, sizeof(record), 2, "sta got ip=10.0.0.8"));
    TEST_ASSERT_EQUAL_STRING("VKEY_LOG/1 {\"seq\":2,\"msg\":\"sta got ip=10.0.0.8\"}\n", record);
    TEST_ASSERT_TRUE(ventured_format_ping_event(record, sizeof(record), 4, "8.8.8.8", true, 18, 0, 3));
    TEST_ASSERT_EQUAL_STRING(
        "VKEY_PING/1 {\"seq\":4,\"host\":\"8.8.8.8\",\"ok\":true,\"ms\":18,\"lost\":0,\"sent\":3}\n",
        record);
    TEST_ASSERT_NULL(strstr(record, "password"));
    TEST_ASSERT_NULL(strstr(record, "apiKey"));
}

void test_rec_event_formatter_is_exact_and_not_an_input(void) {
    char record[192];
    ventured_rec_status_t start = {.sequence = 1, .state = VENTURED_REC_START};
    TEST_ASSERT_TRUE(ventured_format_rec_event(record, sizeof(record), &start));
    TEST_ASSERT_EQUAL_STRING(
        "VKEY_REC/1 {\"seq\":1,\"state\":\"START\",\"ms\":0,\"samples\":0,\"rms\":0,\"peak\":0}\n",
        record);
    TEST_ASSERT_EQUAL_STRING("录音中", ventured_rec_state_zh(VENTURED_REC_START));
    ventured_rec_status_t done = {
        .sequence = 3,
        .state = VENTURED_REC_DONE,
        .ms = 1200,
        .samples = 19200,
        .rms = 800,
        .peak = 9000,
        .silence = false,
    };
    TEST_ASSERT_TRUE(ventured_format_rec_event(record, sizeof(record), &done));
    TEST_ASSERT_EQUAL_STRING(
        "VKEY_REC/1 {\"seq\":3,\"state\":\"DONE\",\"ms\":1200,\"samples\":19200,\"rms\":800,\"peak\":9000,\"silence\":false}\n",
        record);
    ventured_rec_status_t fail = {.sequence = 4, .state = VENTURED_REC_FAIL};
    strncpy(fail.reason, "I2S", sizeof(fail.reason) - 1U);
    TEST_ASSERT_TRUE(ventured_format_rec_event(record, sizeof(record), &fail));
    TEST_ASSERT_EQUAL_STRING(
        "VKEY_REC/1 {\"seq\":4,\"state\":\"FAIL\",\"ms\":0,\"samples\":0,\"rms\":0,\"peak\":0,\"reason\":\"I2S\"}\n",
        record);
    strncpy(fail.reason, "WIFI", sizeof(fail.reason) - 1U);
    fail.sequence = 5;
    TEST_ASSERT_TRUE(ventured_format_rec_event(record, sizeof(record), &fail));
    TEST_ASSERT_EQUAL_STRING(
        "VKEY_REC/1 {\"seq\":5,\"state\":\"FAIL\",\"ms\":0,\"samples\":0,\"rms\":0,\"peak\":0,\"reason\":\"WIFI\"}\n",
        record);
    TEST_ASSERT_NULL(strstr(record, "VKEY_INPUT"));
}

void test_status_font_contains_chinese_and_ip_glyphs(void) {
    const uint32_t *rows = NULL;
    TEST_ASSERT_TRUE(ventured_font_bits(0x7F51, &rows));
    TEST_ASSERT_EQUAL_INT(32, VENTURED_FONT_ROWS);
    TEST_ASSERT_EQUAL_INT(32, VENTURED_FONT_COLS);
    unsigned ink = 0;
    for (int row = 0; row < VENTURED_FONT_ROWS; ++row) {
        uint32_t bits = rows[row];
        while (bits != 0U) {
            ink += bits & 1U;
            bits >>= 1;
        }
    }
    TEST_ASSERT_TRUE(ink > 200);
    TEST_ASSERT_TRUE(ventured_font_bits(0x5F55, &rows));
    TEST_ASSERT_TRUE(ventured_font_bits(0x97F3, &rows));
    TEST_ASSERT_TRUE(ventured_font_bits(0x5B8C, &rows));
    TEST_ASSERT_TRUE(ventured_font_bits(0x6210, &rows));
    TEST_ASSERT_TRUE(ventured_font_bits((uint32_t)'8', &rows));
    TEST_ASSERT_TRUE(ventured_font_bits(0x8F6C, &rows));
    TEST_ASSERT_TRUE(ventured_font_bits(0x5199, &rows));
    TEST_ASSERT_FALSE(ventured_font_bits(0x4E00, &rows));
}

void test_wav_header_is_pcm16_mono_16k(void) {
    uint8_t header[VENTURED_WAV_HEADER_BYTES];
    ventured_wav_write_header(header, 32000, 16000);
    TEST_ASSERT_TRUE(ventured_wav_header_is_pcm16_mono(header, 32000, 16000));
    TEST_ASSERT_EQUAL_INT('R', header[0]);
    TEST_ASSERT_EQUAL_UINT8(1, header[20]);
    TEST_ASSERT_EQUAL_UINT8(1, header[22]);
    TEST_ASSERT_EQUAL_UINT8(16, header[34]);
}

void test_sensor_event_formatter_is_exact(void) {
    char record[128];
    ventured_sensor_status_t ok = {
        .sequence = 1,
        .pir = true,
        .has_distance = true,
        .dist_mm = 312,
        .state = VENTURED_SENSOR_OK,
    };
    TEST_ASSERT_TRUE(ventured_format_sensor_event(record, sizeof(record), &ok));
    TEST_ASSERT_EQUAL_STRING(
        "VKEY_SENSOR/1 {\"seq\":1,\"pir\":true,\"state\":\"OK\",\"distMm\":312}\n",
        record);
    ventured_sensor_status_t fail = {
        .sequence = 2,
        .pir = false,
        .state = VENTURED_SENSOR_TOF,
    };
    TEST_ASSERT_TRUE(ventured_format_sensor_event(record, sizeof(record), &fail));
    TEST_ASSERT_EQUAL_STRING("VKEY_SENSOR/1 {\"seq\":2,\"pir\":false,\"state\":\"TOF\"}\n", record);
    TEST_ASSERT_NULL(strstr(record, "VKEY_INPUT"));
}

void test_asr_event_extracts_text_and_omits_secrets(void) {
    char text[64];
    TEST_ASSERT_TRUE(ventured_asr_extract_text("{\"text\":\"今天天气不错\"}", text, sizeof(text)));
    TEST_ASSERT_EQUAL_STRING("今天天气不错", text);
    ventured_asr_status_t done = {.sequence = 2, .state = VENTURED_ASR_DONE};
    strncpy(done.text, "今天天气不错", sizeof(done.text) - 1U);
    char record[256];
    TEST_ASSERT_TRUE(ventured_format_asr_event(record, sizeof(record), &done));
    TEST_ASSERT_EQUAL_STRING("VKEY_ASR/1 {\"seq\":2,\"state\":\"DONE\",\"text\":\"今天天气不错\"}\n", record);
    TEST_ASSERT_NULL(strstr(record, "apiKey"));
    TEST_ASSERT_NULL(strstr(record, "Bearer"));
    ventured_asr_status_t cancel = {.sequence = 3, .state = VENTURED_ASR_FAIL};
    strncpy(cancel.reason, "CANCEL", sizeof(cancel.reason) - 1U);
    TEST_ASSERT_TRUE(ventured_format_asr_event(record, sizeof(record), &cancel));
    TEST_ASSERT_EQUAL_STRING("VKEY_ASR/1 {\"seq\":3,\"state\":\"FAIL\",\"reason\":\"CANCEL\"}\n", record);
}
