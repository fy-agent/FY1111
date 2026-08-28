#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "audio/mic_rec.h"
#include "display/st7789_status.h"
#include "input/board_c_encoder.h"
#include "net/asr_upload.h"
#include "net/config_rx.h"
#include "net/status_out.h"
#include "net/wifi_sta.h"
#include "protocol/net_event.h"

static const char *TAG = "board_c_demo";

static void on_net(const ventured_net_status_t *status) {
    char record[256];
    if (ventured_format_net_event(record, sizeof(record), status)) {
        fputs(record, stdout);
        fflush(stdout);
    }
    ventured_display_show_net(status);
}

void app_main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    ventured_wifi_set_listener(NULL);
    if (ventured_display_start() != ESP_OK) {
        ESP_LOGW(TAG, "status display unavailable; wifi continues");
    }
    ESP_ERROR_CHECK(ventured_status_out_start(on_net));
    ESP_ERROR_CHECK(ventured_wifi_start());
    ESP_ERROR_CHECK(ventured_config_rx_start());
    ESP_ERROR_CHECK(board_c_encoder_start());
    ESP_ERROR_CHECK(ventured_asr_start());
    ESP_ERROR_CHECK(ventured_mic_rec_start());
    ESP_LOGI(TAG, "encoder + wifi + mic + asr demo ready");
}
