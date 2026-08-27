#pragma once

#include "esp_err.h"

#include "protocol/net_event.h"

esp_err_t ventured_display_start(void);
void ventured_display_show_net(const ventured_net_status_t *status);
