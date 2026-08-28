#pragma once

#include <stdbool.h>

#include "protocol/net_event.h"

const char *ventured_net_state_zh(ventured_net_state_t state);
const char *ventured_net_hero_zh(ventured_net_state_t state);
bool ventured_lcd_net_should_redraw(ventured_net_state_t prev_state, const char *prev_reason,
                                    ventured_net_state_t next_state, const char *next_reason);
