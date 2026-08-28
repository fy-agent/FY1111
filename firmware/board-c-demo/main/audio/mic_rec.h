#pragma once

#include <stdbool.h>

#include "esp_err.h"

esp_err_t ventured_mic_rec_start(void);
void ventured_mic_rec_set_hand_held(bool held);
