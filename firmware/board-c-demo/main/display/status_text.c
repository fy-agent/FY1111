#include "status_text.h"

#include <string.h>

const char *ventured_net_state_zh(ventured_net_state_t state) {
    switch (state) {
        case VENTURED_NET_CONNECTING: return "连接中";
        case VENTURED_NET_CONNECTED: return "已连接";
        case VENTURED_NET_FAILED: return "失败";
        case VENTURED_NET_DISCONNECTED:
        default: return "未连接";
    }
}

const char *ventured_net_hero_zh(ventured_net_state_t state) {
    if (state == VENTURED_NET_FAILED) return "连接失败";
    return ventured_net_state_zh(state);
}

bool ventured_lcd_net_should_redraw(ventured_net_state_t prev_state, const char *prev_reason,
                                    ventured_net_state_t next_state, const char *next_reason) {
    if (prev_state != next_state) return true;
    if (next_state != VENTURED_NET_FAILED) return false;
    const char *prev = prev_reason != NULL ? prev_reason : "";
    const char *next = next_reason != NULL ? next_reason : "";
    return strcmp(prev, next) != 0;
}
