#pragma once

#include "esp_err.h"

#include "protocol/config_record.h"
#include "protocol/net_event.h"

typedef void (*ventured_net_listener_t)(const ventured_net_status_t *status);

void ventured_wifi_set_listener(ventured_net_listener_t listener);
esp_err_t ventured_wifi_start(void);
esp_err_t ventured_wifi_apply(const ventured_device_config_t *config);
void ventured_wifi_copy_status(ventured_net_status_t *out);
bool ventured_wifi_cloud_ready(void);
bool ventured_wifi_copy_cloud(char *api_key, size_t key_len, char *model, size_t model_len);
void ventured_wifi_publish_current(void);
