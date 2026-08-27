idf_component_register(
    SRCS
        "app_main.c"
        "input/ec11_core.c"
        "input/board_c_encoder.c"
        "protocol/input_event.c"
    INCLUDE_DIRS "." "input" "protocol"
)

