#include "wifi_sta.h"

#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "link_probe.h"
#include "status_out.h"

static const char *k_nvs = "ventured";
static ventured_net_listener_t s_listener;
static ventured_net_status_t s_status;
static char s_api_key[VENTURED_API_KEY_MAX + 1];
static char s_model[VENTURED_MODEL_MAX + 1];
static uint32_t s_seq;
static int s_retries;
static bool s_started;

static void remember_cloud(const ventured_device_config_t *config);

static void set_reason(const char *reason) {
    memset(s_status.reason, 0, sizeof(s_status.reason));
    if (reason != NULL) {
        strncpy(s_status.reason, reason, sizeof(s_status.reason) - 1);
    }
}

static void publish(ventured_net_state_t state) {
    s_status.state = state;
    if (state != VENTURED_NET_FAILED) {
        s_status.reason[0] = '\0';
    }
    s_status.sequence = ++s_seq;
    ventured_status_out_post(&s_status);
    if (s_listener) {
        /* Legacy hook kept for tests; production sink is status_out. */
    }
}

static const char *fail_reason_for(unsigned reason) {
    if (reason == WIFI_REASON_NO_AP_FOUND ||
        reason == WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY ||
        reason == WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD ||
        reason == WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD) {
        return ventured_ssid_looks_5g(s_status.ssid) ? "BAND" : "NO_AP";
    }
    if (reason == WIFI_REASON_AUTH_EXPIRE ||
        reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT ||
        reason == WIFI_REASON_AUTH_FAIL ||
        reason == WIFI_REASON_HANDSHAKE_TIMEOUT) {
        return "AUTH";
    }
    if (reason == WIFI_REASON_BEACON_TIMEOUT || reason == WIFI_REASON_CONNECTION_FAIL) {
        return "TIMEOUT";
    }
    return "UNKNOWN";
}

static int max_retries_for(unsigned reason) {
    if (reason == WIFI_REASON_NO_AP_FOUND ||
        reason == WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY ||
        reason == WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD ||
        reason == WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD) {
        return 1;
    }
    if (reason == WIFI_REASON_AUTH_EXPIRE ||
        reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT ||
        reason == WIFI_REASON_AUTH_FAIL ||
        reason == WIFI_REASON_HANDSHAKE_TIMEOUT) {
        return 2;
    }
    return 4;
}

static esp_err_t nvs_write_str(nvs_handle_t handle, const char *key, const char *value) {
    return nvs_set_str(handle, key, value == NULL ? "" : value);
}

static esp_err_t persist(const ventured_device_config_t *config) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(k_nvs, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    err = nvs_write_str(handle, "ssid", config->ssid);
    if (err == ESP_OK) err = nvs_write_str(handle, "pass", config->password);
    if (err == ESP_OK) err = nvs_write_str(handle, "key", config->api_key);
    if (err == ESP_OK) err = nvs_write_str(handle, "model", config->model);
    if (err == ESP_OK) err = nvs_commit(handle);
    if (err == ESP_OK) remember_cloud(config);
    nvs_close(handle);
    return err;
}

static bool load_saved(ventured_device_config_t *config) {
    nvs_handle_t handle;
    if (nvs_open(k_nvs, NVS_READONLY, &handle) != ESP_OK) return false;
    size_t ssid_len = sizeof(config->ssid);
    size_t pass_len = sizeof(config->password);
    size_t key_len = sizeof(config->api_key);
    size_t model_len = sizeof(config->model);
    bool ok = nvs_get_str(handle, "ssid", config->ssid, &ssid_len) == ESP_OK &&
              nvs_get_str(handle, "pass", config->password, &pass_len) == ESP_OK &&
              nvs_get_str(handle, "key", config->api_key, &key_len) == ESP_OK &&
              nvs_get_str(handle, "model", config->model, &model_len) == ESP_OK &&
              config->ssid[0] != '\0' && ventured_model_allowed(config->model);
    nvs_close(handle);
    return ok;
}

static void apply_sta_config(const ventured_device_config_t *config) {
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, config->ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, config->password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode =
        config->password[0] == '\0' ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA_PSK;
    wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    memset(wifi_config.sta.password, 0, sizeof(wifi_config.sta.password));
}

