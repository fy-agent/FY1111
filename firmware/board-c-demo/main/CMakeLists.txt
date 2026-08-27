idf_component_register(
    SRCS
        "app_main.c"
        "input/ec11_core.c"
        "input/board_c_encoder.c"
        "protocol/input_event.c"
        "protocol/config_record.c"
        "protocol/net_event.c"
        "protocol/link_debug.c"
        "net/wifi_sta.c"
        "net/config_rx.c"
        "net/link_probe.c"
        "net/status_out.c"
        "display/status_text.c"
        "display/font16.c"
        "display/st7789_status.c"
    INCLUDE_DIRS "." "input" "protocol" "net" "display"
    REQUIRES driver esp_wifi esp_event esp_netif nvs_flash esp_lcd lwip
)
