#include "unity.h"

#include <string.h>

#include "config_record.h"
#include "font16.h"
#include "link_debug.h"
#include "net_event.h"
#include "status_text.h"

void test_config_parser_accepts_open_network_and_allowed_model(void) {
    ventured_device_config_t config;
    TEST_ASSERT_TRUE(ventured_parse_config_line(
        "VKEY_CONFIG/1 {\"seq\":3,\"ssid\":\"cafe\",\"password\":\"\",\"apiKey\":\"sk-demo\",\"model\":\"FunAudioLLM/SenseVoiceSmall\"}\n",
        &config));
    TEST_ASSERT_EQUAL_UINT32(3, config.sequence);
    TEST_ASSERT_EQUAL_STRING("cafe", config.ssid);
    TEST_ASSERT_EQUAL_STRING("", config.password);
    TEST_ASSERT_EQUAL_STRING("sk-demo", config.api_key);
    TEST_ASSERT_TRUE(ventured_model_allowed(config.model));
}

void test_config_parser_rejects_unknown_fields_and_models(void) {
    ventured_device_config_t config;
    TEST_ASSERT_FALSE(ventured_parse_config_line(
        "VKEY_CONFIG/1 {\"seq\":1,\"ssid\":\"a\",\"password\":\"b\",\"apiKey\":\"c\",\"model\":\"FunAudioLLM/SenseVoiceSmall\",\"extra\":true}",
        &config));
    TEST_ASSERT_FALSE(ventured_parse_config_line(
        "VKEY_CONFIG/1 {\"seq\":1,\"ssid\":\"a\",\"password\":\"b\",\"apiKey\":\"c\",\"model\":\"unknown\"}",
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

void test_status_font_contains_chinese_and_ip_glyphs(void) {
    const uint16_t *rows = NULL;
    TEST_ASSERT_TRUE(ventured_font16_bits(0x7F51, &rows));
    TEST_ASSERT_TRUE(ventured_font16_bits((uint32_t)'8', &rows));
    TEST_ASSERT_FALSE(ventured_font16_bits(0x4E00, &rows));
}