static void on_wifi(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg;
    (void)base;
    if (id == WIFI_EVENT_STA_START) {
        ventured_link_logf("sta start ssid=%s", s_status.ssid);
        if (s_status.ssid[0] != '\0') {
            esp_wifi_connect();
        }
        return;
    }
    if (id != WIFI_EVENT_STA_DISCONNECTED) return;
    unsigned reason = 0;
    if (data != NULL) {
        const wifi_event_sta_disconnected_t *event = data;
        reason = event->reason;
    }
    s_status.ip[0] = '\0';
    s_status.rssi = 0;
    if (s_retries < max_retries_for(reason)) {
        ++s_retries;
        ventured_link_logf("sta disconnect reason=%u retry=%d", reason, s_retries);
        publish(VENTURED_NET_CONNECTING);
        esp_wifi_connect();
        return;
    }
    set_reason(fail_reason_for(reason));
    ventured_link_logf("sta failed reason=%u code=%s", reason, s_status.reason);
    publish(VENTURED_NET_FAILED);
}

static void on_ip(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg;
    (void)base;
    if (id != IP_EVENT_STA_GOT_IP) return;
    const ip_event_got_ip_t *event = data;
    snprintf(s_status.ip, sizeof(s_status.ip), IPSTR, IP2STR(&event->ip_info.ip));
    wifi_ap_record_t ap = {0};
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        s_status.rssi = ap.rssi;
    }
    s_retries = 0;
    ventured_link_logf("sta got ip=%s rssi=%d ssid=%s", s_status.ip, s_status.rssi, s_status.ssid);
    publish(VENTURED_NET_CONNECTED);
    ventured_link_probe_kick();
}

void ventured_wifi_set_listener(ventured_net_listener_t listener) { s_listener = listener; }

void ventured_wifi_copy_status(ventured_net_status_t *out) {
    if (out) *out = s_status;
}

static void remember_cloud(const ventured_device_config_t *config) {
    memset(s_api_key, 0, sizeof(s_api_key));
    memset(s_model, 0, sizeof(s_model));
    if (config == NULL) return;
    strncpy(s_api_key, config->api_key, sizeof(s_api_key) - 1);
    strncpy(s_model, config->model, sizeof(s_model) - 1);
}

bool ventured_wifi_cloud_ready(void) {
    return s_status.state == VENTURED_NET_CONNECTED && s_api_key[0] != '\0' && s_model[0] != '\0';
}

bool ventured_wifi_copy_cloud(char *api_key, size_t key_len, char *model, size_t model_len) {
    if (api_key == NULL || key_len == 0 || model == NULL || model_len == 0) return false;
    memset(api_key, 0, key_len);
    memset(model, 0, model_len);
    strncpy(api_key, s_api_key, key_len - 1);
    strncpy(model, s_model, model_len - 1);
    return s_status.state == VENTURED_NET_CONNECTED && s_api_key[0] != '\0' && s_model[0] != '\0';
}

void ventured_wifi_publish_current(void) {
    if (s_status.state == VENTURED_NET_CONNECTED) {
        wifi_ap_record_t ap = {0};
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
            s_status.rssi = ap.rssi;
        }
    }
    publish(s_status.state);
}

esp_err_t ventured_wifi_apply(const ventured_device_config_t *config) {
    if (config == NULL || config->ssid[0] == '\0' || !ventured_model_allowed(config->model)) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = persist(config);
    if (err != ESP_OK) return err;
    memset(s_status.ssid, 0, sizeof(s_status.ssid));
    strncpy(s_status.ssid, config->ssid, sizeof(s_status.ssid) - 1);
    s_status.ip[0] = '\0';
    s_status.rssi = 0;
    s_retries = 0;
    set_reason(NULL);
    if (s_started) {
        esp_wifi_disconnect();
    }
    if (ventured_ssid_looks_5g(s_status.ssid)) {
        set_reason("BAND");
        ventured_link_logf("sta reject 5g ssid=%s (2.4g only)", s_status.ssid);
        publish(VENTURED_NET_FAILED);
        return ESP_OK;
    }
    apply_sta_config(config);
    ventured_link_logf("sta apply ssid=%s", s_status.ssid);
    publish(VENTURED_NET_CONNECTING);
    if (s_started) {
        esp_wifi_connect();
    }
    return ESP_OK;
}

esp_err_t ventured_wifi_start(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) return err;
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_ip, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    s_started = true;
    ESP_ERROR_CHECK(ventured_link_probe_start());
    publish(VENTURED_NET_DISCONNECTED);
    ventured_link_logf("wifi ready");
    ventured_device_config_t saved = {0};
    if (load_saved(&saved)) {
        err = ventured_wifi_apply(&saved);
        memset(saved.password, 0, sizeof(saved.password));
        memset(saved.api_key, 0, sizeof(saved.api_key));
    }
    return err;
}
