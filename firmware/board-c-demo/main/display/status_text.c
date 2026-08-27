#include "status_text.h"

const char *ventured_net_state_zh(ventured_net_state_t state) {
    switch (state) {
        case VENTURED_NET_CONNECTING: return "连接中";
        case VENTURED_NET_CONNECTED: return "已连接";
        case VENTURED_NET_FAILED: return "失败";
        case VENTURED_NET_DISCONNECTED:
        default: return "未连接";
    }
}
