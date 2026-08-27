#include "config_rx.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "esp_log.h"
#include "esp_vfs_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "protocol/config_record.h"
#include "wifi_sta.h"

static const char *TAG = "board_c_cfg";

static void config_task(void *arg) {
    (void)arg;
    setvbuf(stdin, NULL, _IONBF, 0);
    char line[1025];
    for (;;) {
        if (fgets(line, sizeof(line), stdin) == NULL) {
            vTaskDelay(pdMS_TO_TICKS(20));
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
    usb_serial_jtag_driver_config_t cfg = {
        .tx_buffer_size = 1024,
        .rx_buffer_size = 1024,
    };
    esp_err_t err = usb_serial_jtag_driver_install(&cfg);
    if (err != ESP_OK) return err;
    usb_serial_jtag_vfs_set_rx_line_endings(ESP_LINE_ENDINGS_LF);
    usb_serial_jtag_vfs_set_tx_line_endings(ESP_LINE_ENDINGS_LF);
    usb_serial_jtag_vfs_use_driver();
    fcntl(fileno(stdin), F_SETFL, 0);
    fcntl(fileno(stdout), F_SETFL, 0);
    if (xTaskCreate(config_task, "cfg_rx", 6144, NULL, 5, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
