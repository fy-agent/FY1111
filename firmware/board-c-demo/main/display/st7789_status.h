#pragma once

#include "esp_err.h"

#include "protocol/asr_event.h"
#include "protocol/net_event.h"
#include "protocol/rec_event.h"

esp_err_t ventured_display_start(void);
void ventured_display_show_net(const ventured_net_status_t *status);
void ventured_display_show_rec(const ventured_rec_status_t *status);
void ventured_display_show_asr(const ventured_asr_status_t *status);
