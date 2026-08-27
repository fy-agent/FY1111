#include "link_probe.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "ping/ping_sock.h"

#include "protocol/link_debug.h"
#include "status_out.h"
#include "wifi_sta.h"

static const char *TAG = "board_c_link";
static const char *k_hosts[] = {"8.8.8.8", "dns.google", "www.baidu.com"};
static uint32_t s_ping_seq;
static TaskHandle_t s_task;

typedef struct {
    uint32_t sent;
    uint32_t received;
    uint32_t total_ms;
    TaskHandle_t waiter;
} ping_wait_t;

static void emit_line(const char *line) {
    if (line == NULL || line[0] == '\0') return;
    fputs(line, stdout);
    fflush(stdout);
}

void ventured_link_logf(const char *fmt, ...) {
    char message[160];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    ESP_LOGI(TAG, "%s", message);
    ventured_status_out_log(message);
}

static void on_ping_success(esp_ping_handle_t handle, void *args) {
    uint32_t elapsed = 0;
    ping_wait_t *wait = args;
    esp_ping_get_profile(handle, ESP_PING_PROF_TIMEGAP, &elapsed, sizeof(elapsed));
    wait->received++;
    wait->total_ms += elapsed;
}

static void on_ping_timeout(esp_ping_handle_t handle, void *args) {
    (void)handle;
    (void)args;
}

static void on_ping_end(esp_ping_handle_t handle, void *args) {
    ping_wait_t *wait = args;
    uint32_t transmitted = 0;
    esp_ping_get_profile(handle, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    wait->sent = transmitted;
    if (wait->waiter) xTaskNotifyGive(wait->waiter);
    esp_ping_delete_session(handle);
}

static bool resolve_ipv4(const char *host, ip_addr_t *target) {
    struct addrinfo hint = {0};
    struct addrinfo *result = NULL;
    hint.ai_family = AF_INET;
    if (getaddrinfo(host, NULL, &hint, &result) != 0 || result == NULL) {
        return false;
    }
    const struct sockaddr_in *addr = (const struct sockaddr_in *)result->ai_addr;
    ip_addr_set_ip4_u32(target, addr->sin_addr.s_addr);
    freeaddrinfo(result);
    return true;
}

static void ping_one(const char *host) {
    ip_addr_t target;
    if (!resolve_ipv4(host, &target)) {
        ventured_link_logf("dns fail host=%s", host);
        char record[192];
        if (ventured_format_ping_event(record, sizeof(record), ++s_ping_seq, host, false, 0, 3, 0)) {
            emit_line(record);
        }
        return;
    }
    ping_wait_t wait = {
        .waiter = xTaskGetCurrentTaskHandle(),
    };
    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    config.target_addr = target;
    config.count = 3;
    config.interval_ms = 400;
    config.timeout_ms = 1500;
    config.task_stack_size = 3072;
    esp_ping_callbacks_t callbacks = {
        .cb_args = &wait,
        .on_ping_success = on_ping_success,
        .on_ping_timeout = on_ping_timeout,
        .on_ping_end = on_ping_end,
    };
    esp_ping_handle_t ping = NULL;
    if (esp_ping_new_session(&config, &callbacks, &ping) != ESP_OK) {
        ventured_link_logf("ping start fail host=%s", host);
        return;
    }
    esp_ping_start(ping);
    if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(8000)) == 0) {
        ventured_link_logf("ping wait timeout host=%s", host);
        esp_ping_stop(ping);
        esp_ping_delete_session(ping);
        return;
    }
    unsigned lost = wait.sent > wait.received ? wait.sent - wait.received : 0;
    bool ok = wait.received > 0;
    int ms = ok ? (int)(wait.total_ms / wait.received) : 0;
    ventured_link_logf("ping host=%s ok=%d ms=%d lost=%u/%u", host, (int)ok, ms, lost, wait.sent);
    char record[192];
    if (ventured_format_ping_event(record, sizeof(record), ++s_ping_seq, host, ok, ms, lost, wait.sent)) {
        emit_line(record);
    }
}

void ventured_link_probe_now(void) {
    ventured_net_status_t status;
    ventured_wifi_copy_status(&status);
    if (status.state != VENTURED_NET_CONNECTED) {
        ventured_link_logf("probe skipped state=%s", ventured_net_state_token(status.state));
        return;
    }
    ventured_wifi_publish_current();
    for (size_t i = 0; i < sizeof(k_hosts) / sizeof(k_hosts[0]); ++i) {
        ping_one(k_hosts[i]);
    }
}

static void probe_task(void *arg) {
    (void)arg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10000));
        ventured_net_status_t status;
        ventured_wifi_copy_status(&status);
        if (status.state != VENTURED_NET_CONNECTED) continue;
        ventured_link_logf("heartbeat ip=%s rssi=%d ssid=%s", status.ip, status.rssi, status.ssid);
        ventured_link_probe_now();
    }
}

esp_err_t ventured_link_probe_start(void) {
    if (s_task != NULL) return ESP_OK;
    if (xTaskCreate(probe_task, "link_probe", 4096, NULL, 4, &s_task) != pdPASS) {
        s_task = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void ventured_link_probe_kick(void) {
    if (s_task) xTaskNotifyGive(s_task);
}
