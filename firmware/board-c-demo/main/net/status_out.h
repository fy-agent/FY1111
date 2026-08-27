#pragma once

#include "esp_err.h"

#include "protocol/net_event.h"

typedef void (*ventured_status_sink_t)(const ventured_net_status_t *status);

esp_err_t ventured_status_out_start(ventured_status_sink_t sink);
void ventured_status_out_post(const ventured_net_status_t *status);
void ventured_status_out_log(const char *message);
