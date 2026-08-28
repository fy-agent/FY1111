#include "config_rx.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "protocol/config_record.h"
#include "usb/ventured_link.h"
#include "wifi_sta.h"

static const char *TAG = "board_c_cfg";

static void config_task(void *arg) {
    (void)arg;
    char line[1025];
    for (;;) {
        if (!ventured_link_readline(line, sizeof(line), pdMS_TO_TICKS(50))) {
            continue;
        }
        ventured_device_config_t config;
        if (!ventured_parse_config_line(line, &config)) {
            memset(line, 0, sizeof(line));
            continue;
        }
        ESP_LOGI(TAG, "received wifi config");
        if (ventured_wifi_apply(&config) != ESP_OK) {
            ESP_LOGW(TAG, "wifi apply failed");
        }
        memset(config.password, 0, sizeof(config.password));
        memset(config.api_key, 0, sizeof(config.api_key));
        memset(line, 0, sizeof(line));
    }
}

esp_err_t ventured_config_rx_start(void) {
    if (xTaskCreate(config_task, "cfg_rx", 6144, NULL, 5, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
