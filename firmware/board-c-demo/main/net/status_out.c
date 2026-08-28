#include "status_out.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "protocol/link_debug.h"
#include "usb/ventured_link.h"

static QueueHandle_t s_status_q;
static QueueHandle_t s_log_q;
static ventured_status_sink_t s_sink;
static uint32_t s_log_seq;

static void status_task(void *arg) {
    (void)arg;
    for (;;) {
        char message[161];
        while (xQueueReceive(s_log_q, message, 0) == pdTRUE) {
            char record[256];
            if (ventured_format_log_event(record, sizeof(record), ++s_log_seq, message)) {
                ventured_link_write(record);
            }
        }
        ventured_net_status_t status;
        if (xQueueReceive(s_status_q, &status, pdMS_TO_TICKS(50)) == pdTRUE && s_sink) {
            s_sink(&status);
        }
    }
}

esp_err_t ventured_status_out_start(ventured_status_sink_t sink) {
    s_sink = sink;
    s_status_q = xQueueCreate(1, sizeof(ventured_net_status_t));
    s_log_q = xQueueCreate(8, 161);
    if (s_status_q == NULL || s_log_q == NULL) return ESP_ERR_NO_MEM;
    if (xTaskCreate(status_task, "status_out", 6144, NULL, 4, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void ventured_status_out_post(const ventured_net_status_t *status) {
    if (s_status_q == NULL || status == NULL) return;
    xQueueOverwrite(s_status_q, status);
}

void ventured_status_out_log(const char *message) {
    if (s_log_q == NULL || message == NULL || message[0] == '\0') return;
    char copy[161];
    memset(copy, 0, sizeof(copy));
    strncpy(copy, message, sizeof(copy) - 1);
    xQueueSend(s_log_q, copy, 0);
}
