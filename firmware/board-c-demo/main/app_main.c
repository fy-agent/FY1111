#include <stdio.h>

#include "esp_log.h"

#include "input/board_c_encoder.h"

void app_main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    ESP_ERROR_CHECK(board_c_encoder_start());
    ESP_LOGI("board_c_demo", "encoder demo ready; emits VKEY_INPUT/1 records");
}
